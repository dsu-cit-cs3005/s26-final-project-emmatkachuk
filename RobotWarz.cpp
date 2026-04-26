#include "Arena.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    Arena arena;

    if (!arena.loadConfig(argv[1])) {
        std::cerr << "Failed to load config file." << std::endl;
        return 1;
    }

    arena.run();
    return 0;
}