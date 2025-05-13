#pragma once
#include "../utils/header.h"
#include "../utils/paras.h"
#include "../Worker/Worker.h"
#include "../MIPTree/MIPTree.h"
class Worker;
class RootWorker;
class GeneralWorker;
class MIPTree;
class MIPNode;
class Presolve;

class Scheduler
{
public:
  Scheduler();
  ~Scheduler();
  void Optimize();
  const bool HaveIncumbent();
  const double GetIncumbent() const;
  const double GetFinalBestObj() const;
  void NodeResult(
      MIPNode *_node, const HighsModelStatus &_status,
      const bool &_haveIncumbent, const HighsSolution &_solution,
      const size_t &_tid, const double &_obj) const;
  bool GetNodeToRun(
      const size_t &_tid) const;
  const size_t GetIdleWorkerNum() const;
  const bool IsRootWorkerDone() const;
  inline const HighsModel &GetRootModel() const { return highs_.getModel(); }

private:
  mutable boost::mutex mutexTree_;
  MIPTree *mipTree_;
  string logPath_;
  double cutoff_;
  size_t threadNum_;
  atomic<bool> terminated_;
  vector<Worker *> workerSet_;
  RootWorker *rootWorker_;
  atomic<bool> haveIncumbent_;
  Highs highs_;

  void InitWorkerSet();
  void InitModel();
  void Solve();
  void TerminateWorker();
  void PrintResult();
  void SimpleResult(); 
};