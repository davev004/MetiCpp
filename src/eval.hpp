#pragma once
#include "board.hpp"
#include "types.hpp"

namespace Eval {

    // Standard centipawn piece values. 
    // King value is zero because we don't evaluate captured kings.
    constexpr int PAWN_VALUE   = 100;
    constexpr int KNIGHT_VALUE = 290;
    constexpr int BISHOP_VALUE = 320;
    constexpr int ROOK_VALUE   = 500;
    constexpr int QUEEN_VALUE  = 900;

    inline int evaluate(const Board& board) {
        int score = 0;

        // --- White Material ---
        score += __builtin_popcountll(board.bitboards[W_PAWN])   * PAWN_VALUE;
        score += __builtin_popcountll(board.bitboards[W_KNIGHT]) * KNIGHT_VALUE;
        score += __builtin_popcountll(board.bitboards[W_BISHOP]) * BISHOP_VALUE;
        score += __builtin_popcountll(board.bitboards[W_ROOK])   * ROOK_VALUE;
        score += __builtin_popcountll(board.bitboards[W_QUEEN])  * QUEEN_VALUE;

        // --- Black Material ---
        score -= __builtin_popcountll(board.bitboards[B_PAWN])   * PAWN_VALUE;
        score -= __builtin_popcountll(board.bitboards[B_KNIGHT]) * KNIGHT_VALUE;
        score -= __builtin_popcountll(board.bitboards[B_BISHOP]) * BISHOP_VALUE;
        score -= __builtin_popcountll(board.bitboards[B_ROOK])   * ROOK_VALUE;
        score -= __builtin_popcountll(board.bitboards[B_QUEEN])  * QUEEN_VALUE;

        // Return from the perspective of the side to move (Negamax standard).
        // The ternary operator here will compile to a branchless conditional move (cmov).
        return (board.state.sideToMove == WHITE) ? score : -score;
    }

}