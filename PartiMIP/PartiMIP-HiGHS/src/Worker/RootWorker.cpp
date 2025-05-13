#include "Worker.h"
std::atomic<double> incumbentRoot(INF);

HighsCallbackFunctionType userInterruptCallback =
    [](int callback_type, const std::string &message,
       const HighsCallbackDataOut *data_out, HighsCallbackDataIn *data_in,
       void *user_callback_data)
{
  CallbackData *data = static_cast<CallbackData *>(user_callback_data);
  if (callback_type == kCallbackMipInterrupt ||
      callback_type == kCallbackSimplexInterrupt ||
      callback_type == kCallbackIpmInterrupt)
    data_in->user_interrupt = data->STOP.load();
  if (callback_type == kCallbackMipImprovingSolution)
    incumbentRoot.store(data_out->objective_function_value);
};

void RootWorker::Run()
{
  SetPrecision();
  workerStatus_ = WorkerStatus::Busy;
  highs_.setCallback(userInterruptCallback, callbackData_);
  highs_.startCallback(kCallbackMipInterrupt);
  highs_.startCallback(kCallbackSimplexInterrupt);
  highs_.startCallback(kCallbackIpmInterrupt);
  highs_.startCallback(kCallbackMipImprovingSolution);
  highs_.run();
  const HighsModelStatus &model_status = highs_.getModelStatus();
  const HighsInfo &info = highs_.getInfo();
  const HighsLp &lp = highs_.getLp();
  const bool has_values = info.primal_solution_status;
  const HighsSolution &solution = highs_.getSolution();
  // log_ << "c Model status: " << highs_.modelStatusToString(model_status) << endl;
  // if (IsFeasible())
  // {
  //   log_ << "c Objective function value: " << info.objective_function_value << endl;
  //   log_ << "c Primal  solution status: "
  //        << highs_.solutionStatusToString(info.primal_solution_status) << endl;
  //   if (has_values)
  //     for (size_t col = 0; col < lp.num_col_; col++)
  //     {
  //       if (solution.col_value[col] != 0.0)
  //         log_ << lp.col_names_[col] << " " << solution.col_value[col] << endl;
  //     }
  // }
  solveTime_ = ElapsedTime();
  workerStatus_ = WorkerStatus::Idle;
  printf("c %10.2lf    [%-10s]    RootWorker %ld\n",
         ElapsedTime(), "End", tid_);
}

RootWorker::RootWorker(int _tid, Scheduler *_scheduler)
    : Worker(_tid, _scheduler), solveTime_(INF)
{
  highs_.passModel(_scheduler->GetRootModel());
  logPath_ = OPT(logPath) + "0_root.log";
  // log_ = ofstream(logPath_);
  highs_.setOptionValue("log_to_console", "false");
  // highs_.setOptionValue("log_file", logPath_.c_str());
  highs_.setOptionValue("time_limit", OPT(cutoff) - ElapsedTime());
  highs_.setOptionValue("threads", 1);
}

bool RootWorker::IsUnknown()
{
  return !IsFeasible() && !IsInfeasible();
}

bool RootWorker::IsInfeasible()
{
  const HighsModelStatus &model_status = highs_.getModelStatus();
  return model_status == HighsModelStatus::kInfeasible;
}

bool RootWorker::IsEnd()
{
  return IsOptimal() || IsInfeasible();
}

bool RootWorker::IsOptimal()
{
  const HighsModelStatus &model_status = highs_.getModelStatus();
  return model_status == HighsModelStatus::kOptimal;
}

double RootWorker::GetFinalBestObj()
{
  assert(IsFeasible());
  return highs_.getObjectiveValue();
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
  const HighsModelStatus &model_status = highs_.getModelStatus();
  cout << "c Root Worker: " << highs_.modelStatusToString(model_status) << endl;
  const HighsInfo &info = highs_.getInfo();
  if (IsFeasible())
    printf("c Gap: %lf; %lf\n", info.mip_gap, info.objective_function_value);
  if (IsEnd())
  {
    printf("c %10.2lf    ", ElapsedTime());
    if (IsOptimal())
      cout << highs_.modelStatusToString(model_status) << "; " << info.objective_function_value << endl;
    else
      cout << highs_.modelStatusToString(model_status) << endl;
  }
}
