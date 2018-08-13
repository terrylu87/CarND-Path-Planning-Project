#include "helpers.h"
#include <vector>
#include <string>
//#include "Eigen-3.3/Eigen/Core"
//#include "Eigen-3.3/Eigen/QR"


class PathPlanner
{
public:
    void processMessage(std::string msg);
    void setMap(std::vector<double> map_waypoints_x,
                std::vector<double> map_waypoints_y,
                std::vector<double> map_waypoints_s,
                std::vector<double> map_waypoints_dx,
                std::vector<double> map_waypoints_dy);
    std::vector<double> next_x(){return _next_x_vals;}
    std::vector<double> next_y(){return _next_y_vals;}
private:
    int _state;
    std::vector<double> _map_waypoints_x;
    std::vector<double> _map_waypoints_y;
    std::vector<double> _map_waypoints_s;
    std::vector<double> _map_waypoints_dx;
    std::vector<double> _map_waypoints_dy;
    std::vector<double> _next_x_vals;
    std::vector<double> _next_y_vals;
};