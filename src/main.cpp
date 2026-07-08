#include <iostream>
#include <string>
#include <chrono>
#include "perft.hpp"
#include "constants.hpp"
#include "types.hpp"
#include "zobrist.hpp"
#include "board.hpp"
#include "fen.hpp"
#include "search.hpp"
#include "uci.hpp"

int main() {
    // 1. Initialise Zobrist Keys (Must be done exactly once at startup)
    Zobrist::init();

    UCI::loop();

    return 0;
}