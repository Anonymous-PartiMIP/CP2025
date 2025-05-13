#include "Worker.h"
atomic<double> incumbentRoot(INF);

SCIP_DECL_EVENTEXEC(eventExecCallback)
{
  SCIP_SOL *bestsol = SCIPgetBestSol(scip);
  if (bestsol != nullptr)
  {
    double a = SCIPgetSolOrigObj(scip, bestsol);
    if (a < incumbentRoot.load())
      incumbentRoot.store(a);
  }
  return SCIP_OKAY;
}

void RootWorker::Run()
{
  HighsToSCIP(scheduler_->GetRootModel());
  SCIP_CALL_ABORT(SCIPsetRealParam(scip_, "limits/time", OPT(cutoff) - ElapsedTime()));
  SetPrecision();
  SCIP_EVENTHDLR *eventHandler = nullptr;
  SCIP_CALL_ABORT(
      SCIPincludeEventhdlrBasic(
          scip_, &eventHandler, "BestSolutionHandler",
          "Handles updates to the best solution found", eventExecCallback, nullptr));
  SCIP_CALL_ABORT(SCIPtransformProb(scip_));
  SCIP_CALL_ABORT(SCIPcatchEvent(scip_, SCIP_EVENTTYPE_BESTSOLFOUND, eventHandler, nullptr, nullptr));
  if (!scipSolveStarted_)
  {
    scipSolveStarted_ = true;
    SCIP_CALL_ABORT(SCIPsolve(scip_));
    scipSolveStarted_ = false;
  }
  solveTime_ = ElapsedTime();
  workerStatus_ = WorkerStatus::Idle;
  printf("c %10.2lf    [%-10s]    RootWorker %ld\n",
         ElapsedTime(), "End", tid_);
}

RootWorker::RootWorker(int _tid, Scheduler *_scheduler)
    : Worker(_tid, _scheduler), solveTime_(INF)
{
  workerStatus_ = WorkerStatus::Busy;
  highs_.setOptionValue("log_to_console", "false");
}

bool RootWorker::IsUnknown()
{
  return !IsFeasible() && !IsInfeasible();
}

bool RootWorker::IsInfeasible()
{
  SCIP_STATUS status = SCIPgetStatus(scip_);
  return status == SCIP_STATUS_INFEASIBLE;
}

bool RootWorker::IsEnd()
{
  return IsOptimal() || IsInfeasible();
}

bool RootWorker::IsOptimal()
{
  SCIP_STATUS status = SCIPgetStatus(scip_);
  return status == SCIP_STATUS_OPTIMAL;
}

double RootWorker::GetFinalBestObj()
{
  assert(IsFeasible());
  SCIP_SOL *sol = SCIPgetBestSol(scip_);
  double objValue = SCIPgetSolOrigObj(scip_, sol);
  return objValue;
}

double RootWorker::GetIncumbent()
{
  return incumbentRoot.load();
}

bool RootWorker::HaveIncumbent()
{
  return incumbentRoot.load() < INF;
}

void RootWorker::PrintResult()
{
  cout << "c Root Worker: ";
  SCIP_STATUS status = SCIPgetStatus(scip_);
  cout << "Solver status of Root Worker: ";
  switch (status)
  {
  case SCIP_STATUS_OPTIMAL:
    cout << "Optimal solution found." << endl;
    break;
  case SCIP_STATUS_INFEASIBLE:
    cout << "Problem is infeasible." << endl;
    break;
  case SCIP_STATUS_UNBOUNDED:
    cout << "Problem is unbounded." << endl;
    break;
  case SCIP_STATUS_INFORUNBD:
    cout << "Problem is infeasible or unbounded." << endl;
    break;
  case SCIP_STATUS_TIMELIMIT:
    cout << "Time limit reached." << endl;
    break;
  case SCIP_STATUS_NODELIMIT:
    cout << "Node limit reached." << endl;
    break;
  case SCIP_STATUS_MEMLIMIT:
    cout << "Memory limit reached." << endl;
    break;
  case SCIP_STATUS_USERINTERRUPT:
    cout << "Solver interrupted by user." << endl;
    break;
  default:
    cout << "Other status." << endl;
  }

  if (IsFeasible())
    printf("c Gap: %lf; %lf\n", SCIPgetGap(scip_) * 100, SCIPgetPrimalbound(scip_));
  if (IsEnd())
  {
    printf("c %10.2lf    ", ElapsedTime());
    if (IsOptimal())
      cout << "Optimal; " << SCIPgetPrimalbound(scip_) << endl;
    else if (IsInfeasible())
      cout << "Infeasible" << endl;
  }
}
