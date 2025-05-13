#include "Worker.h"

std::atomic<double> incumbentSubNode(INF);

HighsCallbackFunctionType userSubNodeCallback =
    [](int callback_type, const std::string &message,
       const HighsCallbackDataOut *data_out, HighsCallbackDataIn *data_in,
       void *user_callback_data)
{
  CallbackData *data = static_cast<CallbackData *>(user_callback_data);
  if (callback_type == kCallbackMipInterrupt ||
      callback_type == kCallbackSimplexInterrupt ||
      callback_type == kCallbackIpmInterrupt)
    data_in->user_interrupt = data->STOP.load();
  else if (callback_type == kCallbackMipImprovingSolution)
    if (data_out->objective_function_value < incumbentSubNode.load())
      incumbentSubNode.store(data_out->objective_function_value);
  if (OPT(cutoff) + 10 < ElapsedTime())
    data_in->user_interrupt = true;
};

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
        highs_.passModel(node_->GetModelToSolve());
        ObjCut();
        SetPrecision();
        SetCallback();
        SetParameter();
      }
      if (!endRuning)
        highs_.run();
      Idle();
      DealResult();
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
    const double incunmbentValue = scheduler_->GetIncumbent();
    if (incunmbentValue < node_->GetObjCutValue())
    {
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
      DEBUG_PRINT("c %10.2lf    [%-10s]    Worker(%3ld) [%lf]\n",
                  ElapsedTime(), "Obj Cut", tid_, incunmbentValue);
      delete[] indices;
      delete[] values;
    }
  }
}

GeneralWorker::GeneralWorker(int _tid, Scheduler *_scheduler)
    : Worker(_tid, _scheduler),
      node_(nullptr),
      terminated_(false),
      endRuning(false)
{
  logPath_ = OPT(logPath) + to_string(tid_) + "_thread.log";
  // log_ = ofstream(logPath_);
  highs_.setOptionValue("log_to_console", "false");
  // highs_.setOptionValue("log_file", logPath_.c_str());
}

void GeneralWorker::SetCallback()
{
  callbackData_->STOP.store(false);
  highs_.setCallback(userSubNodeCallback, callbackData_);
  highs_.startCallback(kCallbackMipInterrupt);
  highs_.startCallback(kCallbackSimplexInterrupt);
  highs_.startCallback(kCallbackIpmInterrupt);
  highs_.startCallback(kCallbackMipImprovingSolution);
}

void GeneralWorker::SetParameter()
{
  highs_.setOptionValue("threads", 1);
  highs_.setOptionValue("time_limit", max(OPT(cutoff) - ElapsedTime(), (double)1));
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
  if (workerStatus_ == WorkerStatus::Busy)
    callbackData_->STOP.store(true);
  endRuning = true;
}

void GeneralWorker::DealResult()
{
  scheduler_->NodeResult(
      node_, highs_.getModelStatus(), IsFeasible(), highs_.getSolution(), highs_.getObjectiveValue(), tid_);
}

void GeneralWorker::Terminate()
{
  if (workerStatus_ == WorkerStatus::Busy)
    callbackData_->STOP.store(true);
  terminated_ = true;
}