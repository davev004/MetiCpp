#pragma once
#include <chrono>
#include <iostream>
#include "board.hpp"
#include "movegen.hpp"
#include "move_logic.hpp"
#include "eval.hpp"
#include "move_ordering.hpp"

namespace Search {
    
    constexpr int INF = 32000;
    constexpr int MATE = 30000;

    // The Alpha-Beta Negamax Framework (Stateless & Recursive)
    inline int negamax(Board& board, int depth, int alpha, int beta, int ply, uint64_t& nodes) {
        nodes++; // Increment the node count for performance tracking
        
        // Leaf node: return static evaluation
        if (depth == 0) return Eval::evaluate(board);

        Meti::MoveList list;
        MoveGen::generate(board, list);
        MoveOrdering::sort_moves(board, list);

        int legal_moves = 0;
        int best_score = -INF;

        for (int i = 0; i < list.count; ++i) {
            Meti::Move move = list.moves[i];
            
            make_move(board, move);
            if (!is_legal(board, move)) {
                unmake_move(board, move);
                continue;
            }
            legal_moves++;

            // Recursively search the resulting position. 
            // Notice the window is inverted (-beta, -alpha) and the result is negated.
            int score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, nodes);
            
            unmake_move(board, move);

            if (score > best_score) best_score = score;
            
            // --- The "Two Lines" of Alpha-Beta Pruning ---
            if (best_score > alpha) alpha = best_score;
            if (alpha >= beta) break; // A refutation was found; prune this branch!
        }

        // Terminal Node Detection (This is how the King is protected)
        if (legal_moves == 0) {
            Colour opponent = static_cast<Colour>(board.state.sideToMove ^ 1);
            Piece our_king = (board.state.sideToMove == WHITE) ? W_KING : B_KING;
            uint64_t king_bb = board.bitboards[our_king];
            
            if (king_bb == 0) return 0; // Failsafe
            int king_sq = __builtin_ctzll(king_bb);

            bool in_check = (opponent == WHITE) ? 
                            Threats::is_square_attacked<WHITE>(board, king_sq) : 
                            Threats::is_square_attacked<BLACK>(board, king_sq);

            if (in_check) {
                // We are in checkmate. We return -MATE, but we add `ply` to the score.
                // This ensures the engine prefers to mate the opponent as quickly as possible, 
                // and delays getting mated for as long as possible.
                return -MATE + ply; 
            } else {
                // Stalemate is a draw (0)
                return 0; 
            }
        }

        return best_score;
    }

    // The Root function: Kicks off the search and remembers the actual move
    inline Meti::Move search_root(Board& board, int depth) {
        Meti::MoveList list;
        MoveGen::generate(board, list);

        MoveOrdering::sort_moves(board, list);
        
        int best_score = -INF;
        Meti::Move best_move = 0;
        uint64_t nodes = 0; // Stack-allocated counter

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < list.count; ++i) {
            Meti::Move move = list.moves[i];
            make_move(board, move);
            
            if (!is_legal(board, move)) {
                unmake_move(board, move);
                continue;
            }

            int score = -negamax(board, depth - 1, -INF, INF, 1, nodes);
            unmake_move(board, move);

            if (score > best_score) {
                best_score = score;
                best_move = move;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        uint64_t nps = (elapsed_ms > 0) ? (nodes * 1000) / elapsed_ms : 0;

        // Print in standard UCI format
        std::cout << "info depth " << depth 
                  << " score cp " << best_score 
                  << " nodes " << nodes 
                  << " nps " << nps 
                  << " time " << elapsed_ms << "\n";

        return best_move;
    }
}