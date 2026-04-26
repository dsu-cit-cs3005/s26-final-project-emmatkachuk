#include "Arena.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdlib>

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

void Arena::run() {
    initializeBoard();
    placeObstacles();
    std::cout << "Arena started." << std::endl;
    std::cout << "Board size: " << m_height << " x " << m_width << std::endl;
    printBoard();
}