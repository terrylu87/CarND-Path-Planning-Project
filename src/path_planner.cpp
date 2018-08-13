#include "path_planner.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

void PathPlanner::setMap(vector<double> map_waypoints_x,
                         vector<double> map_waypoints_y,
                         vector<double> map_waypoints_s,
                         vector<double> map_waypoints_dx,
                         vector<double> map_waypoints_dy)
{
    vector<double> _map_waypoints_x = map_waypoints_x;
    vector<double> _map_waypoints_y = map_waypoints_y;
    vector<double> _map_waypoints_s = map_waypoints_s;
    vector<double> _map_waypoints_dx = map_waypoints_dx;
    vector<double> _map_waypoints_dy = map_waypoints_dy;
}

void PathPlanner::processMessage(string msg)
{
    auto j = json::parse(msg);

    string event = j[0].get<string>();

    if (event == "telemetry") {
        // j[1] is the data JSON object

        // Main car's localization Data
        double car_x = j[1]["x"];
        double car_y = j[1]["y"];
        double car_s = j[1]["s"];
        double car_d = j[1]["d"];
        double car_yaw = j[1]["yaw"];
        double car_speed = j[1]["speed"];

        // Previous path data given to the Planner
        auto previous_path_x = j[1]["previous_path_x"];
        auto previous_path_y = j[1]["previous_path_y"];
        // Previous path's end s and d values
        double end_path_s = j[1]["end_path_s"];
        double end_path_d = j[1]["end_path_d"];

        // Sensor Fusion Data, a list of all other cars on the same side of the road.
        auto sensor_fusion = j[1]["sensor_fusion"];


    }
}