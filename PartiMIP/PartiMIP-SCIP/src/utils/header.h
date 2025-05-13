#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <unordered_set>
#include <set>
#include <map>
#include <random>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include <stdlib.h>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <atomic>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <thread>
#include "Highs.h"
#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip/event.h"

// #define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

// #define TABU

using namespace std;
constexpr double INF = std::numeric_limits<double>::infinity();
extern std::chrono::steady_clock::time_point timeStart;
enum WorkerStatus
{
  Idle,
  Busy
};

enum ProblemStatus
{
  Unknown,
  Optimal,
  Feasible,
  Infeasible
};

enum NodeStatus
{
  New,
  Waiting,
  Running,
  BranchedWaiting,
  BranchedRunning,
  End
};

inline const double ElapsedTime()
{
  return std::chrono::duration_cast<std::chrono::duration<double>>(
             std::chrono::steady_clock::now() - timeStart)
      .count();
}
void RecreateDirectory(const string &_path);
const char *NodeStatusToString(const NodeStatus &_nodeStatus);
const char *ProblemStatusToString(const ProblemStatus &_problemStatus);
