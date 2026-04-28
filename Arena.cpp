#include "Arena.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <dlfcn.h>

RobotEntry::RobotEntry()
    : m_robot(nullptr), m_handle(nullptr), m_symbol('?'), m_alive(false),
      m_source_file(""), m_summary("") {
}

Arena::Arena()
    : m_height(20), m_width(20), m_max_rounds(100),
      m_sleep_interval(0.5), m_game_state_live(true),
      m_num_flamethrowers(0), m_num_pits(0), m_num_mounds(0),
      m_round(1) {
}

Arena::~Arena() {
}

std::string Arena::trim(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        start++;
    }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        end--;
    }

    return text.substr(start, end - start);
}

bool Arena::loadConfig(const std::string& filename) {
    std::ifstream fin(filename);
    if (!fin) {
        std::cerr << "Could not open config file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(fin, line)) {
        line = trim(line);

        if (line.empty()) {
            continue;
        }

        std::size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, colon_pos));
        std::string value = trim(line.substr(colon_pos + 1));

        if (key == "Arena_Size") {
            std::istringstream iss(value);
            iss >> m_height >> m_width;
        }
        else if (key == "Max_Rounds") {
            m_max_rounds = std::stoi(value);
        }
        else if (key == "Sleep_interval") {
            m_sleep_interval = std::stod(value);
        }
        else if (key == "Game_State_Live") {
            if (value == "true") {
                m_game_state_live = true;
            }
            else {
                m_game_state_live = false;
            }
        }
        else if (key == "Flamethrowers") {
            m_num_flamethrowers = std::stoi(value);
        }
        else if (key == "Pits") {
            m_num_pits = std::stoi(value);
        }
        else if (key == "Mounds") {
            m_num_mounds = std::stoi(value);
        }
    }

    std::cout << "Loaded config from: " << filename << std::endl;
    std::cout << "Arena size: " << m_height << " x " << m_width << std::endl;
    std::cout << "Max rounds: " << m_max_rounds << std::endl;
    std::cout << "Sleep interval: " << m_sleep_interval << std::endl;
    std::cout << "Game state live: " << (m_game_state_live ? "true" : "false") << std::endl;
    std::cout << "Flamethrowers: " << m_num_flamethrowers << std::endl;
    std::cout << "Pits: " << m_num_pits << std::endl;
    std::cout << "Mounds: " << m_num_mounds << std::endl;

    return true;
}

void Arena::initializeBoard() {
    m_board.clear();
    m_board.resize(m_height, std::vector<char>(m_width, '.'));
}

void Arena::printBoard() {
    std::cout << "   ";
    for (int col = 0; col < m_width; col++) {
        std::cout << col % 10 << ' ';
    }
    std::cout << std::endl;

    for (int row = 0; row < m_height; row++) {
        if (row < 10) {
            std::cout << ' ' << row << ' ';
        }
        else {
            std::cout << row << ' ';
        }

        for (int col = 0; col < m_width; col++) {
            std::cout << m_board[row][col] << ' ';
        }
        std::cout << std::endl;
    }
}

void Arena::placeRandomObstacle(char obstacle_char) {
    while (true) {
        int row = std::rand() % m_height;
        int col = std::rand() % m_width;

        if (m_board[row][col] == '.') {
            m_board[row][col] = obstacle_char;
            return;
        }
    }
}

void Arena::placeObstacles() {
    for (int i = 0; i < m_num_flamethrowers; i++) {
        placeRandomObstacle('F');
    }

    for (int i = 0; i < m_num_pits; i++) {
        placeRandomObstacle('P');
    }

    for (int i = 0; i < m_num_mounds; i++) {
        placeRandomObstacle('M');
    }
}

std::vector<std::string> Arena::findRobotSourceFiles() {
    std::vector<std::string> robot_files;

    for (const auto& entry : std::filesystem::directory_iterator("robots")) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string filename = entry.path().filename().string();

        if (filename.size() >= 10 &&
            filename.substr(0, 6) == "Robot_" &&
            filename.substr(filename.size() - 4) == ".cpp") {
            robot_files.push_back(entry.path().string());
        }
    }

    return robot_files;
}

void Arena::printRobotSourceFiles(const std::vector<std::string>& robot_files) {
    std::cout << "Robot source files found:" << std::endl;

    for (std::size_t i = 0; i < robot_files.size(); i++) {
        std::cout << "  " << robot_files[i] << std::endl;
    }
}

bool Arena::compileRobotSource(const std::string& robot_cpp_file, std::string& shared_lib_file) {
    std::filesystem::path cpp_path(robot_cpp_file);
    std::string stem = cpp_path.stem().string();   // example: Robot_Ratboy
    shared_lib_file = "robots/lib" + stem + ".so"; // example: robots/libRobot_Ratboy.so

    std::string compile_cmd =
        "g++ -shared -fPIC -o " + shared_lib_file + " " +
        robot_cpp_file + " RobotBase.o -I. -std=c++20";

    std::cout << "Compiling " << robot_cpp_file
              << " into " << shared_lib_file << "..." << std::endl;

    int result = std::system(compile_cmd.c_str());
    if (result != 0) {
        std::cerr << "Failed to compile " << robot_cpp_file << std::endl;
        return false;
    }

    return true;
}

bool Arena::loadRobotLibrary(const std::string& shared_lib_file, const std::string& source_file) {
    using RobotSummaryFn = const char* (*)();

    void* handle = dlopen(shared_lib_file.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load " << shared_lib_file << ": " << dlerror() << std::endl;
        return false;
    }

    RobotFactory create_robot = (RobotFactory)dlsym(handle, "create_robot");
    if (!create_robot) {
        std::cerr << "Failed to find create_robot in " << shared_lib_file << ": " << dlerror() << std::endl;
        dlclose(handle);
        return false;
    }

    RobotSummaryFn robot_summary = (RobotSummaryFn)dlsym(handle, "robot_summary");
    if (!robot_summary) {
        std::cerr << "Failed to find robot_summary in " << shared_lib_file << ": " << dlerror() << std::endl;
        dlclose(handle);
        return false;
    }

    const char* summary_text = robot_summary();
    if (!summary_text) {
        std::cerr << "robot_summary returned null for " << shared_lib_file << std::endl;
        dlclose(handle);
        return false;
    }

    RobotBase* robot = create_robot();
    if (!robot) {
        std::cerr << "Failed to create robot from " << shared_lib_file << std::endl;
        dlclose(handle);
        return false;
    }

    RobotEntry entry;
    entry.m_robot = robot;
    entry.m_handle = handle;
    entry.m_alive = true;
    entry.m_source_file = source_file;
    entry.m_summary = summary_text;

    m_robots.push_back(entry);

    std::cout << "Loaded robot from " << source_file << std::endl;
    std::cout << "Summary: " << entry.m_summary << std::endl;

    return true;
}

void Arena::loadRobots() {
    std::vector<std::string> robot_files = findRobotSourceFiles();

    if (robot_files.empty()) {
        std::cout << "No robot source files found." << std::endl;
        return;
    }

    for (std::size_t i = 0; i < robot_files.size(); i++) {
        std::string shared_lib_file;

        if (!compileRobotSource(robot_files[i], shared_lib_file)) {
            continue;
        }

        loadRobotLibrary(shared_lib_file, robot_files[i]);
    }

    std::cout << "Total robots loaded: " << m_robots.size() << std::endl;
}

void Arena::assignRobotSymbols() {
    std::string symbols = "@#$!%&*ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (std::size_t i = 0; i < m_robots.size(); i++) {
        if (i < symbols.size()) {
            m_robots[i].m_symbol = symbols[i];
        }
        else {
            m_robots[i].m_symbol = '?';
        }
    }
}

void Arena::placeRobots() {
    for (std::size_t i = 0; i < m_robots.size(); i++) {
        while (true) {
            int row = std::rand() % m_height;
            int col = std::rand() % m_width;

            if (m_board[row][col] == '.') {
                m_robots[i].m_robot->move_to(row, col);
                m_robots[i].m_robot->set_boundaries(m_height, m_width);
                m_board[row][col] = m_robots[i].m_symbol;
                break;
            }
        }
    }
}

int Arena::countLivingRobots() const {
    int count = 0;

    for (std::size_t i = 0; i < m_robots.size(); i++) {
        if (m_robots[i].m_alive && m_robots[i].m_robot->get_health() > 0) {
            count++;
        }
    }

    return count;
}

void Arena::printRobotStats() {
    std::cout << "Robot stats:" << std::endl;

    for (std::size_t i = 0; i < m_robots.size(); i++) {
        std::cout << "  " << m_robots[i].m_robot->print_stats() << std::endl;
    }
}

void Arena::addRadarCell(std::vector<RadarObj>& radar_results, int row, int col, int robot_row, int robot_col) {
    if (row < 0 || row >= m_height || col < 0 || col >= m_width) {
        return;
    }

    if (row == robot_row && col == robot_col) {
        return;
    }

    bool alive = false;
    if (isRobotAtLocation(row, col, alive)) {
        if (alive) {
            radar_results.push_back(RadarObj('R', row, col));
        }
        else {
            radar_results.push_back(RadarObj('X', row, col));
        }
        return;
    }

    char cell = m_board[row][col];
    if (cell == '.') {
        return;
    }

    radar_results.push_back(RadarObj(cell, row, col));
}

std::vector<RadarObj> Arena::performRadarScan(RobotBase* robot, int radar_direction) {
    std::vector<RadarObj> radar_results;

    int robot_row = 0;
    int robot_col = 0;
    robot->get_current_location(robot_row, robot_col);

    if (radar_direction == 0) {
        for (int dir = 1; dir <= 8; dir++) {
            int row = robot_row + directions[dir].first;
            int col = robot_col + directions[dir].second;
            addRadarCell(radar_results, row, col, robot_row, robot_col);
        }
        return radar_results;
    }

    int main_row_delta = directions[radar_direction].first;
    int main_col_delta = directions[radar_direction].second;

    int side_row_delta = 0;
    int side_col_delta = 0;

    if (radar_direction == 1 || radar_direction == 5) {
        side_row_delta = 0;
        side_col_delta = 1;
    }
    else if (radar_direction == 3 || radar_direction == 7) {
        side_row_delta = 1;
        side_col_delta = 0;
    }
    else if (radar_direction == 2 || radar_direction == 6) {
        side_row_delta = 1;
        side_col_delta = -1;
    }
    else if (radar_direction == 4 || radar_direction == 8) {
        side_row_delta = 1;
        side_col_delta = 1;
    }

    int current_row = robot_row + main_row_delta;
    int current_col = robot_col + main_col_delta;

    while (current_row >= 0 && current_row < m_height &&
           current_col >= 0 && current_col < m_width) {

        addRadarCell(radar_results, current_row, current_col, robot_row, robot_col);
        addRadarCell(radar_results, current_row + side_row_delta, current_col + side_col_delta, robot_row, robot_col);
        addRadarCell(radar_results, current_row - side_row_delta, current_col - side_col_delta, robot_row, robot_col);

        current_row += main_row_delta;
        current_col += main_col_delta;
    }

    return radar_results;
}

void Arena::printRadarResults(const std::vector<RadarObj>& radar_results) {
    if (radar_results.empty()) {
        std::cout << "  radar scan returned nothing" << std::endl;
        return;
    }

    std::cout << "  radar scan returned:" << std::endl;
    for (std::size_t i = 0; i < radar_results.size(); i++) {
        std::cout << "    " << radar_results[i].m_type
                  << " at (" << radar_results[i].m_row
                  << "," << radar_results[i].m_col << ")" << std::endl;
    }
}

int Arena::findRobotIndexAtLocation(int row, int col) {
    for (std::size_t i = 0; i < m_robots.size(); i++) {
        int robot_row = 0;
        int robot_col = 0;
        m_robots[i].m_robot->get_current_location(robot_row, robot_col);

        if (robot_row == row && robot_col == col) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool Arena::isRobotAtLocation(int row, int col, bool& alive_out) {
    int index = findRobotIndexAtLocation(row, col);
    if (index == -1) {
        return false;
    }

    alive_out = m_robots[index].m_alive && m_robots[index].m_robot->get_health() > 0;
    return true;
}

bool Arena::handleRobotShot(RobotEntry& robot_entry) {
    int shot_row = 0;
    int shot_col = 0;

    if (!robot_entry.m_robot->get_shot_location(shot_row, shot_col)) {
        std::cout << "  robot does not fire" << std::endl;
        return false;
    }

    std::cout << "  robot fires at (" << shot_row << "," << shot_col << ")" << std::endl;

    if (!isWithinBounds(shot_row, shot_col)) {
        std::cout << "  shot is out of bounds" << std::endl;
        return true;
    }

    WeaponType weapon = robot_entry.m_robot->get_weapon();

    if (weapon == railgun) {
        handleRailgunShot(robot_entry, shot_row, shot_col);
    }
    else if (weapon == hammer) {
        handleHammerShot(robot_entry, shot_row, shot_col);
    }
    else if (weapon == grenade) {
        handleGrenadeShot(robot_entry, shot_row, shot_col);
    }
    else if (weapon == flamethrower) {
        handleFlamethrowerShot(robot_entry, shot_row, shot_col);
    }

    return true;
}

void Arena::handleRailgunShot(RobotEntry& robot_entry, int shot_row, int shot_col) {
    int robot_row = 0;
    int robot_col = 0;
    robot_entry.m_robot->get_current_location(robot_row, robot_col);

    int delta_row = shot_row - robot_row;
    int delta_col = shot_col - robot_col;

    if (delta_row != 0) {
        if (delta_row > 0) {
            delta_row = 1;
        }
        else {
            delta_row = -1;
        }
    }

    if (delta_col != 0) {
        if (delta_col > 0) {
            delta_col = 1;
        }
        else {
            delta_col = -1;
        }
    }

    if (delta_row == 0 && delta_col == 0) {
        std::cout << "  railgun cannot target its own cell" << std::endl;
        return;
    }

    int current_row = robot_row + delta_row;
    int current_col = robot_col + delta_col;

    while (isWithinBounds(current_row, current_col)) {
        applyDamageToRobotAt(current_row, current_col, railgun);
        current_row += delta_row;
        current_col += delta_col;
    }
}

bool Arena::isBlockingCell(int row, int col) {
    if (!isWithinBounds(row, col)) {
        return true;
    }

    bool alive = false;
    if (isRobotAtLocation(row, col, alive)) {
        return true;
    }

    char cell = m_board[row][col];
    if (cell == 'M' || cell == 'X') {
        return true;
    }

    return false;
}

void Arena::handlePitCell(RobotEntry& robot_entry, int row, int col) {
    std::cout << "  robot falls into pit at (" << row << "," << col << ")" << std::endl;
    moveRobotOnBoard(robot_entry, row, col);
    robot_entry.m_robot->disable_movement();
}

void Arena::handleFlamethrowerCell(RobotEntry& robot_entry, int row, int col) {
    std::cout << "  robot passes through flamethrower at (" << row << "," << col << ")" << std::endl;
    moveRobotOnBoard(robot_entry, row, col);

    int damage = calculateWeaponDamage(flamethrower);
    int armor = robot_entry.m_robot->get_armor();

    double reduction = armor * 0.10;
    int reduced_damage = static_cast<int>(damage * (1.0 - reduction));

    if (reduced_damage < 0) {
        reduced_damage = 0;
    }

    robot_entry.m_robot->reduce_armor(1);
    int health_after = robot_entry.m_robot->take_damage(reduced_damage);

    std::cout << "  flamethrower damage dealt: " << reduced_damage << std::endl;
    std::cout << "  health now: " << health_after << std::endl;

    if (health_after <= 0) {
        robot_entry.m_alive = false;
        m_board[row][col] = 'X';
        std::cout << "  robot is destroyed" << std::endl;
    }
}

void Arena::handleHammerShot(RobotEntry& robot_entry, int shot_row, int shot_col) {
    int robot_row = 0;
    int robot_col = 0;
    robot_entry.m_robot->get_current_location(robot_row, robot_col);

    int row_diff = shot_row - robot_row;
    int col_diff = shot_col - robot_col;

    if (row_diff < -1 || row_diff > 1 || col_diff < -1 || col_diff > 1) {
        std::cout << "  hammer target is too far away" << std::endl;
        return;
    }

    if (row_diff == 0 && col_diff == 0) {
        std::cout << "  hammer cannot hit its own cell" << std::endl;
        return;
    }

    applyDamageToRobotAt(shot_row, shot_col, hammer);
}

void Arena::handleGrenadeShot(RobotEntry& robot_entry, int shot_row, int shot_col) {
    (void)robot_entry;

    for (int row = shot_row - 1; row <= shot_row + 1; row++) {
        for (int col = shot_col - 1; col <= shot_col + 1; col++) {
            if (isWithinBounds(row, col)) {
                applyDamageToRobotAt(row, col, grenade);
            }
        }
    }
}

void Arena::handleFlamethrowerShot(RobotEntry& robot_entry, int shot_row, int shot_col) {
    int robot_row = 0;
    int robot_col = 0;
    robot_entry.m_robot->get_current_location(robot_row, robot_col);

    int delta_row = shot_row - robot_row;
    int delta_col = shot_col - robot_col;

    if (delta_row != 0) {
        if (delta_row > 0) {
            delta_row = 1;
        }
        else {
            delta_row = -1;
        }
    }

    if (delta_col != 0) {
        if (delta_col > 0) {
            delta_col = 1;
        }
        else {
            delta_col = -1;
        }
    }

    int side_row = 0;
    int side_col = 0;

    if (delta_row != 0 && delta_col == 0) {
        side_row = 0;
        side_col = 1;
    }
    else if (delta_row == 0 && delta_col != 0) {
        side_row = 1;
        side_col = 0;
    }
    else {
        side_row = -delta_col;
        side_col = delta_row;
    }

    int current_row = robot_row;
    int current_col = robot_col;

    for (int step = 1; step <= 4; step++) {
        current_row += delta_row;
        current_col += delta_col;

        int center_row = current_row;
        int center_col = current_col;

        int left_row = center_row + side_row;
        int left_col = center_col + side_col;

        int right_row = center_row - side_row;
        int right_col = center_col - side_col;

        if (isWithinBounds(center_row, center_col)) {
            applyDamageToRobotAt(center_row, center_col, flamethrower);
        }

        if (isWithinBounds(left_row, left_col)) {
            applyDamageToRobotAt(left_row, left_col, flamethrower);
        }

        if (isWithinBounds(right_row, right_col)) {
            applyDamageToRobotAt(right_row, right_col, flamethrower);
        }
    }
}

bool Arena::handleRobotMovement(RobotEntry& robot_entry) {
    int move_direction = 0;
    int move_distance = 0;

    robot_entry.m_robot->get_move_direction(move_direction, move_distance);

    if (move_direction == 0 || move_distance == 0) {
        std::cout << "  robot does not move" << std::endl;
        return false;
    }

    int max_move = robot_entry.m_robot->get_move_speed();
    if (move_distance > max_move) {
        move_distance = max_move;
    }

    int current_row = 0;
    int current_col = 0;
    robot_entry.m_robot->get_current_location(current_row, current_col);

    int delta_row = directions[move_direction].first;
    int delta_col = directions[move_direction].second;

    int final_row = current_row;
    int final_col = current_col;

    for (int step = 0; step < move_distance; step++) {
        int next_row = final_row + delta_row;
        int next_col = final_col + delta_col;

        if (!isWithinBounds(next_row, next_col)) {
            break;
        }

        if (m_board[next_row][next_col] != '.') {
            break;
        }

        final_row = next_row;
        final_col = next_col;
    }

    if (final_row == current_row && final_col == current_col) {
        std::cout << "  robot could not move" << std::endl;
        return false;
    }

    std::cout << "  robot moves to (" << final_row << "," << final_col << ")" << std::endl;
    moveRobotOnBoard(robot_entry, final_row, final_col);
    return true;
}

bool Arena::isWithinBounds(int row, int col) const {
    return row >= 0 && row < m_height && col >= 0 && col < m_width;
}

void Arena::moveRobotOnBoard(RobotEntry& robot_entry, int new_row, int new_col) {
    int old_row = 0;
    int old_col = 0;
    robot_entry.m_robot->get_current_location(old_row, old_col);

    m_board[old_row][old_col] = '.';
    robot_entry.m_robot->move_to(new_row, new_col);
    m_board[new_row][new_col] = robot_entry.m_symbol;
}

int Arena::calculateWeaponDamage(WeaponType weapon) {
    if (weapon == railgun) {
        return 10 + (std::rand() % 11);
    }
    else if (weapon == hammer) {
        return 50 + (std::rand() % 11);
    }
    else if (weapon == grenade) {
        return 10 + (std::rand() % 31);
    }
    else if (weapon == flamethrower) {
        return 30 + (std::rand() % 21);
    }

    return 0;
}

void Arena::applyDamageToRobotAt(int row, int col, WeaponType weapon) {
    int robot_index = findRobotIndexAtLocation(row, col);
    if (robot_index == -1) {
        return;
    }

    if (!m_robots[robot_index].m_alive || m_robots[robot_index].m_robot->get_health() <= 0) {
        return;
    }

    RobotBase* target_robot = m_robots[robot_index].m_robot;

    int damage = calculateWeaponDamage(weapon);
    int armor = target_robot->get_armor();

    double reduction = armor * 0.10;
    int reduced_damage = static_cast<int>(damage * (1.0 - reduction));

    if (reduced_damage < 0) {
        reduced_damage = 0;
    }

    target_robot->reduce_armor(1);
    int health_after = target_robot->take_damage(reduced_damage);

    std::cout << "  hit robot at (" << row << "," << col << ")" << std::endl;
    std::cout << "  damage dealt: " << reduced_damage << std::endl;
    std::cout << "  health now: " << health_after << std::endl;

    if (health_after <= 0) {
        m_robots[robot_index].m_alive = false;
        m_board[row][col] = 'X';
        std::cout << "  robot is destroyed" << std::endl;
    }
}

int Arena::findLastLivingRobotIndex() const {
    for (std::size_t i = 0; i < m_robots.size(); i++) {
        if (m_robots[i].m_alive && m_robots[i].m_robot->get_health() > 0) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void Arena::run() {
    initializeBoard();
    placeObstacles();

    std::cout << "Arena started." << std::endl;
    std::cout << "Board size: " << m_height << " x " << m_width << std::endl;

    loadRobots();
    assignRobotSymbols();
    placeRobots();

    while (m_round <= m_max_rounds) {
        std::cout << std::endl;
        std::cout << "=========== starting round " << m_round << " ===========" << std::endl;

        printBoard();
        printRobotStats();

        if (countLivingRobots() <= 1) {
            std::cout << "Game over." << std::endl;

            int winner_index = findLastLivingRobotIndex();
            if (winner_index != -1) {
                std::cout << "Winner: robot " << m_robots[winner_index].m_symbol << std::endl;
                std::cout << m_robots[winner_index].m_robot->print_stats() << std::endl;
            }
            else {
                std::cout << "No robot survived." << std::endl;
            }

            break;
        }

        for (std::size_t i = 0; i < m_robots.size(); i++) {
            if (!m_robots[i].m_alive || m_robots[i].m_robot->get_health() <= 0) {
                continue;
            }

            std::cout << "Taking turn for robot " << m_robots[i].m_symbol << std::endl;

            int radar_direction = 0;
            m_robots[i].m_robot->get_radar_direction(radar_direction);

            std::cout << "  radar direction: " << radar_direction << std::endl;

            std::vector<RadarObj> radar_results =
                performRadarScan(m_robots[i].m_robot, radar_direction);

            printRadarResults(radar_results);

            m_robots[i].m_robot->process_radar_results(radar_results);

            bool fired = handleRobotShot(m_robots[i]);

            if (fired) {
                std::cout << "  shot action completed" << std::endl;
            }

            if (!fired) {
                handleRobotMovement(m_robots[i]);
            }
        }

        m_round++;
    }
}