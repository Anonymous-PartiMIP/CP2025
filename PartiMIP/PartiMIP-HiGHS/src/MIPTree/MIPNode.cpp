#include "MIPNode.h"

void MIPNode::DealOptimal(const HighsSolution &_reducedSolution)
{
  Postsolve(_reducedSolution);
  Tree_->SetNodeStatus(this, NodeStatus::End);
  RealseModel();
  SetProblemStatus(ProblemStatus::Optimal);
  UpPropagation();
  DownPropagation();
  if (Obj_ < TreeBestObj_)
  {
    TreeBestObj_ = Obj_;
    TreeBestNode_ = this;
  }
}

void MIPNode::DealInfeasible()
{
  Tree_->SetNodeStatus(this, NodeStatus::End);
  RealseModel();
  SetProblemStatus(ProblemStatus::Infeasible);
  UpPropagation();
  DownPropagation();
}

void MIPNode::DealFeasible(const HighsSolution &_reducedSolution)
{
  Postsolve(_reducedSolution);
  SetProblemStatus(ProblemStatus::Feasible);
  if (Obj_ < TreeBestObj_)
  {
    TreeBestObj_ = Obj_;
    TreeBestNode_ = this;
  }
}

void MIPNode::DealEndFeasible(const HighsSolution &_reducedSolution, const double _obj)
{
  if (_obj < TreeBestObj_ - 1e-4)
  {
    printf("c %10.2lf    [%-10s]    Node(%ld) [%lf] [%lf] [%lf]\n",
           ElapsedTime(), "End Feas", nodeID_, GetObj(), _obj, TreeBestObj_);
    Postsolve(_reducedSolution);
    if (Obj_ < TreeBestObj_)
    {
      TreeBestObj_ = Obj_;
      TreeBestNode_ = this;
    }
  }
}

void MIPNode::DealUnknown()
{
}

void MIPNode::RealseModel()
{
  highs_.clear();
}

MIPTree *MIPNode::Tree_ = nullptr;
size_t MIPNode::NODEID_ = 0;
// size_t MIPNode::BranchTimes_ = 0;
double MIPNode::TreeBestObj_ = INF;
MIPNode *MIPNode::TreeBestNode_ = nullptr;
map<string, size_t> MIPNode::VarBranchInSolved_;
// map<string, size_t> MIPNode::VarTabu_;
boost::mutex MIPNode::mutexNODEID__;
// boost::mutex MIPNode::mutexBranchTimes_;

MIPNode::MIPNode(
    const size_t _depth,
    MIPNode *_parent,
    const HighsModel &_oriModel,
    const double objCutValue_)
    : depth_(_depth),
      parentNode_(_parent),
      presolve_(_oriModel, Tree_->GetInitDone()),
      problemStatus_(ProblemStatus::Unknown),
      nodeStatus_(NodeStatus::New),
      leftNode_(nullptr),
      rightNode_(nullptr),
      varNum_(0),
      conNum_(0),
      nonzeroNum_(0),
      Obj_(INF),
      worker_(nullptr),
      objCutValue_(objCutValue_),
      runningStartTime_(INF)
{
  {
    boost::mutex::scoped_lock lock(mutexNODEID__);
    nodeID_ = NODEID_;
    NODEID_++;
  }
}

MIPNode::~MIPNode()
{
  if (leftNode_ != nullptr)
    delete leftNode_;
  if (rightNode_ != nullptr)
    delete rightNode_;
  leftNode_ = nullptr;
  rightNode_ = nullptr;
}

void MIPNode::ReducedModel()
{
  presolve_.PresolveByHighs();
}

void MIPNode::Activate()
{
  if (CheckPresolveInfeas())
    return;
  if (CheckPresolveOptimal())
    return;
  InitModel();
}

vector<MIPNode *> MIPNode::SelectVarToBrach()
{
  bestIndex_ = -1;
  bestShortDegree_ = 0;
  bestVarDegree_ = 0;
  bestType_ = 'R';
  bestVarBranchInSolved_ = 0;
  candidateVars_.clear();
  for (size_t idx = 0; idx < varNum_; ++idx)
  {
    const size_t &shortDegree = shortDegree_[idx];
    const size_t &varDegree = varDegree_[idx];
    const string &varName = highs_.getLp().col_names_[idx];
    // const auto tabu_it = VarTabu_.find(varName);
    // bool isTabu = (tabu_it != VarTabu_.end() && tabu_it->second > BranchTimes_);
    if (highs_.getLp().col_lower_[idx] + OPT(Obstacle) > highs_.getLp().col_upper_[idx] ||
        (-INF >= highs_.getLp().col_lower_[idx] || highs_.getLp().col_upper_[idx] >= INF))
      continue;
    const char &type = varType_[idx];
    size_t varBranchInSolved = 0;
    const auto it = VarBranchInSolved_.find(varName);
    if (it != VarBranchInSolved_.end())
      varBranchInSolved = it->second;
    if (Branch(idx, shortDegree, varDegree, varBranchInSolved, type))
    {
      candidateVars_.clear();
      bestIndex_ = idx;
      bestShortDegree_ = shortDegree;
      bestVarDegree_ = varDegree;
      bestVarBranchInSolved_ = varBranchInSolved;
      bestType_ = type;
    }
  }
  if (bestIndex_ == -1)
  {
    for (size_t idx = 0; idx < varNum_; ++idx)
    {
      const size_t &shortDegree = shortDegree_[idx];
      const size_t &varDegree = varDegree_[idx];
      const string &varName = highs_.getLp().col_names_[idx];
      const char &type = varType_[idx];
      size_t varBranchInSolved = 0;
      const auto it = VarBranchInSolved_.find(varName);
      if (it != VarBranchInSolved_.end())
        varBranchInSolved = it->second;
      if (Branch(idx, shortDegree, varDegree, varBranchInSolved, type))
      {
        candidateVars_.clear();
        bestIndex_ = idx;
        bestShortDegree_ = shortDegree;
        bestVarDegree_ = varDegree;
        bestVarBranchInSolved_ = varBranchInSolved;
        bestType_ = type;
      }
    }
  }
  assert(bestIndex_ != -1);
  candidateVars_.push_back(bestIndex_);
  bestIndex_ = candidateVars_[rand() % candidateVars_.size()];
  branchVarName_ = highs_.getLp().col_names_[bestIndex_];
  const double bestLB = highs_.getLp().col_lower_[bestIndex_];
  const double bestUB = highs_.getLp().col_upper_[bestIndex_];
  double mid = GetMid(bestLB, bestUB);
  bestType_ = varType_[bestIndex_];
  bestShortDegree_ = shortDegree_[bestIndex_];
  bestVarDegree_ = varDegree_[bestIndex_];
  if (VarBranchInSolved_.find(branchVarName_) != VarBranchInSolved_.end())
    bestVarBranchInSolved_ = VarBranchInSolved_[branchVarName_];
  else
    bestVarBranchInSolved_ = 0;
  DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) ---> Var(%s) \
   [ %.2lf | %.2lf | %.2lf ] [ %c | %ld | %ld | %ld ] (#%ld) \n",
              ElapsedTime(), "Branch", nodeID_, branchVarName_.c_str(), bestLB, mid, bestUB,
              bestType_, bestVarBranchInSolved_, bestShortDegree_, bestVarDegree_, candidateVars_.size());

  // UpdateTabu(branchVarName_);
  vector<MIPNode *> newNodes;
  ObjCut();
  if (highs_.getLp().integrality_.size() == varNum_ &&
      highs_.getLp().integrality_[bestIndex_] == HighsVarType::kInteger)
    highs_.changeColBounds(bestIndex_, bestLB, floor(mid));
  else
    highs_.changeColBounds(bestIndex_, bestLB, mid);
  MIPNode *left = new MIPNode(depth_ + 1, this, highs_.getModel(), objCutValue_);
  newNodes.push_back(left);
  this->leftNode_ = left;
  if (highs_.getLp().integrality_.size() == varNum_ &&
      highs_.getLp().integrality_[bestIndex_] == HighsVarType::kInteger)
    highs_.changeColBounds(bestIndex_, ceil(mid), bestUB);
  else
    highs_.changeColBounds(bestIndex_, mid, bestUB);
  MIPNode *right = new MIPNode(depth_ + 1, this, highs_.getModel(), objCutValue_);
  newNodes.push_back(right);
  this->rightNode_ = right;

  highs_.changeColBounds(bestIndex_, bestLB, bestUB);
  return newNodes;
}

void MIPNode::ObjCut()
{
  if (Tree_->HaveGlobalIncumbent())
  {
    const double incunmbentValue = Tree_->GetGlobalIncumbent();
    if (incunmbentValue < objCutValue_)
    {
      objCutValue_ = incunmbentValue;
      const vector<double> &col_cost = highs_.getLp().col_cost_;
      HighsInt num_new_nz = 0;
      for (HighsInt iCol = 0; iCol < highs_.getNumCol(); iCol++)
        if (col_cost[iCol] != 0.0)
          num_new_nz++;
      HighsInt *indices = new HighsInt[num_new_nz];
      double *values = new double[num_new_nz];
      size_t idx = 0;
      for (HighsInt iCol = 0; iCol < highs_.getNumCol(); iCol++)
        if (col_cost[iCol] != 0.0)
        {
          indices[idx] = iCol;
          values[idx] = col_cost[iCol];
          idx++;
        }
      assert(idx == num_new_nz);
      highs_.addRow(-INF, incunmbentValue - highs_.getLp().offset_, num_new_nz, indices, values);
      DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) [%lf]\n",
                  ElapsedTime(), "Obj Cut", nodeID_, incunmbentValue);
      delete[] indices;
      delete[] values;
    }
  }
}

void MIPNode::InitModel()
{
  highs_.setOptionValue("log_to_console", "false");
  if (Tree_->GetInitDone())
    highs_.setOptionValue("threads", 1);
  else
    highs_.setOptionValue("threads", OPT(threadNum) / 2);
  highs_.passModel(presolve_.GetReducedModel());
  varNum_ = highs_.getNumCol();
  shortDegree_.resize(varNum_, 0);
  varDegree_.resize(varNum_, 0);
  varType_.resize(varNum_, 'R');
  const HighsLp &lp = highs_.getLp();
  if (lp.integrality_.size() == varNum_)
    for (size_t i = 0; i < varNum_; ++i)
    {
      varType_[i] =
          lp.integrality_[i] == HighsVarType::kInteger
              ? 'I'
              : 'R';
    }
  conNum_ = highs_.getNumRow();
  nonzeroNum_ = highs_.getNumNz();
  for (int i = 0; i < varNum_; i++)
    varDegree_[i] = lp.a_matrix_.start_[i + 1] - lp.a_matrix_.start_[i];
}

bool MIPNode::CheckPresolveInfeas()
{
  if (presolve_.CheckPresolveInfeas())
  {
    Tree_->SetNodeStatus(this, NodeStatus::End);
    RealseModel();
    SetProblemStatus(ProblemStatus::Infeasible);
    DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) Presolve Infeasible\n",
                ElapsedTime(), "End", nodeID_);
    UpPropagation();
    return true;
  }
  return false;
}

bool MIPNode::CheckPresolveOptimal()
{
  if (presolve_.CheckPresolveOptimal())
  {
    assert(!IsEnd() && varNum_ == 0);
    Postsolve();
    Tree_->SetNodeStatus(this, NodeStatus::End);
    RealseModel();
    SetProblemStatus(ProblemStatus::Optimal);
    DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) Presolve Optimal\n",
                ElapsedTime(), "End", nodeID_);
    UpPropagation();
    if (Obj_ < TreeBestObj_)
    {
      TreeBestObj_ = Obj_;
      TreeBestNode_ = this;
    }
    return true;
  }
  return false;
}

void MIPNode::UpPropagation()
{
  MIPNode *now = this;
  MIPNode *parent = parentNode_;
  while (parent != nullptr)
  {
    if (parent->leftNode_->IsEnd() && parent->rightNode_->IsEnd())
    {
      if (parent->GetNodeStatus() != NodeStatus::BranchedRunning &&
          parent->GetNodeStatus() != NodeStatus::BranchedWaiting)
        DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) NodeStatus(%s)\n",
                    ElapsedTime(), "Warning", parent->nodeID_,
                    NodeStatusToString(parent->GetNodeStatus()));
      if (parent->GetNodeStatus() == NodeStatus::BranchedRunning)
        parent->worker_->EndRunning();
      Tree_->SetNodeStatus(parent, NodeStatus::End);
      parent->IncreaseVarBranchInSolved();
      if (parent->leftNode_->IsInfeasible() && parent->rightNode_->IsInfeasible())
      {
        DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) Infeasible\n",
                    ElapsedTime(), "Up Prop", parent->nodeID_);
        parent->SetProblemStatus(ProblemStatus::Infeasible);
      }
      else
      {
        DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) Optimal\n",
                    ElapsedTime(), "Up Prop", parent->nodeID_);
        parent->SetProblemStatus(ProblemStatus::Optimal);
        // if (parent->leftNode_->IsOptimal() && parent->rightNode_->IsOptimal())
        // {
        //   if (parent->leftNode_->Obj_ < parent->rightNode_->Obj_)
        //   {
        //     parent->Obj_ = parent->leftNode_->Obj_;
        //     parent->Postsolve(parent->leftNode_->presolve_.GetOriSolution());
        //   }
        //   else
        //   {
        //     parent->Obj_ = parent->rightNode_->Obj_;
        //     parent->Postsolve(parent->rightNode_->presolve_.GetOriSolution());
        //   }
        // }
        // else if (parent->leftNode_->IsOptimal() && parent->rightNode_->IsInfeasible())
        // {
        //   parent->Obj_ = parent->leftNode_->Obj_;
        //   parent->Postsolve(parent->leftNode_->presolve_.GetOriSolution());
        // }
        // else
        // {
        //   assert(parent->leftNode_->IsInfeasible() && parent->rightNode_->IsOptimal());
        //   parent->Obj_ = parent->rightNode_->Obj_;
        //   parent->Postsolve(parent->rightNode_->presolve_.GetOriSolution());
        // }
      }
      now = parent;
      parent = parent->parentNode_;
    }
    else
      break;
  }
}

void MIPNode::DownPropagation()
{
  if (leftNode_ != nullptr)
  {
    DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) %s\n",
                ElapsedTime(), "Down Prop", leftNode_->nodeID_,
                ProblemStatusToString(problemStatus_));
    if (leftNode_->GetNodeStatus() == NodeStatus::Running ||
        leftNode_->GetNodeStatus() == NodeStatus::BranchedRunning)
      leftNode_->worker_->EndRunning();
    Tree_->SetNodeStatus(leftNode_, NodeStatus::End);
    leftNode_->SetProblemStatus(problemStatus_);
    leftNode_->DownPropagation();
  }
  if (rightNode_ != nullptr)
  {
    DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) %s\n",
                ElapsedTime(), "Down Prop", rightNode_->nodeID_,
                ProblemStatusToString(problemStatus_));
    if (rightNode_->GetNodeStatus() == NodeStatus::Running ||
        rightNode_->GetNodeStatus() == NodeStatus::BranchedRunning)
      rightNode_->worker_->EndRunning();
    Tree_->SetNodeStatus(rightNode_, NodeStatus::End);
    rightNode_->SetProblemStatus(problemStatus_);
    rightNode_->DownPropagation();
  }
}

void MIPNode::SolPropagation()
{
  MIPNode *now = this;
  MIPNode *parent = parentNode_;
  while (parent != nullptr)
  {
    double tempObj = now->Obj_;
    if (tempObj <= parent->Obj_ + 1e-6)
    {
      // assert(!parent->IsInfeasible());
      if (parent->IsInfeasible())
        DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) [%lf]\n",
                    ElapsedTime(), "WARNING", parent->nodeID_, tempObj);
      if (!parent->IsOptimal())
        parent->SetProblemStatus(ProblemStatus::Feasible);
      parent->Obj_ = tempObj;
      parent->Postsolve(now->presolve_.GetOriSolution());
      now = parent;
      parent = parent->parentNode_;
    }
    else
      break;
  }
}

void MIPNode::Postsolve()
{
  presolve_.PostSolveByHighs();
  Obj_ = presolve_.GetOriObj();
  DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) [%lf]\n",
              ElapsedTime(), "UPDATE OBJ", nodeID_, GetObj());
}

void MIPNode::Postsolve(const HighsSolution &_reducedSolution)
{
  presolve_.PostSolveByHighs(_reducedSolution);
  Obj_ = presolve_.GetOriObj();
  DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) [%lf]\n",
              ElapsedTime(), "UPDATE OBJ", nodeID_, GetObj());
}

void MIPNode::IncreaseVarBranchInSolved()
{
  if (VarBranchInSolved_.find(branchVarName_) != VarBranchInSolved_.end())
    VarBranchInSolved_[branchVarName_] += OPT(DecreaseVar);
  else
    VarBranchInSolved_[branchVarName_] = OPT(DecreaseVar);
  DEBUG_PRINT("c %10.2lf    [%-10s]    Var(%s) [%ld]\n",
              ElapsedTime(), "Inc Score", branchVarName_.c_str(),
              VarBranchInSolved_[branchVarName_]);
}

void MIPNode::UpdateVarMsg()
{
  if (VarBranchInSolved_.find(branchVarName_) != VarBranchInSolved_.end() &&
      VarBranchInSolved_[branchVarName_] > 0)
  {
    VarBranchInSolved_[branchVarName_] -= 1;
    DEBUG_PRINT("c %10.2lf    [%-10s]    Var(%s) [%ld]\n",
                ElapsedTime(), "Dec Score", branchVarName_.c_str(),
                VarBranchInSolved_[branchVarName_]);
  }
}

// void MIPNode::UpdateTabu(const string &_branchVarName)
// {
//   boost::mutex::scoped_lock lock(mutexBranchTimes_);
//   BranchTimes_++;
//   VarTabu_[_branchVarName] = BranchTimes_ + OPT(TabuBase);
// }
