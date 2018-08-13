#include "helpers.h"
#include <string>
//#include "Eigen-3.3/Eigen/Core"
//#include "Eigen-3.3/Eigen/QR"


class PathPlanner
{
public:
    void processMessage(std::string msg);
private:
    int state;
};