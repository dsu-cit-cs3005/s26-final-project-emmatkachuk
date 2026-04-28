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
    std::vector<std::string> findRobotSourceFiles();
    void printRobotSourceFiles(const std::vector<std::string>& robot_files);
    bool compileRobotSource(const std::string& robot_cpp_file, std::string& shared_lib_file);
    bool loadRobotLibrary(const std::string& shared_lib_file, const std::string& source_file);
    void loadRobots();
    void assignRobotSymbols();
    void placeRobots();
    int countLivingRobots() const;
    void printRobotStats();
    std::vector<RadarObj> performRadarScan(RobotBase* robot, int radar_direction);
    void addRadarCell(std::vector<RadarObj>& radar_results, int row, int col, int robot_row, int robot_col);
    void printRadarResults(const std::vector<RadarObj>& radar_results);
    bool isRobotAtLocation(int row, int col, bool& alive_out);
    int findRobotIndexAtLocation(int row, int col);
    bool handleRobotShot(RobotEntry& robot_entry);
};