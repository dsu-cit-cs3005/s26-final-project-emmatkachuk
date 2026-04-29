#include "RobotBase.h"
#include <vector>

class Robot_AmbusherBot : public RobotBase {
private:
    int m_target_row;
    int m_target_col;
    bool m_has_target;
    int m_scan_direction;
    int m_scan_mode_index;
    int m_no_target_turns;
    bool m_move_up;

public:
    Robot_AmbusherBot();

    virtual void get_radar_direction(int& radar_direction) override;
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override;
    virtual bool get_shot_location(int& shot_row, int& shot_col) override;
    virtual void get_move_direction(int& move_direction, int& move_distance) override;
};

Robot_AmbusherBot::Robot_AmbusherBot()
    : RobotBase(3, 4, railgun),
      m_target_row(-1),
      m_target_col(-1),
      m_has_target(false),
      m_scan_direction(3),
      m_scan_mode_index(0),
      m_no_target_turns(0),
      m_move_up(true) {
    m_name = "AmbusherBot";
}

void Robot_AmbusherBot::get_radar_direction(int& radar_direction) {
    int current_row = 0;
    int current_col = 0;
    get_current_location(current_row, current_col);

    if (m_no_target_turns >= 3) {
        radar_direction = 0;
        return;
    }

    if (current_col > 0) {
        radar_direction = 3;
        return;
    }

    int scan_choices[3] = {2, 3, 4};
    radar_direction = scan_choices[m_scan_mode_index];
    m_scan_mode_index = (m_scan_mode_index + 1) % 3;
}

void Robot_AmbusherBot::process_radar_results(const std::vector<RadarObj>& radar_results) {
    m_has_target = false;

    for (std::size_t i = 0; i < radar_results.size(); i++) {
        if (radar_results[i].m_type == 'R') {
            m_target_row = radar_results[i].m_row;
            m_target_col = radar_results[i].m_col;
            m_has_target = true;
            m_no_target_turns = 0;
            return;
        }
    }

    m_no_target_turns++;
}

bool Robot_AmbusherBot::get_shot_location(int& shot_row, int& shot_col) {
    if (m_has_target) {
        shot_row = m_target_row;
        shot_col = m_target_col;
        return true;
    }

    return false;
}

void Robot_AmbusherBot::get_move_direction(int& move_direction, int& move_distance) {
    int current_row = 0;
    int current_col = 0;
    get_current_location(current_row, current_col);

    if (current_col > 0) {
        move_direction = 7;
        move_distance = 1;
        return;
    }

    if (m_no_target_turns >= 6) {
        if (m_move_up) {
            if (current_row > 0) {
                move_direction = 1;
                move_distance = 1;
                m_move_up = false;
                return;
            }
            else {
                m_move_up = false;
            }
        }

        if (!m_move_up) {
            if (current_row < m_board_row_max - 1) {
                move_direction = 5;
                move_distance = 1;
                m_move_up = true;
                return;
            }
            else {
                m_move_up = true;
            }
        }
    }

    move_direction = 0;
    move_distance = 0;
}

extern "C" RobotBase* create_robot() {
    return new Robot_AmbusherBot();
}

extern "C" const char* robot_summary() {
    return "Holds wall, scans lanes, repositions if stalled.";
}