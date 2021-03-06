#include "path_planner.h"
#include "json.hpp"
#include "spline.h"

using namespace std;
using json = nlohmann::json;

const double SAFE_DISTANCE = 8;
const double FRONT_DISTANCE = 15;

void PathPlanner::setMap(vector<double> map_waypoints_x,
                         vector<double> map_waypoints_y,
                         vector<double> map_waypoints_s,
                         vector<double> map_waypoints_dx,
                         vector<double> map_waypoints_dy)
{
    _map_waypoints_x = map_waypoints_x;
    _map_waypoints_y = map_waypoints_y;
    _map_waypoints_s = map_waypoints_s;
    _map_waypoints_dx = map_waypoints_dx;
    _map_waypoints_dy = map_waypoints_dy;


    _lane = 1;

    _ref_vel = 0; //mph
    _target_vel = 49.5;

    _state = PathPlanner::FOLLOW_IN_LANE;
}

void PathPlanner::processMessage(string msg)
{
    auto j = json::parse(msg);

    string event = j[0].get<string>();
    unsigned int i;

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
        // [ id, x, y, vx, vy, s, d]
        auto sensor_fusion = j[1]["sensor_fusion"];

        int prev_size = previous_path_x.size();

        if(prev_size > 0){
            car_s = end_path_s;
        }

        bool slow_vehicle_in_front = false;
        bool left_lane_safe = true;
        bool right_lane_safe = true;
        // -1 for left lane, 0 for current lane, 1 for right lane
        int fastest_lane = 0;
        //bool change_lane_complete = false;

        double lane_speed[3] = {999,999,999};
        // prediction based on fusion data

        if(car_d<(2+4*_lane+1) && car_d>(2+4*_lane-1)){
            _change_lane_complete = true;
        }

        if(_lane == 0){
            left_lane_safe = false;
            lane_speed[0] = 0;
        }
        if(_lane == 2){
            right_lane_safe = false;
            lane_speed[2] = 0;
        }

        for(i=0;i<sensor_fusion.size();++i)
        {
            float d = sensor_fusion[i][6];
            // car is in my lane
            if(d<(2+4*_lane+2) && d>(2+4*_lane-2))
            {
                double vx = sensor_fusion[i][3];
                double vy = sensor_fusion[i][4];
                double check_speed = sqrt(vx*vx+vy*vy);
                double check_car_s = sensor_fusion[i][5];

                // if using previous points can project a value outwards in time
                check_car_s += (double)prev_size*0.02*check_speed;

                // check s value greater than mine and s gap
                if(check_car_s > car_s && (check_car_s - car_s) < FRONT_DISTANCE){
                    slow_vehicle_in_front = true;
                    if(lane_speed[1] > check_speed){
                        lane_speed[1] = check_speed;
                    }
                }
            // check left lane
            }else if(_lane != 0 &&d<(2+4*(_lane-1)+2) && d>(2+4*(_lane-1)-2)){
                double vx = sensor_fusion[i][3];
                double vy = sensor_fusion[i][4];
                double check_speed = sqrt(vx*vx+vy*vy);
                double check_car_s = sensor_fusion[i][5];

                check_car_s += (double)prev_size*0.02*check_speed;

                if(fabs(car_s - check_car_s) < SAFE_DISTANCE*2
                   && (lane_speed[0] > check_speed)){
                    lane_speed[0] = check_speed;
                }


                if(fabs(car_s - check_car_s) < SAFE_DISTANCE){
                    left_lane_safe = false;
                }
            // check right lane
            }else if(_lane != 2 &&d<(2+4*(_lane+1)+2) && d>(2+4*(_lane+1)-2)){
                double vx = sensor_fusion[i][3];
                double vy = sensor_fusion[i][4];
                double check_speed = sqrt(vx*vx+vy*vy);
                double check_car_s = sensor_fusion[i][5];

                check_car_s += (double)prev_size*0.02*check_speed;

                if(fabs(car_s - check_car_s) < SAFE_DISTANCE*2
                   && (lane_speed[2] > check_speed)){
                    lane_speed[2] = check_speed;
                }


                if(fabs(car_s - check_car_s) < SAFE_DISTANCE){
                    right_lane_safe = false;
                }
            }
        }

        fastest_lane = 1;
        for(i=0;i<3;++i)
        {
            if(lane_speed[i]>lane_speed[fastest_lane]){
                fastest_lane = i;
            }
        }

        for(i=0;i<3;++i)
        {
            if(lane_speed[i] == 999){
                lane_speed[i] = 49.5;
            }
        }

        // change state based on the signals provided by prediction
        int current_state = _state;
        int current_lane = _lane;

        if(_state == PathPlanner::FOLLOW_IN_LANE){
            if(slow_vehicle_in_front){
                _target_vel = lane_speed[1] * 3600 / 1609;
                if(fastest_lane != 1){
                    _state = PathPlanner::PREPARE_LANE_CHANGE;
                }
            }
        }else if(_state == PathPlanner::PREPARE_LANE_CHANGE){
            _target_vel = lane_speed[1] * 3600 / 1609;
            if(fastest_lane == 0){
                 if(left_lane_safe){
                    // turn left
                    _lane = _lane-1;
                    _state = PathPlanner::CHANGE_LANE;
                    _change_lane_complete = false;
                }
            }else if(fastest_lane == 2){
                if(right_lane_safe){
                    // turn right
                    _lane = _lane+1;
                    _state = PathPlanner::CHANGE_LANE;
                    _change_lane_complete = false;
                }
            }
        }else if(_state == PathPlanner::CHANGE_LANE){
            if(_change_lane_complete){
                _state = PathPlanner::FOLLOW_IN_LANE;
                _target_vel = 49.5;
            }
        }
        //if(current_state != _state){
        //    cout << "++++++++++++++++++++++++++++++" << endl;
        //    cout << "current lane : " << current_lane << endl;
        //    cout << "state == " << current_state << " ;    ";
        //    cout << "next state == " << _state << endl;
        //    cout << "slow vehicle infront : " << slow_vehicle_in_front << endl;
        //    cout << "left lane safe : " << left_lane_safe << endl;
        //    cout << "right lane safe : " << right_lane_safe << endl;
        //    cout << "next lane : " << _lane << endl;
        //    cout << "_change_lane_complete : " << _change_lane_complete << endl;

        //    cout << "lane speed left : " << lane_speed[0] << endl;
        //    cout << "lane speed current: " << lane_speed[1] << endl;
        //    cout << "lane speed right: " << lane_speed[2] << endl;

        //    cout<< "-----------------------------------------" << endl;
            //}

        // take actions based on state change

        if(_target_vel - _ref_vel > 0.224){
            //_ref_vel += .224;
            _ref_vel += .448;
        }else if(_ref_vel - _target_vel > 0.224){
            //_ref_vel -= .224;
            _ref_vel -= .448;
        }

        vector<double> ptsx;
        vector<double> ptsy;

        double ref_x = car_x;
        double ref_y = car_y;
        double ref_yaw = deg2rad(car_yaw);


        if(prev_size<2){
            double prev_car_x = car_x - cos(car_yaw);
            double prev_car_y = car_y - sin(car_yaw);
            ptsx.push_back(prev_car_x);
            ptsx.push_back(car_x);
            ptsy.push_back(prev_car_y);
            ptsy.push_back(car_y);
        }else{
            ref_x = previous_path_x[prev_size-1];
            ref_y = previous_path_y[prev_size-1];
            double ref_x_prev = previous_path_x[prev_size-2];
            double ref_y_prev = previous_path_y[prev_size-2];
            ref_yaw = atan2(ref_y-ref_y_prev,ref_x-ref_x_prev);
            ptsx.push_back(ref_x_prev);
            ptsx.push_back(ref_x);
            ptsy.push_back(ref_y_prev);
            ptsy.push_back(ref_y);
        }

        vector<double> next_wp0 = getXY(car_s+30,(2+4*_lane),
                                        _map_waypoints_s,_map_waypoints_x,_map_waypoints_y);
        vector<double> next_wp1 = getXY(car_s+60,(2+4*_lane),
                                        _map_waypoints_s,_map_waypoints_x,_map_waypoints_y);
        vector<double> next_wp2 = getXY(car_s+90,(2+4*_lane),
                                        _map_waypoints_s,_map_waypoints_x,_map_waypoints_y);
        ptsx.push_back(next_wp0[0]);
        ptsx.push_back(next_wp1[0]);
        ptsx.push_back(next_wp2[0]);

        ptsy.push_back(next_wp0[1]);
        ptsy.push_back(next_wp1[1]);
        ptsy.push_back(next_wp2[1]);

        // shift to car point of view, (0,0) is the current position, yaw is always 0
        for(i=0;i<ptsx.size();++i)
        {
            // shift
            double shift_x = ptsx[i] - ref_x;
            double shift_y = ptsy[i] - ref_y;
            // rotation
            ptsx[i] = (shift_x * cos(-ref_yaw)-shift_y*sin(-ref_yaw));
            ptsy[i] = (shift_x * sin(-ref_yaw)+shift_y*cos(-ref_yaw));
        }

        tk::spline s;
        s.set_points(ptsx,ptsy);
        vector<double> next_x_vals;
        vector<double> next_y_vals;

        for(i=0;i<previous_path_x.size();++i)
        {
            next_x_vals.push_back(previous_path_x[i]);
            next_y_vals.push_back(previous_path_y[i]);
        }

        double target_x = 30.0;
        double target_y = s(target_x);
        double target_dist = sqrt(target_x*target_x+target_y*target_y);

        double x_add_on = 0;
        for(i=1;i<=50-previous_path_x.size();++i)
        {
            // 2.24 is for transform from miles/hour to m/s
            double N = target_dist/(0.02*_ref_vel/2.24);
            double x_point = x_add_on + target_x/N;
            double y_point = s(x_point);
            x_add_on = x_point;

            double x_ref = x_point;
            double y_ref = y_point;

            x_point = x_ref*cos(ref_yaw)-y_ref*sin(ref_yaw);
            y_point = x_ref*sin(ref_yaw)+y_ref*cos(ref_yaw);

            x_point += ref_x;
            y_point += ref_y;

            next_x_vals.push_back(x_point);
            next_y_vals.push_back(y_point);
        }

        _next_x_vals = next_x_vals;
        _next_y_vals = next_y_vals;

    }
}