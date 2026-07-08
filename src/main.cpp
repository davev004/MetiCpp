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

int main() {
    // 1. Initialise Zobrist Keys (Must be done exactly once at startup)
    Zobrist::init();

    Board board;
    std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::string kiwi_pete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

    // ==========================================
    // 1. STARTING POSITION BENCHMARKS
    // ==========================================
    std::cout << "=== STARTING POSITION ===\n";
    FEN::parse(board, start_fen);
    
    // PERFT
    uint64_t total_nodes = 0;
    uint64_t total_time_ms = 0;
    std::cout << "Perft nodes by depth:\n";
    for (int depth = 1; depth <= 6; ++depth) {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t nodes = Perft::run(board, depth);
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "Depth " << depth << ": " << nodes << " nodes (" << elapsed_ms << " ms)\n";
        total_nodes += nodes;
        total_time_ms += elapsed_ms;
    }
    
    if (total_time_ms > 0) {
        uint64_t nps_int = static_cast<uint64_t>((long double)total_nodes / ((long double)total_time_ms / 1000.0));
        std::cout << "Perft NPS: " << nps_int << "\n\n";
    }

    // SEARCH
    std::cout << "Search (Depth 6):\n";
    FEN::parse(board, start_fen); // Reset state just in case
    Meti::Move best_move_start = Search::search_root(board, 6);
    
    char from1[3], to1[3];
    Meti::square_to_coord(Meti::get_from(best_move_start), from1);
    Meti::square_to_coord(Meti::get_to(best_move_start), to1);
    std::cout << "Engine chose: " << from1 << to1 << "\n\n";

    // ==========================================
    // 2. KIWI PETE BENCHMARKS
    // ==========================================
    std::cout << "=== KIWI PETE ===\n";
    board = Board(); // Reset the board memory completely
    FEN::parse(board, kiwi_pete);
    
    // PERFT
    total_nodes = 0;
    total_time_ms = 0;
    std::cout << "Perft nodes by depth:\n";
    for (int depth = 1; depth <= 5; ++depth) {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t nodes = Perft::run(board, depth);
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "Depth " << depth << ": " << nodes << " nodes (" << elapsed_ms << " ms)\n";
        total_nodes += nodes;
        total_time_ms += elapsed_ms;
    }
    
    if (total_time_ms > 0) {
        uint64_t nps_int = static_cast<uint64_t>((long double)total_nodes / ((long double)total_time_ms / 1000.0));
        std::cout << "Perft NPS: " << nps_int << "\n\n";
    }

    // SEARCH
    std::cout << "Search (Depth 5):\n";
    FEN::parse(board, kiwi_pete); // Reset state
    Meti::Move best_move_kiwi = Search::search_root(board, 5);
    
    char from2[3], to2[3];
    Meti::square_to_coord(Meti::get_from(best_move_kiwi), from2);
    Meti::square_to_coord(Meti::get_to(best_move_kiwi), to2);
    std::cout << "Engine chose: " << from2 << to2 << "\n";

    return 0;
}