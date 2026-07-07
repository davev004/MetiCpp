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
        // The side that just moved is the opposite of the current sideToMove
        Colour mover = static_cast<Colour>(board.state.sideToMove ^ 1);
        Colour opponent = static_cast<Colour>(board.state.sideToMove);
        Piece king = (mover == WHITE) ? W_KING : B_KING;

        // Scan for the mover's King
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
            } else {

            }

            unmake_move(board, list.moves[i]);
        }
        return nodes;
    }

    inline void divide(Board& board, int depth) {
    Meti::MoveList list;
    MoveGen::generate(board, list);

    uint64_t total = 0;

    for (int i = 0; i < list.count; ++i) {
        Meti::Move move = list.moves[i];

        make_move(board, move);

        if (is_legal(board, move)) {
            uint64_t nodes = run(board, depth - 1);

            char from[3], to[3];
            Meti::square_to_coord(Meti::get_from(move), from);
            Meti::square_to_coord(Meti::get_to(move), to);

            std::cout << from << to << ": " << nodes << " flag: " << Meti::get_flag(move) << '\n';

            total += nodes;
        }

        unmake_move(board, move);
    }

    std::cout << "Total: " << total << '\n';
}
}