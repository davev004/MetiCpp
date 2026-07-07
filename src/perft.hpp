#pragma once
#include <iostream>
#include "board.hpp"
#include "movegen.hpp"
#include "move_logic.hpp"
#include "threats.hpp"

namespace Perft {

    // Helper to determine if a move is legal (i.e., does not leave King in check)
    // Because we chose Option B, we scan for the King every time.
    inline bool is_legal(const Board& board, Meti::Move move) {
        // board.state.sideToMove has been flipped by make_move
        // so the side that just moved is the opposite of the current sideToMove
        Colour mover = static_cast<Colour>(board.state.sideToMove ^ 1);
        Colour opponent = static_cast<Colour>(board.state.sideToMove);
        Piece king = (mover == WHITE) ? W_KING : B_KING;

        // Scan the bitboard for the mover's King
        int king_sq = __builtin_ctzll(board.bitboards[king]);

        // Check if the mover's King is attacked by the opponent
        return !Threats::is_square_attacked(board, king_sq, opponent);
    }

    inline uint64_t run(Board& board, int depth) {
        if (depth == 0) return 1;

        Meti::MoveList list;
        MoveGen::generate(board, list);

        uint64_t nodes = 0;
        for (int i = 0; i < list.count; ++i) {
            make_move(board, list.moves[i]);

            if (is_legal(board, list.moves[i])) {
                nodes += run(board, depth - 1);
            }
            unmake_move(board, list.moves[i]);
        }
        return nodes;
    }
}