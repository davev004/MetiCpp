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
#include "transitiontable.hpp"
#include "smp.hpp"

int main() {
    // 1. Initialise core systems
    Zobrist::init();
    TT::allocate(64);
    SMP::init(); // Boot the persistent thread pool
    
    UCI::loop();

    SMP::stop_all(); // Clean shutdown when quit is received
    return 0;
}