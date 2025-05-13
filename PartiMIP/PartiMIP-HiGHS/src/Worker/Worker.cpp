#include "Worker.h"

Worker::Worker(int _tid, Scheduler *_scheduler)
    : tid_(_tid), scheduler_(_scheduler),
      workerStatus_(WorkerStatus::Idle),
      callbackData_(new CallbackData())
{
  highs_.setOptionValue("log_to_console", "false");
}

void Worker::Terminate()
{
  if (workerStatus_ == WorkerStatus::Busy)
    callbackData_->STOP.store(true);
}

Worker::~Worker()
{
  delete callbackData_;
}

void Worker::SetPrecision()
{
  if (OPT(defualtPrecision) == 0)
  {
    highs_.setOptionValue("mip_abs_gap", OPT(AbsMIPGap));
    highs_.setOptionValue("mip_rel_gap", OPT(MIPGap));
  }
  else
  {
    highs_.setOptionValue("mip_abs_gap", 0.0);
    highs_.setOptionValue("mip_rel_gap", 0.0);
  }
}

bool Worker::IsFeasible()
{
  const HighsModelStatus &model_status = highs_.getModelStatus();
  const HighsInfo &info = highs_.getInfo();
  return model_status == HighsModelStatus::kOptimal ||
         info.primal_solution_status == SolutionStatus::kSolutionStatusFeasible;
}