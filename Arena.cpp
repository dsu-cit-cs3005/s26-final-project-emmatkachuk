#include "Arena.h"
#include <iostream>

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

bool Arena::loadConfig(const std::string& filename) {
    std::cout << "Loading config from: " << filename << std::endl;
    return true;
}

void Arena::initializeBoard() {
    m_board.clear();
    m_board.resize(m_height, std::vector<char>(m_width, '.'));
}

void Arena::run() {
    initializeBoard();
    std::cout << "Arena started." << std::endl;
    std::cout << "Board size: " << m_height << " x " << m_width << std::endl;
}