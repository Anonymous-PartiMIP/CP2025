#include "Scheduler.h"

void *WorkerSolve(void *arg)
{
  Worker *worker = (Worker *)arg;
  worker->Run();
  return nullptr;
}

void *InitNodes(void *arg)
{
  MIPTree *mipTree = (MIPTree *)arg;
  mipTree->BuildInitNodes();
  return nullptr;
}

// void *FillWaitingNodes(void *arg)
// {
//   MIPTree *mipTree = (MIPTree *)arg;
//   mipTree->FillWaitingNodes();
//   return nullptr;
// }

void Scheduler::InitWorkerSet()
{
  rootWorker_ = new RootWorker(0, this);
  workerSet_.push_back(rootWorker_);
  for (size_t tid = 1; tid < threadNum_; ++tid)
  {
    GeneralWorker *generalWorker = new GeneralWorker(tid, this);
    workerSet_.push_back(generalWorker);
  }
}

void Scheduler::InitModel()
{
  highs_.readModel(OPT(instance));
  printf("c %10.2lf    [%-10s] [ Var: %d; Range: %d; NonZero: %d ]\n",
         ElapsedTime(), "Ori Model", highs_.getNumCol(), highs_.getNumRow(), highs_.getNumNz());
}

void Scheduler::TerminateWorker()
{
  for (size_t tid = 0; tid < threadNum_; ++tid)
    workerSet_[tid]->Terminate();
}

void Scheduler::Solve()
{
  printf("c -----------------solve start----------------------\n");
  vector<pthread_t> workerPtr(threadNum_);
  for (size_t tid = 0; tid < threadNum_; tid++)
    pthread_create(&workerPtr[tid], nullptr, WorkerSolve, workerSet_[tid]);
  pthread_t initNodesPtr;
  pthread_create(&initNodesPtr, nullptr, InitNodes, mipTree_);
  while (!mipTree_->GetInitDone())
  {
    this_thread::sleep_for(chrono::seconds(1));
    if (ElapsedTime() + 10 >= cutoff_)
    {
      terminated_ = true;
      TerminateWorker();
      break;
    }
  }
  // pthread_t fillWaitingNodesPtr;
  // bool startFill = false;
  if (mipTree_->GetInitDone() &&
      !rootWorker_->IsDone() &&
      !terminated_ &&
      ElapsedTime() + 10 < cutoff_)
  {
    vector<MIPNode *> initNodes = mipTree_->GetInitNodesToRun();
    if (!initNodes.empty())
    {
      for (size_t tid = 1; tid < threadNum_ && tid - 1 < initNodes.size(); ++tid)
      {
        GeneralWorker *generalWorker = dynamic_cast<GeneralWorker *>(workerSet_[tid]);
        generalWorker->SetNode(initNodes[tid - 1]);
        initNodes[tid - 1]->SetWorker(generalWorker);
        generalWorker->Busy();
      }
      for (size_t tid = initNodes.size() + 1; tid < threadNum_; ++tid)
      {
        GeneralWorker *generalWorker = dynamic_cast<GeneralWorker *>(workerSet_[tid]);
        generalWorker->SetPhase2();
      }
      // pthread_create(&fillWaitingNodesPtr, nullptr, FillWaitingNodes, mipTree_);
      // startFill = true;
    }
  }
  while (!terminated_)
  {
    this_thread::sleep_for(chrono::seconds(1));
    if (ElapsedTime() >= cutoff_ ||
        mipTree_->IsEnd() ||
        rootWorker_->IsDone())
    {
      terminated_ = true;
      TerminateWorker();
      break;
    }
  }
  printf("c -----------------ending solve----------------------\n");
  SimpleResult();
  for (size_t i = 0; i < threadNum_; i++)
    pthread_join(workerPtr[i], nullptr);
  printf("c -----------------ending join----------------------\n");
  PrintResult();
  pthread_join(initNodesPtr, nullptr);
  // if (startFill)
  //   pthread_join(fillWaitingNodesPtr, nullptr);
}

void Scheduler::SimpleResult()
{
  if (HaveIncumbent())
    printf("c GetIncumbent: %lf\n", GetIncumbent());
  if (rootWorker_->IsEnd() || mipTree_->IsEnd())
  {
    double solveTime = mipTree_->GetSolveTime() < rootWorker_->GetSolveTime()
                           ? mipTree_->GetSolveTime()
                           : rootWorker_->GetSolveTime();
    if (HaveIncumbent() && mipTree_->IsInfeasible())
      printf("o OPTIMAL\n");
    else if (rootWorker_->IsOptimal() || mipTree_->IsOptimal())
      printf("o OPTIMAL\n");
    else if (rootWorker_->IsInfeasible() || mipTree_->IsInfeasible())
      printf("o INFEASIBLE\n");
    printf("o Solve Time: %lf\n", solveTime);
  }
}

void Scheduler::PrintResult()
{
  if (MIPNode::TreeBestNode_ != nullptr)
    MIPNode::TreeBestNode_->SolPropagation();
  printf("c-----------------------result-----------------------\n");
  rootWorker_->PrintResult();
  printf("c-----------------------------------------------------\n");
  if (mipTree_->IsEnd())
    if (mipTree_->IsOptimal() ||
        HaveIncumbent() && mipTree_->IsFeasible())
      printf("c MIP Tree   : Optimal; %lf\n", mipTree_->GetBestObj());
    else
    {
      if (!mipTree_->IsInfeasible())
        printf("c MIP Tree   : %s\n", mipTree_->GetStatus());
      assert(mipTree_->IsInfeasible());
      printf("c MIP Tree   : Infeasible\n");
    }
  else if (mipTree_->IsFeasible())
    printf("c MIP Tree   : Feasible; %lf\n", mipTree_->GetBestObj());
  else
    printf("c MIP Tree   : Unknown\n");
  printf("c-----------------------------------------------------\n");
  if (rootWorker_->IsFeasible() || mipTree_->IsFeasible() || HaveIncumbent())
    printf("c Best Found Objective Value: %lf\n", GetFinalBestObj());
  if (rootWorker_->IsUnknown() && mipTree_->IsUnknown())
    printf("c UNKNOWN\n");
  if (rootWorker_->IsEnd() || mipTree_->IsEnd())
  {
    double solveTime = mipTree_->GetSolveTime() < rootWorker_->GetSolveTime()
                           ? mipTree_->GetSolveTime()
                           : rootWorker_->GetSolveTime();
    if (HaveIncumbent() && mipTree_->IsInfeasible())
      printf("c OPTIMAL\n");
    else if (rootWorker_->IsOptimal() || mipTree_->IsOptimal())
      printf("c OPTIMAL\n");
    else if (rootWorker_->IsInfeasible() || mipTree_->IsInfeasible())
      printf("c INFEASIBLE\n");
    printf("c Solve Time: %lf\n", solveTime);
  }
  if (mipTree_->IsFeasible())
    mipTree_->CheckSol();
  printf("c-----------------------------------------------------\n");
}

void Scheduler::Optimize()
{
  InitModel();
  InitWorkerSet();
  Solve();
}

const bool Scheduler::HaveIncumbent()
{
  if (haveIncumbent_)
    return true;
  haveIncumbent_ = mipTree_->IsFeasible() ||
                   rootWorker_->HaveIncumbent() ||
                   workerSet_[1]->HaveIncumbent();
  return haveIncumbent_;
}

const double Scheduler::GetIncumbent() const
{
  return min(mipTree_->GetBestObj(),
             min(rootWorker_->GetIncumbent(),
                 workerSet_[1]->GetIncumbent()));
}

const double Scheduler::GetFinalBestObj() const
{
  double res = INF;
  if (mipTree_->IsFeasible())
    res = mipTree_->GetBestObj();
  if (rootWorker_->HaveIncumbent() && rootWorker_->GetIncumbent() < res)
    res = rootWorker_->GetIncumbent();
  if (rootWorker_->IsFeasible() && rootWorker_->GetFinalBestObj() < res)
    res = rootWorker_->GetFinalBestObj();
  if (workerSet_[1]->HaveIncumbent() && workerSet_[1]->GetIncumbent() < res)
    res = workerSet_[1]->GetIncumbent();
  return res;
}

void Scheduler::NodeResult(
    MIPNode *_node, const HighsModelStatus &_status,
    const bool &_haveIncumbent, const HighsSolution &_solution,
    const size_t &_tid, const double &_obj) const
{
  mipTree_->IncreaseInformWorkerNum();
  boost::mutex::scoped_lock lock(mutexTree_);
  printf("c %10.2lf    [%-10s]    Worker(%3ld) ---> Node(%ld): %s\n",
         ElapsedTime(), "Result", _tid,
         _node->GetNodeID(), highs_.modelStatusToString(_status).c_str());
  mipTree_->InformNodeResult(_node, _status, _haveIncumbent, _solution, _obj);
  mipTree_->DecreaseInformWorkerNum();
}

bool Scheduler::GetNodeToRun(const size_t &_tid) const
{
  boost::mutex::scoped_lock lock(mutexTree_);
  if (mipTree_->GetInformWorkerNum() > 0)
    return false;
  GeneralWorker *generalWorker = dynamic_cast<GeneralWorker *>(workerSet_[_tid]);
  if (!mipTree_->IsEnd() && !IsRootWorkerDone() && cutoff_ > ElapsedTime() + 10)
  {
    MIPNode *node = mipTree_->GetNodeToRun();
    if (node != nullptr)
    {
      generalWorker->SetNode(node);
      node->SetWorker(generalWorker);
      generalWorker->Busy();
    }
  };
  return true;
}

const size_t Scheduler::GetIdleWorkerNum() const
{
  size_t res = 0;
  for (size_t tid = 1; tid < threadNum_; tid++)
  {
    GeneralWorker *generalWorker = dynamic_cast<GeneralWorker *>(workerSet_[tid]);
    if (generalWorker->IsIdle())
      res++;
  }
  return res;
}

const bool Scheduler::IsRootWorkerDone() const
{
  return rootWorker_->IsDone();
}

Scheduler::Scheduler()
    : logPath_(OPT(logPath) + "0_root.log"),
      cutoff_(OPT(cutoff)),
      threadNum_(OPT(threadNum)),
      mipTree_(new MIPTree()),
      terminated_(false),
      rootWorker_(nullptr),
      haveIncumbent_(false)
{
  highs_.setOptionValue("log_to_console", "false");
  // highs_.setOptionValue("log_file", logPath_.c_str());
  mipTree_->scheduler_ = this;
}
Scheduler::~Scheduler()
{
  for (size_t tid = 0; tid < workerSet_.size(); tid++)
    delete (workerSet_[tid]);
  delete mipTree_;
}
