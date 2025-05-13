#include "Worker.h"

atomic<double> incumbentSubNode(INF);

SCIP_DECL_EVENTEXEC(eventExecCallback_2)
{
  SCIP_SOL *bestsol = SCIPgetBestSol(scip);
  if (bestsol != nullptr)
    incumbentSubNode.store(min(SCIPgetSolOrigObj(scip, bestsol), incumbentSubNode.load()));
  return SCIP_OKAY;
}

void GeneralWorker::Run()
{
  phase2_.store(false);
  while (!terminated_)
  {
    usleep(100000);
    if (phase2_.load())
    {
      while (!scheduler_->GetNodeToRun(tid_))
        usleep(100000);
      phase2_.store(false);
    }
    if (workerStatus_ == WorkerStatus::Busy)
    {
      if (!endRuning)
      {
        HighsToSCIP(node_->GetHighsToSolve());
        ObjCut();
        SetPrecision();
        SetParameter();
        SetCallback();
      }
      if (!scipSolveStarted_ && !endRuning)
      {
        scipSolveStarted_ = true;
        SCIP_CALL_ABORT(SCIPsolve(scip_));
        scipSolveStarted_ = false;
      }
      Idle();
      DealResult();
      ReleaseSCIP();
      while (!scheduler_->GetNodeToRun(tid_))
        usleep(100000);
      endRuning = false;
    }
  }
  DEBUG_PRINT("c %10.2lf    [%-10s]    GeneralWorker(%ld)\n",
              ElapsedTime(), "End", tid_);
}

void GeneralWorker::ObjCut()
{
  if (scheduler_->HaveIncumbent())
  {
    const double incumbentValue = scheduler_->GetIncumbent();
    if (incumbentValue < node_->GetObjCutValue())
    {
      SCIP_CONS *objCutCons = nullptr;
      vector<double> col_cost_copy(node_->GetHighsToSolve().getLp().col_cost_);
      SCIP_CALL_ABORT(SCIPcreateConsBasicLinear(
          scip_,
          &objCutCons,
          "obj_cut",
          numVars_,
          scipVars_.data(),
          col_cost_copy.data(),
          -SCIPinfinity(scip_),
          incumbentValue - node_->GetHighsToSolve().getLp().offset_));
      SCIP_CALL_ABORT(SCIPaddCons(scip_, objCutCons));
      scipCons_.push_back(objCutCons);
      DEBUG_PRINT("c %10.2lf    [%-10s]    Worker(%3ld) [%lf]\n",
                  ElapsedTime(), "Obj Cut", tid_, incumbentValue);
    }
  }
}

GeneralWorker::GeneralWorker(int _tid, Scheduler *_scheduler)
    : Worker(_tid, _scheduler),
      node_(nullptr),
      terminated_(false),
      endRuning(false)
{
}

void GeneralWorker::SetCallback()
{
  SCIP_EVENTHDLR *eventHandler = nullptr;
  SCIP_CALL_ABORT(
      SCIPincludeEventhdlrBasic(
          scip_, &eventHandler, "BestSolutionHandler_2",
          "Handles updates to the best solution found of general worker", eventExecCallback_2, nullptr));
  SCIP_CALL_ABORT(SCIPtransformProb(scip_));
  SCIP_CALL_ABORT(SCIPcatchEvent(scip_, SCIP_EVENTTYPE_BESTSOLFOUND, eventHandler, nullptr, nullptr));
}

void GeneralWorker::SetParameter()
{
  SCIP_CALL_ABORT(
      SCIPsetRealParam(scip_, "limits/time", max(OPT(cutoff) - ElapsedTime(), (double)1)));
}

void GeneralWorker::SetNode(MIPNode *_node)
{
  node_ = _node;
}

void GeneralWorker::Busy()
{
  workerStatus_ = WorkerStatus::Busy;
  DEBUG_PRINT("c %10.2lf    [%-10s]    Worker(%3ld) ---> Node(%ld)\n",
              ElapsedTime(), "Busy", tid_, node_->GetNodeID());
}

void GeneralWorker::Idle()
{
  workerStatus_ = WorkerStatus::Idle;
  DEBUG_PRINT("c %10.2lf    [%-10s]    Worker(%3ld)\n",
              ElapsedTime(), "Idle", tid_);
}

bool GeneralWorker::HaveIncumbent()
{
  return incumbentSubNode.load() < INF;
}

double GeneralWorker::GetIncumbent()
{
  return incumbentSubNode.load();
}

void GeneralWorker::EndRunning()
{
  if (workerStatus_ == WorkerStatus::Busy && scipSolveStarted_ && scip_ != nullptr)
    SCIP_CALL_ABORT(SCIPinterruptSolve(scip_));
  scipSolveStarted_ = false;
  endRuning = true;
}

void GeneralWorker::DealResult()
{
  HighsModelStatus highsStatus = HighsModelStatus::kUnknown;
  HighsSolution highsSolution;
  bool feasible = false;
  double obj = INF;
  if (scip_ != nullptr)
  {
    SCIP_STATUS scipStatus = SCIPgetStatus(scip_);
    feasible = IsFeasible();

    switch (scipStatus)
    {
    case SCIP_STATUS_OPTIMAL:
      highsStatus = HighsModelStatus::kOptimal;
      break;
    case SCIP_STATUS_INFEASIBLE:
      highsStatus = HighsModelStatus::kInfeasible;
      break;
    case SCIP_STATUS_INFORUNBD:
      highsStatus = HighsModelStatus::kUnboundedOrInfeasible;
      break;
    case SCIP_STATUS_UNBOUNDED:
      highsStatus = HighsModelStatus::kUnbounded;
      break;
    case SCIP_STATUS_TIMELIMIT:
      highsStatus = HighsModelStatus::kTimeLimit;
      break;
    case SCIP_STATUS_MEMLIMIT:
      highsStatus = HighsModelStatus::kMemoryLimit;
      break;
    case SCIP_STATUS_USERINTERRUPT:
      highsStatus = HighsModelStatus::kInterrupt;
      break;
    default:
      highsStatus = HighsModelStatus::kUnknown;
      break;
    }

    if (feasible)
    {
      double *solVals = new double[numVars_];
      SCIP_SOL *sol = SCIPgetBestSol(scip_);
      SCIPgetSolVals(scip_, sol, numVars_, scipVars_.data(), solVals);
      highsSolution.col_value.assign(solVals, solVals + numVars_);
      obj = SCIPgetSolOrigObj(scip_, sol);
      delete[] solVals;
    }
  }
  scheduler_->NodeResult(
      node_, highsStatus, feasible, highsSolution, tid_, obj);
}

void GeneralWorker::Terminate()
{
  if (workerStatus_ == WorkerStatus::Busy && scipSolveStarted_ && scip_ != nullptr)
    SCIP_CALL_ABORT(SCIPinterruptSolve(scip_));
  scipSolveStarted_ = false;
  terminated_ = true;
}