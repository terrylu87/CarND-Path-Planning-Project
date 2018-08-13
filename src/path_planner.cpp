#include "path_planner.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

void PathPlanner::processMessage(string msg)
{
    auto j = json::parse(msg);
}