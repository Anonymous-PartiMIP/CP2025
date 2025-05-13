#pragma once
#include "../utils/header.h"
#include "../utils/paras.h"

class Presolve
{
public:
  Presolve(const HighsModel &_oriModel, const bool _initDone);
  void PresolveByHighs();
  void PostSolveByHighs();
  void PostSolveByHighs(const HighsSolution &_reducedSolution);
  inline const HighsModel &GetReducedModel() const { return presolver_.getPresolvedModel(); }
  inline const HighsSolution &GetOriSolution() const { return presolver_.getSolution(); }
  void CheckSol() const;
  inline double GetOriObj() const { return oriObj_; }
  void SetReducedSolution(const HighsSolution &_reducedSolution) { reducedSolution_ = _reducedSolution; }
  bool CheckPresolveInfeas();
  bool CheckPresolveOptimal();
  ~Presolve() = default;

private:
  HighsSolution reducedSolution_;
  Highs presolver_;

  double oriObj_;
};