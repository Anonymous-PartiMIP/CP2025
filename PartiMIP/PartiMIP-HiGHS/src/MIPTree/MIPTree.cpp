#include "MIPTree.h"

vector<MIPNode *> MIPTree::tempNewNodes_;
boost::mutex MIPTree::mutexTempNewNodes_;

bool LaunchStart = false;

void *Global_ActivateNode(void *arg)
{
  MIPNode *node = (MIPNode *)arg;
  node->ReducedModel();
  return nullptr;
}

void *Global_PartitionNode(void *arg)
{
  MIPNode *node = (MIPNode *)arg;
  DEBUG_PRINT("c %10.2lf    [%-10s]    Partition Node(%ld) \n",
              ElapsedTime(), "Partition", node->GetNodeID());
  vector<MIPNode *> newNodes = node->SelectVarToBrach();
  MIPTree::ActivateNodes(newNodes);
  MIPTree::InsertTempNewNodes(newNodes);
  return nullptr;
}

void MIPTree::InsertTempNewNodes(const vector<MIPNode *> &_newNodes)
{
  boost::mutex::scoped_lock lock(mutexTempNewNodes_);
  for (MIPNode *node : _newNodes)
    tempNewNodes_.push_back(node);
}

MIPTree::MIPTree()
    : coreNum_(OPT(threadNum) - 1),
      initDone_(false),
      solveTime_(INF),
      scheduler_(nullptr),
      rootNode_(nullptr),
      informWorkerNum_(0)
{
  MIPNode::Tree_ = this;
}

MIPTree::~MIPTree()
{
  delete rootNode_;
}

void MIPTree::BuildRootNode()
{
  rootNode_ = new MIPNode(1, nullptr, scheduler_->GetRootModel(), INF);
  rootNode_->ReducedModel();
  rootNode_->Activate();
  InsertWaitingNodes(rootNode_);
  if (!waitingNodes_.empty())
  {
    MIPNode *selectedNode = SelectWaitingNodeToBranch();
    vector<MIPNode *> newNodes = selectedNode->SelectVarToBrach();
    selectedNode->UpdateVarMsg();
    ActivateNodes(newNodes);
    for (MIPNode *node : newNodes)
    {
      node->Activate();
      InsertWaitingNodes(node);
    }
  }
}

void MIPTree::InsertWaitingNodes(MIPNode *_mipNode)
{
  if (!_mipNode->IsEnd())
  {
    waitingNodes_.insert(_mipNode);
    SetNodeStatus(_mipNode, NodeStatus::Waiting);
    DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) %6ld level  %10ld vars %10ld cons %10ld nonzero; Parent(%ld)\n",
                ElapsedTime(), "Waiting", _mipNode->GetNodeID(), _mipNode->GetDepth(),
                _mipNode->GetVarNum(), _mipNode->GetConNum(), _mipNode->GetNonzeroNum(),
                _mipNode->GetNodeID() == 0 ? 0 : _mipNode->GetParent()->GetNodeID());
  }
  else
  {
    endNodes_.insert(_mipNode);
    DEBUG_PRINT("c %10.2lf    [%-10s]    Node(%ld) %6ld level  %10ld vars %10ld cons %10ld nonzero; Parent(%ld)\n",
                ElapsedTime(), "End", _mipNode->GetNodeID(), _mipNode->GetDepth(),
                _mipNode->GetVarNum(), _mipNode->GetConNum(), _mipNode->GetNonzeroNum(),
                _mipNode->GetNodeID() == 0 ? 0 : _mipNode->GetParent()->GetNodeID());
  }
}

MIPNode *MIPTree::SelectWaitingNodeToBranch()
{
  assert(!waitingNodes_.empty());
  MIPNode *selectedNode = *(waitingNodes_.begin());
  assert(selectedNode->GetNodeStatus() == NodeStatus::Waiting);
  SetNodeStatus(selectedNode, NodeStatus::BranchedWaiting);
  DEBUG_PRINT("c %10.2lf    [%-10s]    Waiting Node(%ld)\n",
              ElapsedTime(), "Select", selectedNode->GetNodeID());
  return selectedNode;
}

MIPNode *MIPTree::SelectRunningNodeToBranch()
{
  assert(!runningNodes_.empty());
  MIPNode *selectedNode = *(runningNodes_.begin());
  assert(selectedNode->GetNodeStatus() == NodeStatus::Running);
  SetNodeStatus(selectedNode, NodeStatus::BranchedRunning);
  DEBUG_PRINT("c %10.2lf    [%-10s]    Running Node(%ld)\n",
              ElapsedTime(), "Select", selectedNode->GetNodeID());
  return selectedNode;
}

void MIPTree::ActivateNodes(const vector<MIPNode *> &_newNodes)
{
  pthread_t *activatePtr = new pthread_t[_newNodes.size()];
  for (size_t i = 0; i < _newNodes.size(); ++i)
    pthread_create(&activatePtr[i], NULL, Global_ActivateNode, _newNodes[i]);
  for (size_t i = 0; i < _newNodes.size(); ++i)
    pthread_join(activatePtr[i], NULL);
  delete[] activatePtr;
}

void MIPTree::BuildInitNodes()
{
  boost::mutex::scoped_lock lock(mutexTree_);
  auto t1 = chrono::high_resolution_clock::now();
  BuildRootNode();
  size_t totalNodes = coreNum_ * OPT(initNodes);
  while (
      !scheduler_->IsRootWorkerDone() &&
      ElapsedTime() + 10 < OPT(cutoff) &&
      !waitingNodes_.empty() &&
      waitingNodes_.size() < totalNodes)
  {
    vector<MIPNode *> selectNodes;
    while (!waitingNodes_.empty() &&
           selectNodes.size() * 2 + waitingNodes_.size() < totalNodes &&
           selectNodes.size() * 2 < coreNum_)
      selectNodes.push_back(SelectWaitingNodeToBranch());
    DEBUG_PRINT("c %10.2lf    [%-10s]    Partition %ld nodes, remaining %ld nodes in waiting.\n",
                ElapsedTime(), "Init Parti", selectNodes.size(), waitingNodes_.size());
    tempNewNodes_.clear();
    PartitionNodes(selectNodes);
    for (MIPNode *node : tempNewNodes_)
    {
      node->Activate();
      InsertWaitingNodes(node);
    }
  }
  auto t2 = chrono::high_resolution_clock::now();
  DEBUG_PRINT("c %10.2lf    [%-10s]    [%ld]\n",
              ElapsedTime(), "Init Time", chrono::duration_cast<chrono::seconds>(t2 - t1).count());
  if (rootNode_->IsEnd())
    solveTime_ = ElapsedTime();
  initDone_ = true;
}

void MIPTree::PartitionNodes(const vector<MIPNode *> &_selectNodes)
{
  pthread_t *partitionPtr = new pthread_t[_selectNodes.size()];
  for (size_t i = 0; i < _selectNodes.size(); ++i)
    pthread_create(&partitionPtr[i], NULL, Global_PartitionNode, _selectNodes[i]);
  for (size_t i = 0; i < _selectNodes.size(); ++i)
    pthread_join(partitionPtr[i], NULL);
  delete[] partitionPtr;
  for (MIPNode *node : _selectNodes)
    node->UpdateVarMsg();
}

vector<MIPNode *> MIPTree::GetInitNodesToRun()
{
  boost::mutex::scoped_lock lock(mutexTree_);
  vector<MIPNode *> initNodes;
  for (MIPNode *node : waitingNodes_)
  {
    initNodes.push_back(node);
    if (initNodes.size() >= coreNum_)
      break;
  }
  for (MIPNode *node : initNodes)
    SetNodeStatus(node, NodeStatus::Running);
  DEBUG_PRINT("c %10.2lf    [%-10s]    Send %ld nodes to run, remaining %ld nodes in waiting. (",
              ElapsedTime(), "Run", initNodes.size(), waitingNodes_.size());
  for (MIPNode *node : initNodes)
    DEBUG_PRINT("%ld, ", node->GetNodeID());
  DEBUG_PRINT(")\n");
  LaunchStart = true;
  return initNodes;
}

void MIPTree::SetNodeStatus(MIPNode *_node, NodeStatus _nodeStatus)
{
  if (_node->GetNodeStatus() == _nodeStatus)
    return;
  switch (_node->GetNodeStatus())
  {
  case NodeStatus::New:
    break;
  case NodeStatus::Waiting:
    // assert(waitingNodes_.find(_node) != waitingNodes_.end());
    waitingNodes_.erase(_node);
    break;
  case NodeStatus::Running:
    // assert(runningNodes_.find(_node) != runningNodes_.end());
    runningNodes_.erase(_node);
    break;
  case NodeStatus::BranchedWaiting:
    // assert(branchedWaitingNodes_.find(_node) != branchedWaitingNodes_.end());
    branchedWaitingNodes_.erase(_node);
    break;
  case NodeStatus::BranchedRunning:
    // assert(branchedRunningNodes_.find(_node) != branchedRunningNodes_.end());
    branchedRunningNodes_.erase(_node);
    break;
  case NodeStatus::End:
    assert(false);
    break;
  default:
    break;
  }
  _node->SetNodeStatus(_nodeStatus);
  switch (_nodeStatus)
  {
  case NodeStatus::New:
    assert(false);
    break;
  case NodeStatus::Waiting:
    waitingNodes_.insert(_node);
    break;
  case NodeStatus::Running:
    _node->SetRunningStartTime();
    runningNodes_.insert(_node);
    break;
  case NodeStatus::BranchedWaiting:
    branchedWaitingNodes_.insert(_node);
    break;
  case NodeStatus::BranchedRunning:
    branchedRunningNodes_.insert(_node);
    break;
  case NodeStatus::End:
    endNodes_.insert(_node);
    break;
  default:
    break;
  }
  if (rootNode_ == _node && _nodeStatus == NodeStatus::End)
    solveTime_ = ElapsedTime();
}

MIPNode *MIPTree::GetNodeToRun()
{
  boost::mutex::scoped_lock lock(mutexTree_);
  if (IsEnd() || scheduler_->IsRootWorkerDone() ||
      OPT(cutoff) < ElapsedTime() + 10)
    return nullptr;
  MIPNode *resNode = nullptr;
  bool success = false;
  if (!waitingNodes_.empty())
  {
    resNode = *(waitingNodes_.begin());
    SetNodeStatus(resNode, NodeStatus::Running);
    success = true;
  }
  else
  {
    while (waitingNodes_.empty() && !runningNodes_.empty())
    {
      vector<MIPNode *> selectNodes;
      auto idleNum_ = scheduler_->GetIdleWorkerNum();
      while (!runningNodes_.empty() &&
             selectNodes.size() * 2 < idleNum_)
        selectNodes.push_back(SelectRunningNodeToBranch());
      DEBUG_PRINT("c %10.2lf    [%-10s]    Partition %ld nodes, remaining %ld nodes in running, %ld inform, %ld idle.\n",
                  ElapsedTime(), "Dyna Parti", selectNodes.size(), runningNodes_.size(), (size_t)informWorkerNum_, idleNum_);
      tempNewNodes_.clear();
      PartitionNodes(selectNodes);
      for (MIPNode *node : tempNewNodes_)
      {
        node->Activate();
        InsertWaitingNodes(node);
      }
    }
    if (!waitingNodes_.empty())
    {
      resNode = *(waitingNodes_.begin());
      SetNodeStatus(resNode, NodeStatus::Running);
      success = true;
    }
  }
  if (success)
    DEBUG_PRINT("c %10.2lf    [%-10s]    Send Node(%ld) to run, remaining %ld nodes in waiting, %ld inform.\n",
                ElapsedTime(), "Run", (resNode)->GetNodeID(), waitingNodes_.size(), (size_t)informWorkerNum_);
  else
  {
    DEBUG_PRINT("c %10.2lf    [%-10s]    no nodes in waiting and waitrunning.\n",
                ElapsedTime(), "NOTHING");
    DEBUG_PRINT("c %10.2lf    [%-10s]    %ld; %ld; %ld; %ld\n",
                ElapsedTime(), "QUEUE", waitingNodes_.size(),
                branchedWaitingNodes_.size(), runningNodes_.size(), branchedRunningNodes_.size());
    DEBUG_PRINT("c %10.2lf    [%-10s]    remaining %ld nodes in branchedWaitingNodes_. (",
                ElapsedTime(), "WARNING", branchedWaitingNodes_.size());
    for (MIPNode *node : branchedWaitingNodes_)
      DEBUG_PRINT("%ld, ", node->GetNodeID());
    DEBUG_PRINT(")\n");
    assert(IsEnd());
  }

  return resNode;
}

void MIPTree::InformNodeResult(
    MIPNode *_node, const HighsModelStatus &_status,
    bool _haveIncumbent, const HighsSolution &_reducedSolution,
    const double _obj)
{
  boost::mutex::scoped_lock lock(mutexTree_);
  if (_node->IsEnd())
  {
    _node->RealseModel();
    if (_haveIncumbent)
      _node->DealEndFeasible(_reducedSolution, _obj);
    return;
  }
  if (_status == HighsModelStatus::kOptimal)
    _node->DealOptimal(_reducedSolution);
  else if (_status == HighsModelStatus::kInfeasible ||
           _status == HighsModelStatus::kUnboundedOrInfeasible)
    _node->DealInfeasible();
  else if (_haveIncumbent)
  {
    assert(_status == HighsModelStatus::kTimeLimit ||
           _status == HighsModelStatus::kInterrupt);
    _node->DealFeasible(_reducedSolution);
  }
  else if (!_haveIncumbent)
    _node->DealUnknown();
  else
    assert(false);
}

const bool MIPTree::HaveGlobalIncumbent() { return scheduler_->HaveIncumbent(); }

const double MIPTree::GetGlobalIncumbent() { return scheduler_->GetIncumbent(); }

bool NodeSelection_Waiting::operator()(const MIPNode *a, const MIPNode *b) const
{
  if (a->GetNonzeroNum() != b->GetNonzeroNum())
    return a->GetNonzeroNum() > b->GetNonzeroNum();
  else if (a->GetVarNum() != b->GetVarNum())
    return a->GetVarNum() > b->GetVarNum();
  else if (a->GetConNum() != b->GetConNum())
    return a->GetConNum() > b->GetConNum();
  else
    return a->GetNodeID() < b->GetNodeID();
}

bool NodeSelection_Running::operator()(const MIPNode *a, const MIPNode *b) const
{

  double scoreA = a->GetNonzeroNum() / a->GetRunningStartTime();
  double scoreB = b->GetNonzeroNum() / b->GetRunningStartTime();
  if (scoreA != scoreB)
    return scoreA > scoreB;
  if (a->GetNonzeroNum() != b->GetNonzeroNum())
    return a->GetNonzeroNum() > b->GetNonzeroNum();
  else if (a->GetVarNum() != b->GetVarNum())
    return a->GetVarNum() > b->GetVarNum();
  else if (a->GetConNum() != b->GetConNum())
    return a->GetConNum() > b->GetConNum();
  else
    return a->GetNodeID() < b->GetNodeID();
}

void MIPTree::CheckSol() const { rootNode_->CheckSol(); }

bool MIPTree::IsEnd() { return rootNode_->IsEnd(); }

bool MIPTree::IsOptimal() { return rootNode_->IsOptimal(); }

bool MIPTree::IsInfeasible() { return rootNode_->IsInfeasible(); }

bool MIPTree::IsUnknown() { return rootNode_->IsUnknown(); }

bool MIPTree::IsFeasible() { return rootNode_->IsFeasible() || rootNode_->IsOptimal() || MIPNode::TreeBestNode_ != nullptr; }

double MIPTree::GetBestObj() { return MIPNode::TreeBestObj_; }

// void MIPTree::FillWaitingNodes()
// {
//   this_thread::sleep_for(chrono::seconds(10));
//   while (!scheduler_->IsRootWorkerDone() &&
//          ElapsedTime() + 10 < OPT(cutoff) &&
//          !IsEnd())
//     if (informWorkerNum_ > 0 ||
//         waitingNodes_.size() > OPT(Full))
//       this_thread::sleep_for(chrono::seconds(1));
//     else
//       GenerateNewNodes();
// }

// void MIPTree::GenerateNewNodes()
// {
//   boost::mutex::scoped_lock lock(mutexTree_);
//   if (IsEnd() || scheduler_->IsRootWorkerDone() ||
//       OPT(cutoff) < ElapsedTime() + 10)
//     return;
//   while (waitingNodes_.size() < OPT(Full) &&
//          !runningNodes_.empty())
//   {
//     if (informWorkerNum_ > 0)
//       break;
//     MIPNode *selectedNode = SelectRunningNodeToBranch();
//     vector<MIPNode *> newNodes = selectedNode->SelectVarToBrach();
//     selectedNode->UpdateVarMsg();
//     ActivateNodes(newNodes);
//     for (MIPNode *node : newNodes)
//     {
//       node->Activate();
//       InsertWaitingNodes(node);
//     }
//     DEBUG_PRINT("c %10.2lf    [%-10s]    Remaining %ld nodes in waiting\n",
//                 ElapsedTime(), "Produce", waitingNodes_.size());
//   }
// }
