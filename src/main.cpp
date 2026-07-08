#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include "perft.hpp"
#include "constants.hpp"
#include "types.hpp"
#include "zobrist.hpp"
#include "board.hpp"
#include "bitboard.hpp"
#include "fen.hpp"
#include "debug.hpp" 


int main() {


    // 1. Initialize Zobrist Keys (Must be done exactly once at startup)
    Zobrist::init();

    Board board;
    std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    FEN::parse(board, start_fen);
    uint64_t total_nodes = 0;
    uint64_t total_time_ms = 0;
    std::cout << "Perft nodes by depth for: " << start_fen << "\n";
    for (int depth = 1; depth <= 6; ++depth) {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t nodes = Perft::run(board, depth);
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "Depth " << depth << ": " << nodes << " nodes";
        std::cout << " (" << elapsed_ms << " ms)\n";
        total_nodes += nodes;
        total_time_ms += elapsed_ms;
    }
    std::cout << "Total nodes: " << total_nodes << "\n";
    std::cout << "Total time: " << total_time_ms << " ms\n";
    //nodes per second
    if (total_time_ms > 0) {
        long double nps = (long double)total_nodes / ((long double)total_time_ms / 1000.0);
        uint64_t nps_int = static_cast<uint64_t>(nps);
        std::cout << "Nodes per second: " << nps_int << "\n";
    }
    
    //std::cout << "\n===== PERFT DIVIDE (Depth 5) =====\n";
    //Perft::divide(board, 5);

    return 0;
}

