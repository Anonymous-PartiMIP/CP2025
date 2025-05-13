#include "Worker.h"

Worker::Worker(int _tid, Scheduler *_scheduler)
    : tid_(_tid), scheduler_(_scheduler),
      workerStatus_(WorkerStatus::Idle),
      scip_(nullptr),
      numVars_(0),
      numCons_(0),
      scipSolveStarted_(false)
{
}

void Worker::Terminate()
{
  if (workerStatus_ == WorkerStatus::Busy && scipSolveStarted_ && scip_ != nullptr)
    SCIP_CALL_ABORT(SCIPinterruptSolve(scip_));
  scipSolveStarted_ = false;
}

Worker::~Worker()
{
  ReleaseSCIP();
}

void Worker::ReleaseSCIP()
{
  for (auto &scipCons : scipCons_)
  {
    if (scipCons != nullptr)
    {
      SCIP_CALL_ABORT(SCIPreleaseCons(scip_, &scipCons));
      scipCons = nullptr;
    }
  }

  for (auto &scipVar : scipVars_)
  {
    if (scipVar != nullptr)
    {
      SCIP_CALL_ABORT(SCIPreleaseVar(scip_, &scipVar));
      scipVar = nullptr;
    }
  }
  if (scip_ != nullptr)
    SCIP_CALL_ABORT(SCIPfree(&scip_));
  scip_ = nullptr;
}

void Worker::SetPrecision()
{
  if (OPT(defualtPrecision) == 0)
  {
    SCIP_CALL_ABORT(SCIPsetRealParam(scip_, "limits/gap", OPT(MIPGap)));
  }
  else
  {
    SCIP_CALL_ABORT(SCIPsetRealParam(scip_, "limits/gap", 0.0));
  }
}

void RootWorker::HighsToSCIP(const HighsModel &_highsmodel)
{
  highs_.setOptionValue("log_to_console", "false");
  highs_.passModel(_highsmodel);
  highs_.setMatrixFormat(MatrixFormat::kRowwise);
  SCIP_CALL_ABORT(SCIPcreate(&scip_));
  SCIP_CALL_ABORT(SCIPincludeDefaultPlugins(scip_));
  SCIP_CALL_ABORT(SCIPsetIntParam(scip_, "parallel/maxnthreads", 1));
  SCIP_CALL_ABORT(SCIPsetIntParam(scip_, "display/verblevel", 0));
  SCIP_CALL_ABORT(SCIPcreateProbBasic(
      scip_, ("SCIP_worker" + to_string(tid_)).c_str()));

  const HighsLp &lp = highs_.getLp();
  numVars_ = lp.num_col_;
  numCons_ = lp.num_row_;

  const vector<double> &obj = lp.col_cost_;
  const vector<double> &lb = lp.col_lower_;
  const vector<double> &ub = lp.col_upper_;
  const vector<HighsVarType> &varTypes = lp.integrality_;

  const vector<double> &rowLb = lp.row_lower_;
  const vector<double> &rowUb = lp.row_upper_;
  const vector<int> &startIndices = lp.a_matrix_.start_;
  const vector<int> &indices = lp.a_matrix_.index_;
  const vector<double> &values = lp.a_matrix_.value_;

  scipVars_.resize(numVars_, nullptr);
  scipCons_.resize(numCons_, nullptr);

  SCIP_VARTYPE defaultVarType = SCIP_VARTYPE_CONTINUOUS;
  bool hasIntegerVars = (varTypes.size() == numVars_);
  for (size_t i = 0; i < numVars_; ++i)
  {
    SCIP_VAR *var = nullptr;
    SCIP_VARTYPE scipVarType = defaultVarType;

    if (hasIntegerVars)
    {
      scipVarType = (varTypes[i] == HighsVarType::kInteger)
                        ? SCIP_VARTYPE_INTEGER
                        : SCIP_VARTYPE_CONTINUOUS;
    }

    SCIP_CALL_ABORT(SCIPcreateVarBasic(
        scip_,
        &var,
        lp.col_names_[i].c_str(),
        lb[i],
        ub[i],
        obj[i],
        scipVarType));

    SCIP_CALL_ABORT(SCIPaddVar(scip_, var));
    scipVars_[i] = var;
  }

  for (size_t i = 0; i < numCons_; ++i)
  {
    size_t startIdx = startIndices[i];
    size_t endIdx = (i == numCons_ - 1) ? indices.size() : startIndices[i + 1];

    vector<SCIP_VAR *> consVars;
    vector<double> consCoeffs;
    consVars.reserve(endIdx - startIdx);
    consCoeffs.reserve(endIdx - startIdx);

    for (size_t j = startIdx; j < endIdx; ++j)
    {
      consVars.emplace_back(scipVars_[indices[j]]);
      consCoeffs.emplace_back(values[j]);
    }

    SCIP_CONS *cons = nullptr;
    SCIP_CALL_ABORT(SCIPcreateConsBasicLinear(
        scip_,
        &cons,
        lp.row_names_[i].c_str(),
        consVars.size(),
        consVars.data(),
        consCoeffs.data(),
        rowLb[i],
        rowUb[i]));

    SCIP_CALL_ABORT(SCIPaddCons(scip_, cons));
    scipCons_[i] = cons;
  }

  if (lp.sense_ == ObjSense::kMinimize)
    SCIP_CALL_ABORT(SCIPsetObjsense(scip_, SCIP_OBJSENSE_MINIMIZE));
  else if (lp.sense_ == ObjSense::kMaximize)
    SCIP_CALL_ABORT(SCIPsetObjsense(scip_, SCIP_OBJSENSE_MAXIMIZE));

  SCIP_CALL_ABORT(SCIPaddOrigObjoffset(scip_, lp.offset_));
}

void GeneralWorker::HighsToSCIP(const Highs &_highs)
{
  SCIP_CALL_ABORT(SCIPcreate(&scip_));
  SCIP_CALL_ABORT(SCIPincludeDefaultPlugins(scip_));
  SCIP_CALL_ABORT(SCIPsetIntParam(scip_, "parallel/maxnthreads", 1));
  SCIP_CALL_ABORT(SCIPsetIntParam(scip_, "display/verblevel", 0));
  SCIP_CALL_ABORT(SCIPcreateProbBasic(
      scip_, ("SCIP_worker" + to_string(tid_)).c_str()));

  numVars_ = _highs.getNumCol();
  numCons_ = _highs.getNumRow();
  HighsInt num_nz = _highs.getNumNz();

  scipVars_.resize(numVars_, nullptr);
  scipCons_.resize(numCons_, nullptr);

  const vector<double> &colCost = _highs.getLp().col_cost_;
  const vector<double> &colLower = _highs.getLp().col_lower_;
  const vector<double> &colUpper = _highs.getLp().col_upper_;

  for (size_t i = 0; i < numVars_; ++i)
  {
    SCIP_VAR *var = nullptr;
    SCIP_VARTYPE scipVarType = SCIP_VARTYPE_CONTINUOUS;
    HighsVarType integrality;
    _highs.getColIntegrality(i, integrality);
    if (integrality == HighsVarType::kInteger)
      scipVarType = SCIP_VARTYPE_INTEGER;

    SCIP_CALL_ABORT(SCIPcreateVarBasic(
        scip_,
        &var,
        "",
        colLower[i],
        colUpper[i],
        colCost[i],
        scipVarType));

    SCIP_CALL_ABORT(SCIPaddVar(scip_, var));
    scipVars_[i] = var;
  }

  vector<double> rowLower(numCons_);
  vector<double> rowUpper(numCons_);
  vector<HighsInt> rowStart(numCons_);
  vector<HighsInt> rowIndex(num_nz);
  vector<double> rowValue(num_nz);

  HighsInt numRow;
  _highs.getRows(0, numCons_ - 1, numRow, rowLower.data(), rowUpper.data(), 
                 num_nz, rowStart.data(), rowIndex.data(), rowValue.data());

  vector<double> consCoeffs;
  vector<SCIP_VAR *> consVars;
  consVars.reserve(numVars_);
  consCoeffs.reserve(numVars_);
  for (size_t i = 0; i < numCons_; ++i)
  {
    size_t startIdx = rowStart[i];
    size_t endIdx = (i == numCons_ - 1) ? rowIndex.size() : rowStart[i + 1];
    assert(endIdx <= rowValue.size());

    consVars.resize(endIdx - startIdx);
    consCoeffs.resize(endIdx - startIdx);

    size_t idx = 0;
    for (size_t j = startIdx; j < endIdx; ++j, ++idx)
    {
      consVars[idx] = scipVars_[rowIndex[j]];
      consCoeffs[idx] = rowValue[j];
      assert(j < rowValue.size());
    }
    SCIP_CONS *cons = nullptr;
    SCIP_CALL_ABORT(SCIPcreateConsBasicLinear(
        scip_,
        &cons,
        "",
        consVars.size(),
        consVars.data(),
        consCoeffs.data(),
        rowLower[i],
        rowUpper[i]));

    SCIP_CALL_ABORT(SCIPaddCons(scip_, cons));
    scipCons_[i] = cons;
  }

  ObjSense sense;
  _highs.getObjectiveSense(sense);
  if (sense == ObjSense::kMinimize)
    SCIP_CALL_ABORT(SCIPsetObjsense(scip_, SCIP_OBJSENSE_MINIMIZE));
  else if (sense == ObjSense::kMaximize)
    SCIP_CALL_ABORT(SCIPsetObjsense(scip_, SCIP_OBJSENSE_MAXIMIZE));

  double offset;
  _highs.getObjectiveOffset(offset);
  SCIP_CALL_ABORT(SCIPaddOrigObjoffset(scip_, offset));
}
