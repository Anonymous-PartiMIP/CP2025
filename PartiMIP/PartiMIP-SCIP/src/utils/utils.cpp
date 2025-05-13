#include "header.h"

void RecreateDirectory(const string &_path)
{
  if (filesystem::exists(_path))
    filesystem::remove_all(_path);
  filesystem::create_directory(_path);
}

const char *NodeStatusToString(const NodeStatus &_nodeStatus)
{
  switch (_nodeStatus)
  {
  case NodeStatus::New:
    return "New";
  case NodeStatus::Waiting:
    return "Waiting";
  case NodeStatus::Running:
    return "Running";
  case NodeStatus::BranchedWaiting:
    return "BranchedWaiting";
  case NodeStatus::BranchedRunning:
    return "BranchedRunning";
  case NodeStatus::End:
    return "End";
  default:
    return "Unknown";
  }
}

const char *ProblemStatusToString(const ProblemStatus &_problemStatus)
{
  switch (_problemStatus)
  {
  case ProblemStatus::Unknown:
    return "Unknown";
  case ProblemStatus::Optimal:
    return "Optimal";
  case ProblemStatus::Infeasible:
    return "Infeasible";
  case ProblemStatus::Feasible:
    return "Feasible";
  default:
    return "Unknown";
  }
}