#pragma once
#include "../utils/header.h"
#include "../utils/paras.h"
#include "../Presolve/Presolve.h"
#include "MIPNode.h"
#include "../Scheduler/Scheduler.h"
class MIPNode;
class Scheduler;

struct NodeSelection_Waiting
{
  bool operator()(const MIPNode *a, const MIPNode *b) const;
};

struct NodeSelection_Running
{
  bool operator()(const MIPNode *a, const MIPNode *b) const;
};

class MIPTree
{
public:
  MIPTree();
  ~MIPTree();
  void BuildInitNodes();
  bool IsEnd();
  bool IsOptimal();
  bool IsInfeasible();
  bool IsFeasible();
  bool IsUnknown();
  const char *GetStatus();
  inline bool GetInitDone() { return initDone_; }
  double GetBestObj();
  inline double GetSolveTime() { return solveTime_; }
  void SetNodeStatus(MIPNode *_node, NodeStatus _nodeStatus);
  vector<MIPNode *> GetInitNodesToRun();
  MIPNode *GetNodeToRun();
  void InformNodeResult(
      MIPNode *_node, const HighsModelStatus &_status,
      bool _haveIncumbent, const HighsSolution &_reducedSolution, double _obj);
  const bool HaveGlobalIncumbent();
  const double GetGlobalIncumbent();
  inline const size_t GetInformWorkerNum() { return informWorkerNum_; }
  inline void IncreaseInformWorkerNum()
  {
    boost::mutex::scoped_lock lock(mutexInformWorkerNum_);
    informWorkerNum_++;
  }
  inline void DecreaseInformWorkerNum()
  {
    boost::mutex::scoped_lock lock(mutexInformWorkerNum_);
    informWorkerNum_--;
  }
  // void FillWaitingNodes();
  void CheckSol() const;
  Scheduler *scheduler_;
  static void ActivateNodes(const vector<MIPNode *> &_newNodes);
  static void InsertTempNewNodes(const vector<MIPNode *> &_newNodes);
  static void PartitionNodes(const vector<MIPNode *> &_selectNodes);

private:
  int coreNum_;
  bool initDone_;
  atomic<size_t> informWorkerNum_;
  mutable boost::mutex mutexInformWorkerNum_;
  mutable boost::mutex mutexTree_;
  MIPNode *rootNode_;
  set<MIPNode *, NodeSelection_Waiting> waitingNodes_;
  unordered_set<MIPNode *> branchedWaitingNodes_;
  set<MIPNode *, NodeSelection_Running> runningNodes_;
  unordered_set<MIPNode *> branchedRunningNodes_;
  unordered_set<MIPNode *> endNodes_;
  double solveTime_;

  static vector<MIPNode *> tempNewNodes_;
  static boost::mutex mutexTempNewNodes_;

  void BuildRootNode();
  void InsertWaitingNodes(MIPNode *_mipNode);
  MIPNode *SelectWaitingNodeToBranch();
  MIPNode *SelectRunningNodeToBranch();
  // void GenerateNewNodes();
};
