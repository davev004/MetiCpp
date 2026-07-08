#pragma once
#include <chrono>
#include <iostream>
#include "board.hpp"
#include "movegen.hpp"
#include "move_logic.hpp"
#include "eval.hpp"
#include "move_ordering.hpp"
#include "time.hpp"

namespace Search {
    
    constexpr int INF = 32000;
    constexpr int MATE = 30000;

    inline int quiescence(Board& board, int alpha, int beta, uint64_t& nodes) {
        Time::check(nodes);
        if (Time::time_up) return 0; // Abort instantly if time is up
    
        nodes++;

        // 1. Are we in check?
        Colour us = static_cast<Colour>(board.state.sideToMove);
        Colour opponent = static_cast<Colour>(board.state.sideToMove ^ 1);
        Piece our_king = (us == WHITE) ? W_KING : B_KING;
        uint64_t king_bb = board.bitboards[our_king];
        
        bool in_check = false;
        if (king_bb) {
            int king_sq = __builtin_ctzll(king_bb);
            in_check = (opponent == WHITE) ? 
                       Threats::is_square_attacked<WHITE>(board, king_sq) : 
                       Threats::is_square_attacked<BLACK>(board, king_sq);
        }

        // 2. The "Stand Pat" Score (ONLY if we are not in check)
        if (!in_check) {
            int stand_pat = Eval::evaluate(board);
            if (stand_pat >= beta) return beta;
            if (alpha < stand_pat) alpha = stand_pat;
        }

        Meti::MoveList list;
        MoveGen::generate(board, list);
        MoveOrdering::sort_moves(board, list);

        int legal_moves = 0;

        for (int i = 0; i < list.count; ++i) {
            Meti::Move move = list.moves[i];
            Meti::MoveFlag flag = Meti::get_flag(move);
            Piece moving = Meti::get_moving(move);
            int to = Meti::get_to(move);

            bool is_capture = (flag == Meti::MOVE_CAPTURE);
            bool is_promotion = ((moving == W_PAWN || moving == B_PAWN) && (to < 8 || to > 55));

            // 3. FILTER: If we are safe, only look at captures/promotions.
            // Because our sort puts all non-captures at the end, we can safely break out early!
            if (!in_check && !is_capture && !is_promotion) {
                break; 
            }

            make_move(board, move);
            if (!is_legal(board, move)) {
                unmake_move(board, move);
                continue;
            }
            legal_moves++;

            int score = -quiescence(board, -beta, -alpha, nodes);
            unmake_move(board, move);

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }

        // 4. Checkmate detection in Q-Search
        if (in_check && legal_moves == 0) {
            return -MATE;
        }

        return alpha;
    }

    // The Alpha-Beta Negamax Framework (Stateless & Recursive)
    inline int negamax(Board& board, int depth, int alpha, int beta, int ply, uint64_t& nodes) {
        Time::check(nodes);
        if (Time::time_up) return 0; // Abort instantly if time is up
        
        nodes++; // Increment the node count for performance tracking
        
        // Leaf node: return static evaluation
        if (depth == 0) return quiescence(board, alpha, beta, nodes);

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
    inline Meti::Move search_root(Board& board, int max_depth, long long allocated_ms, uint64_t& nodes) {
        Time::init(allocated_ms);

        // --- HARDCODED 1.c4 OPENING ---
        // If move 1, White to move, and a pawn is on c2 (index 10)
        if (board.state.fullMoveNumber == 1 && board.state.sideToMove == WHITE && board.mailbox[10] == W_PAWN) {
            return Meti::create_move(10, 26, W_PAWN, PIECE_NONE, Meti::MOVE_DOUBLE_PUSH);
        }

        Meti::Move best_move_overall = 0;

        // --- ITERATIVE DEEPENING LOOP ---
        for (int current_depth = 1; current_depth <= max_depth; current_depth++) {
            Meti::MoveList list;
            MoveGen::generate(board, list);
            MoveOrdering::sort_moves(board, list);

            int best_score = -INF;
            Meti::Move best_move_this_depth = 0;

            for (int i = 0; i < list.count; i++) {
                Meti::Move move = list.moves[i];
                
                make_move(board, move);
                if (!is_legal(board, move)) {
                    unmake_move(board, move);
                    continue;
                }

                int score = -negamax(board, current_depth - 1, -INF, INF, 1, nodes);
                unmake_move(board, move);

                // If we ran out of time mid-search, the scores are garbage. Break out.
                if (Time::time_up) break;

                if (score > best_score) {
                    best_score = score;
                    best_move_this_depth = move;
                }
            }

            // If time ran out during this depth, discard it entirely.
            if (Time::time_up) break;

            // Otherwise, we completed the depth safely. Lock in the best move.
            best_move_overall = best_move_this_depth;
            
            // Optional: Print UCI info per depth so CuteChess shows the engine thinking
            std::cout << "info depth " << current_depth << " score cp " << best_score << " nodes " << nodes << "\n";
        }

        return best_move_overall;
    }
}
