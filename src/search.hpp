#pragma once
#include <chrono>
#include <iostream>
#include "board.hpp"
#include "movegen.hpp"
#include "move_logic.hpp"
#include "eval.hpp"
#include "move_ordering.hpp"
#include "time.hpp"
#include "transitiontable.hpp"


namespace Search {
    
    constexpr int INF = 32000;
    constexpr int MATE = 30000;

    inline bool is_repetition(const Board& board) {
        // We only need to check back as far as the last irreversible move (capture/pawn push)
        int limit = board.ply - board.state.halfMoveClock;
        if (limit < 0) limit = 0;

        // Check our previous turns (stepping back by 2 plies)
        for (int i = board.ply - 2; i >= limit; i -= 2) {
            if (board.history[i].zobristKey == board.state.zobristKey) {
                return true;
            }
        }
        return false;
    }
    
    inline int quiescence(Board& board, int alpha, int beta, int ply, uint64_t& nodes) {
        Time::check(nodes);
        if (Time::time_up) return 0; // Abort instantly if time is up
    
        nodes++;

        // --- THE CIRCUIT BREAKERS ---
        // 1. Prevent infinite perpetual checks in Q-search
        if (board.state.halfMoveClock >= 100 || is_repetition(board)) return 0;
        // 2. Prevent array out-of-bounds / Stack Overflow
        if (ply >= Meti::MAX_PLY - 1) return Eval::evaluate(board);

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

            int score = -quiescence(board, -beta, -alpha, ply + 1, nodes);
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
        if (Time::time_up) return 0;
        
        nodes++;
        
        // --- 1. DRAW DETECTION (Repetition & 50-Move Rule) ---
        // If we are caught in a loop or hit 50 moves, score it as a 0 (Draw)
        if (board.state.halfMoveClock >= 100 || is_repetition(board)) {
            return 0; 
        }

        if (ply >= Meti::MAX_PLY - 1) return Eval::evaluate(board);

        // 1. Bound the search window to find the shortest mate
        int mate_value = MATE - ply;
        if (alpha < -mate_value) alpha = -mate_value;
        if (beta > mate_value - 1) beta = mate_value - 1;
        if (alpha >= beta) return alpha;

        if (depth == 0) return quiescence(board, alpha, beta, ply, nodes);

        int alpha_orig = alpha; // Keep original alpha to determine TT bound later

       // --- 1. PROBE THE TT ---
        int tt_score;
        Meti::Move tt_move = 0;
        
        if (TT::probe(board.state.zobristKey, depth, ply, alpha, beta, tt_score, tt_move)) {
            return tt_score; // Hard cutoff confirmed safe by XOR
        }

        Meti::MoveList list;
        MoveGen::generate(board, list);
        MoveOrdering::sort_moves(board, list, tt_move); // --- 2. PASS TT MOVE TO SORTER ---

        int legal_moves = 0;
        int best_score = -INF;
        Meti::Move best_move = 0; // Track the best move for this node

        for (int i = 0; i < list.count; ++i) {
            Meti::Move move = list.moves[i];
            
            make_move(board, move);
            if (!is_legal(board, move)) {
                unmake_move(board, move);
                continue;
            }
            legal_moves++;

            int score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, nodes);
            unmake_move(board, move);

            if (score > best_score) {
                best_score = score;
                best_move = move; // Record the move that caused the score increase
            }
            
            if (best_score > alpha) alpha = best_score;
            if (alpha >= beta) break; // Beta cutoff
        }

        // --- Terminal Node Detection ---
        if (legal_moves == 0) {
            Colour opponent = static_cast<Colour>(board.state.sideToMove ^ 1);
            Piece our_king = (board.state.sideToMove == WHITE) ? W_KING : B_KING;
            uint64_t king_bb = board.bitboards[our_king];
            
            if (king_bb == 0) return 0;
            int king_sq = __builtin_ctzll(king_bb);

            bool in_check = (opponent == WHITE) ? 
                            Threats::is_square_attacked<WHITE>(board, king_sq) : 
                            Threats::is_square_attacked<BLACK>(board, king_sq);

            if (in_check) return -MATE + ply; 
            else return 0; 
        }

        // --- 3. STORE IN TT ---
        TT::Bound bound;
        if (best_score <= alpha_orig) {
            bound = TT::BOUND_UPPER; // Fails low: all moves were worse than alpha
        } else if (best_score >= beta) {
            bound = TT::BOUND_LOWER; // Fails high: a move caused a beta cutoff
        } else {
            bound = TT::BOUND_EXACT; // PV node: score is exactly correct
        }
        
        TT::store(board.state.zobristKey, depth, ply, best_score, best_move, bound);

        return best_score;
    }

    // Added depth_offset (defaults to 0 for single-threaded/main thread usage)
    inline Meti::Move search_root(Board& board, int max_depth, long long allocated_ms, uint64_t& nodes, int depth_offset = 0, bool is_main_thread = true) {
        Meti::Move best_move_overall = 0;

        for (int current_depth = 1 + depth_offset; current_depth <= max_depth; current_depth++) {
            Meti::MoveList list;
            MoveGen::generate(board, list);
            
            // Pass the previous depth's best move to sort it first
            MoveOrdering::sort_moves(board, list, best_move_overall); 

            int alpha = -INF;
            int beta = INF;
            int best_score = -INF;
            Meti::Move best_move_this_depth = 0;

            for (int i = 0; i < list.count; i++) {
                Meti::Move move = list.moves[i];
                make_move(board, move);
                if (!is_legal(board, move)) { unmake_move(board, move); continue; }

                // Pass dynamic -beta and -alpha, NOT -INF and INF
                int score = -negamax(board, current_depth - 1, -beta, -alpha, 1, nodes);
                unmake_move(board, move);

                if (Time::time_up) break;

                if (score > best_score) {
                    best_score = score;
                    best_move_this_depth = move;
                }
                if (best_score > alpha) alpha = best_score; // Shrink the root window!
            }

            if (Time::time_up) break;
            best_move_overall = best_move_this_depth;
            
            if (is_main_thread) {
                std::cout << "info depth " << current_depth << " score cp " << best_score << " nodes " << nodes << std::endl;
            }
        }
        return best_move_overall;
    }
}
