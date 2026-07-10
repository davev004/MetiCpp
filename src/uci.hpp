#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>
#include "board.hpp"
#include "fen.hpp"
#include "search.hpp"
#include "movegen.hpp"
#include "move_logic.hpp"
#include "smp.hpp"
#include "time.hpp"

namespace UCI {

    // Thread management state
    inline std::thread search_thread;
    inline std::atomic<bool> is_searching{false};

    inline void join_search() {
        if (search_thread.joinable()) {
            search_thread.join();
        }
        is_searching.store(false, std::memory_order_relaxed);
    }

    // Helper to safely parse a string like "e2e4" or "e7e8q" into our 32-bit format
    inline Meti::Move parse_move(const Board& board, const std::string& move_str) {
        if (move_str.length() < 4) return 0;

        int from_file = move_str[0] - 'a';
        int from_rank = move_str[1] - '1';
        int to_file   = move_str[2] - 'a';
        int to_rank   = move_str[3] - '1';

        int from = from_rank * 8 + from_file;
        int to   = to_rank * 8 + to_file;

        // Check for promotion character
        Meti::PromotionPiece prom = Meti::PROMOTION_KNIGHT; // Default fallback
        bool is_prom = false;
        if (move_str.length() == 5) {
            is_prom = true;
            if (move_str[4] == 'q') prom = Meti::PROMOTION_QUEEN;
            else if (move_str[4] == 'r') prom = Meti::PROMOTION_ROOK;
            else if (move_str[4] == 'b') prom = Meti::PROMOTION_BISHOP;
            else if (move_str[4] == 'n') prom = Meti::PROMOTION_KNIGHT;
        }

        // Generate all moves and find the exact match to extract the flags
        Meti::MoveList list;
        MoveGen::generate(board, list);

        for (int i = 0; i < list.count; i++) {
            Meti::Move move = list.moves[i];
            if (Meti::get_from(move) == from && Meti::get_to(move) == to) {
                // If it's a promotion, ensure the promotion piece matches too
                if (is_prom) {
                    if (Meti::get_prom(move) == prom) return move;
                } else {
                    return move;
                }
            }
        }
        return 0; // Invalid move
    }

    // The asynchronous worker function
    inline void async_search_worker(Board board, int max_depth, long long allocated_ms) {
        // Run the Iterative Deepening search!
        Meti::Move best_move = SMP::launch(board, max_depth, allocated_ms);
        
        // Translate internal move back to UCI string protocol
        char from[3], to[3];
        Meti::square_to_coord(Meti::get_from(best_move), from);
        Meti::square_to_coord(Meti::get_to(best_move), to);
        
        std::string prom = "";
        Piece moving = Meti::get_moving(best_move);
        
        if ((moving == W_PAWN || moving == B_PAWN) && ((1ULL << Meti::get_to(best_move)) & 0xFF000000000000FFULL)) {
            Meti::PromotionPiece p = Meti::get_prom(best_move);
            if (p == Meti::PROMOTION_QUEEN) prom = "q";
            else if (p == Meti::PROMOTION_ROOK) prom = "r";
            else if (p == Meti::PROMOTION_BISHOP) prom = "b";
            else prom = "n";
        }

        // Fire it back to CuteChess
        std::cout << "bestmove " << from << to << prom << std::endl;
        
        is_searching.store(false, std::memory_order_relaxed);
    }

    inline void loop() {
        Board board;
        std::string line, token;

        FEN::parse(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        // Turn off sync for maximum I/O performance
        std::ios_base::sync_with_stdio(false);

        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            std::istringstream ss(line);
            ss >> token;

            if (token == "quit") {
                Time::time_up = true;
                join_search();
                break;
            } 
            else if (token == "stop") {
                if (is_searching.load(std::memory_order_relaxed)) {
                    Time::time_up = true;
                    join_search();
                }
            }
            else if (token == "uci") {
                std::cout << "id name MetiC++\n";
                std::cout << "id author David Vaughan\n";

                std::cout << "option name Threads type spin default 1 min 1 max " << SMP::MAX_THREADS << "\n";
                std::cout << "option name Hash type spin default 64 min 1 max 16384\n";
                
                std::cout << "uciok\n";
            } 
            else if (token == "isready") {
                std::cout << "readyok\n";
            } 
            else if (token == "position") {
                // Must stop thinking before modifying the board state
                if (is_searching.load(std::memory_order_relaxed)) {
                    Time::time_up = true;
                    join_search();
                }

                ss >> token;
                if (token == "startpos") {
                    FEN::parse(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                    ss >> token; // Consume "moves"
                } else if (token == "fen") {
                    std::string fen;
                    for (int i = 0; i < 6; ++i) { 
                        ss >> token;
                        fen += token + " ";
                    }
                    FEN::parse(board, fen);
                    ss >> token; // Consume "moves"
                }
                
                // --- THE NEW PARSER INTEGRATION ---
                while (ss >> token) {
                    Meti::Move parsed_move = parse_move(board, token);
                    if (parsed_move != 0) {
                        make_move(board, parsed_move);
                    }
                }
            } 
            else if (token == "go") {
                // Safely clean up any dangling thread
                join_search();

                int max_depth = 64; 
                long long wtime = 0, btime = 0;
                long long winc = 0, binc = 0;
                long long allocated_ms = 5000; 
                bool depth_only = false;
                
                // 1. Parse all clock data sent by the GUI
                while (ss >> token) {
                    if (token == "depth") { ss >> max_depth; depth_only = true; }
                    else if (token == "wtime") ss >> wtime;
                    else if (token == "btime") ss >> btime;
                    else if (token == "winc") ss >> winc;
                    else if (token == "binc") ss >> binc;
                }
                
                // 2. Time Allocation Logic
                if (depth_only && wtime == 0 && btime == 0) {
                    allocated_ms = 999999999; // Basically infinite time to reach the depth limit
                } else if (board.state.sideToMove == WHITE && wtime > 0) {
                    allocated_ms = (wtime / 20) + (winc / 2);
                    if (allocated_ms > wtime - 50) allocated_ms = std::max(10LL, wtime - 50);
                } else if (board.state.sideToMove == BLACK && btime > 0) {
                    allocated_ms = (btime / 20) + (binc / 2);
                    if (allocated_ms > btime - 50) allocated_ms = std::max(10LL, btime - 50);
                }

                // 3. Launch asynchronously
                is_searching.store(true, std::memory_order_relaxed);
                search_thread = std::thread(async_search_worker, board, max_depth, allocated_ms);
            }
            else if (token == "setoption") {
                ss >> token; // Consume "name"
                ss >> token; // Read option name
                
                if (token == "Threads") {
                    ss >> token; // Consume "value"
                    int t;
                    if (ss >> t) {
                        // Clamp the value safely between 1 and our stack array limit
                        SMP::active_threads = std::max(1, std::min(t, SMP::MAX_THREADS));
                    }
                }
                else if (token == "Hash") {
                    ss >> token; // Consume "value"
                    int megabytes;
                    if (ss >> megabytes) {
                        // Protect against insane allocations (clamp between 1MB and 16384MB/16GB)
                        megabytes = std::max(1, std::min(megabytes, 16384));
                        TT::allocate(megabytes);
                    }
                }
            }
        }
    }
}