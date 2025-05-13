#pragma once
#include "../utils/header.h"
#include "../utils/paras.h"
#include "../Presolve/Presolve.h"
#include "MIPTree.h"
#include "../Worker/Worker.h"
class MIPTree;
class GeneralWorker;

class MIPNode
{
public:
  MIPNode(
      const size_t _depth, MIPNode *_parent,
      const HighsModel &_oriModel,
      const double _objCutValue);
  ~MIPNode();

  void Activate();
  void ReducedModel();
  inline size_t GetNodeID() const { return nodeID_; }
  const HighsModel &GetModelToSolve() const { return presolve_.GetReducedModel(); }
  const Highs &GetHighsToSolve() const { return highs_; }
  void ReturnSolution(const HighsSolution &_reducedSolution) { presolve_.SetReducedSolution(_reducedSolution); }
  NodeStatus GetNodeStatus() const { return nodeStatus_; }
  ProblemStatus GetProblemStatus() const { return problemStatus_; }
  inline size_t GetDepth() const { return depth_; }
  inline size_t GetVarNum() const { return varNum_; }
  inline size_t GetConNum() const { return conNum_; }
  inline size_t GetNonzeroNum() const { return nonzeroNum_; }
  inline double GetObj() const { return Obj_; }
  bool IsRoot() const { return depth_ == 1; }
  bool IsEnd() const { return nodeStatus_ == NodeStatus::End; }
  bool IsOptimal() const { return problemStatus_ == ProblemStatus::Optimal; }
  bool IsInfeasible() const { return problemStatus_ == ProblemStatus::Infeasible; }
  bool IsFeasible() const { return problemStatus_ == ProblemStatus::Feasible; }
  bool IsUnknown() const { return problemStatus_ == ProblemStatus::Unknown; }
  void DealOptimal(const HighsSolution &_reducedSolution);
  void DealInfeasible();
  void DealFeasible(const HighsSolution &_reducedSolution);
  void DealEndFeasible(const HighsSolution &_reducedSolution, const double _obj);
  void DealUnknown();
  void RealseModel();
  inline double GetObjCutValue() const { return objCutValue_; }
  void SetProblemStatus(ProblemStatus _problemStatus) { problemStatus_ = _problemStatus; }
  void SetNodeStatus(NodeStatus _nodeStatus) { nodeStatus_ = _nodeStatus; }
  void SetWorker(GeneralWorker *_worker) { worker_ = _worker; }
  vector<MIPNode *> SelectVarToBrach();
  void SetRunningStartTime() { runningStartTime_ = ElapsedTime(); }
  inline double GetRunningStartTime() const { return runningStartTime_; }
  void SolPropagation();
  void IncreaseVarBranchInSolved();
  void UpdateVarMsg();
  void CheckSol() const { presolve_.CheckSol(); }
  inline const MIPNode *GetParent() const { return parentNode_; }
  static MIPTree *Tree_;
  static double TreeBestObj_;
  static MIPNode *TreeBestNode_;

private:
  static size_t NODEID_;
  // static size_t BranchTimes_;
  static map<string, size_t> VarBranchInSolved_;
  // static map<string, size_t> VarTabu_;
  static boost::mutex mutexNODEID__;
  // static boost::mutex mutexBranchTimes_;
  inline static double GetMid(const double &_lb, const double &_ub)
  {
    if (_lb < 0 && 0 < _ub)
      return 0;
    else if (-INF < _lb && _ub < INF)
      return _lb / 2 + _ub / 2;
    else if (-INF < _lb)
      return _lb + 10000;
    else if (_ub < INF)
      return _ub - 10000;
    return 0;
  };
  // static void UpdateTabu(const string &_branchVarName);

  string branchVarName_;
  GeneralWorker *worker_;
  Presolve presolve_;
  double objCutValue_;
  size_t depth_;
  size_t nodeID_;
  ProblemStatus problemStatus_;
  NodeStatus nodeStatus_;
  MIPNode *parentNode_;
  MIPNode *leftNode_;
  MIPNode *rightNode_;

  size_t varNum_;
  size_t conNum_;
  size_t nonzeroNum_;
  Highs highs_;
  vector<size_t> shortDegree_;
  vector<size_t> varDegree_;
  vector<char> varType_;
  double Obj_;
  double runningStartTime_;

  void InitModel();
  bool CheckPresolveInfeas();
  bool CheckPresolveOptimal();
  void UpPropagation();
  void DownPropagation();
  void Postsolve();
  void Postsolve(const HighsSolution &_reducedSolution);
  void ObjCut();

  /* Branch*/
  size_t bestIndex_;
  size_t bestShortDegree_;
  size_t bestVarDegree_;
  char bestType_;
  size_t bestVarBranchInSolved_;
  vector<size_t> candidateVars_;
  inline bool Branch(
      const size_t &_varIndex,
      const size_t &_shortDegree,
      const size_t &_varDegree,
      const size_t &_varBranchInSolved,
      const char &_varType)
  {
    bool betterCandidate = false;
    if (_varBranchInSolved > bestVarBranchInSolved_)
      betterCandidate = true;
    else if (_varBranchInSolved == bestVarBranchInSolved_)
    {
      if (_shortDegree > bestShortDegree_ ||
          (_shortDegree == bestShortDegree_ && _varDegree > bestVarDegree_))
        betterCandidate = true;
      else if (_varDegree == bestVarDegree_ &&
               _shortDegree == bestShortDegree_)
        candidateVars_.push_back(_varIndex);
    }
    return betterCandidate;
  }
};
