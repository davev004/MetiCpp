#pragma once
#include "board.hpp"
#include "movegen.hpp"
#include "move_logic.hpp"
#include "threats.hpp"

namespace Perft {

    // Helper to determine if a move is legal (i.e., does not leave King in check)
    // Because we chose Option B, we scan for the King every time.
    inline bool is_legal(const Board& board, Meti::Move move) {
        Colour us = static_cast<Colour>(board.state.sideToMove);
        Colour them = (us == WHITE) ? BLACK : WHITE;
        Piece king = (us == WHITE) ? W_KING : B_KING;

        // Scan the bitboard for our King's position
        int king_sq = __builtin_ctzll(board.bitboards[king]);

        // After making the move, is the King under attack?
        return !Threats::is_square_attacked(board, king_sq, them);
    }

    inline uint64_t run(Board& board, int depth) {
        if (depth == 0) return 1;

        Meti::MoveList list;
        MoveGen::generate(board, list);

        uint64_t nodes = 0;

        for (int i = 0; i < list.count; ++i) {
            Meti::Move move = list.moves[i];

            make_move(board, move);

            // Approach A: Verify legality after making the move
            // We need to check if the move we just made left our King in check
            // Note: Since 'make_move' already flipped the turn, we must check against the *previous* side
            if (is_legal(board, move)) {
                nodes += run(board, depth - 1);
            }

            unmake_move(board, move);
        }

        return nodes;
    }
}