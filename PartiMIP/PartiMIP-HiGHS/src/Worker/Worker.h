#pragma once
#include "../Scheduler/Scheduler.h"
#include "../utils/header.h"

class Scheduler;
class MIPNode;

struct CallbackData
{
  atomic<bool> STOP;
  CallbackData() : STOP(false) {}
};

class Worker
{
public:
  virtual void Run() = 0;
  virtual void Terminate();
  virtual bool HaveIncumbent() = 0;
  virtual double GetIncumbent() = 0;
  Worker(int _tid, Scheduler *_scheduler);
  size_t GetTid() { return tid_; }
  ~Worker();
  bool IsFeasible();

protected:
  void SetPrecision();
  CallbackData *callbackData_;
  string logPath_;
  ofstream log_;
  WorkerStatus workerStatus_;
  Highs highs_;
  size_t tid_;
  Scheduler *scheduler_;
};

class RootWorker : public Worker
{
private:
  double solveTime_;

public:
  void Run();
  RootWorker(int _tid, Scheduler *_scheduler);
  bool IsInfeasible();
  bool IsOptimal();
  bool IsUnknown();
  bool IsEnd();
  double GetFinalBestObj();
  bool HaveIncumbent();
  double GetIncumbent();
  double GetSolveTime() const { return solveTime_; }
  void PrintResult();
  inline bool IsDone() { return workerStatus_ == WorkerStatus::Idle; }
};

class GeneralWorker : public Worker
{
public:
  void Run();
  void Terminate();
  GeneralWorker(int _tid, Scheduler *_scheduler);
  void SetNode(MIPNode *_node);
  void EndRunning();
  void Busy();
  void Idle();
  bool HaveIncumbent();
  double GetIncumbent();
  inline bool IsIdle() { return workerStatus_ == WorkerStatus::Idle; }
  inline void SetPhase2() { phase2_.store(true); }
  
private:
  bool terminated_;
  MIPNode *node_;
  atomic<bool> endRuning;
  atomic<bool> phase2_;

  void DealResult();
  void ObjCut();
  void SetCallback();
  void SetParameter();
};