#pragma once

#include "RobotBase.h"
#include "RadarObj.h"
#include <string>
#include <vector>

struct RobotEntry {
    RobotBase* m_robot;
    void* m_handle;
    char m_symbol;
    bool m_alive;
    std::string m_source_file;
    std::string m_summary;
    RobotEntry();
};

class Arena {
public:
    Arena();
    ~Arena();
    bool loadConfig(const std::string& filename);
    void run();

private:
    int m_height;
    int m_width;
    int m_max_rounds;
    double m_sleep_interval;
    bool m_game_state_live;
    int m_num_flamethrowers;
    int m_num_pits;
    int m_num_mounds;
    int m_round;
    std::vector<std::vector<char>> m_board;
    std::vector<RobotEntry> m_robots;
    void initializeBoard();
    std::string trim(const std::string& text);
    void printBoard();
    void placeRandomObstacle(char obstacle_char);
    void placeObstacles();
};