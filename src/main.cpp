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
    // Force immediate output to prove the binary is actually running
    //std::cout << "DEBUG: Engine started execution!" << std::endl;
    
    Zobrist::init();
    //std::cout << "DEBUG: Zobrist initialized!" << std::endl;
    
    TT::allocate(64);
    //std::cout << "DEBUG: TT allocated!" << std::endl;
    
    SMP::init(); 
    //std::cout << "DEBUG: SMP thread pool initialized!" << std::endl;
    
    UCI::loop();

    SMP::stop_all(); 
    return 0;
}