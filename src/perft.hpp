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
    // The side that just moved (the one who made the move we are validating)
    Colour us = static_cast<Colour>(board.state.sideToMove ^ 1);
    
    // The side whose turn it is now (the opponent of the side that just moved)
    Colour opponent = static_cast<Colour>(board.state.sideToMove);

    Piece our_king = (us == WHITE) ? W_KING : B_KING;
    uint64_t king_bb = board.bitboards[our_king];

    if (king_bb == 0) return false; 
    int king_sq = __builtin_ctzll(king_bb);

    // CRITICAL FIX: Pass 'opponent' as the third argument here
    if (Threats::is_square_attacked(board, king_sq, opponent)) return false;

    // Castling transit squares must also be checked against the opponent
    if (Meti::get_flag(move) == Meti::MOVE_CASTLING) {
        int from = Meti::get_from(move);
        int to = Meti::get_to(move);
        int transit = (from + to) / 2;

        if (Threats::is_square_attacked(board, from, opponent)) return false;
        if (Threats::is_square_attacked(board, transit, opponent)) return false;
    }

    return true;
}

    inline uint64_t run(Board& board, int depth) {
        if (depth == 0) return 1;

        Meti::MoveList list;
        MoveGen::generate(board, list);

        uint64_t nodes = 0;
        for (int i = 0; i < list.count; ++i) {
            if (list.moves[i] == 0) {
                std::cout << "CRITICAL: Zero-move detected at node!" << std::endl;
                continue; 
            }
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
        uint64_t before_occ = board.occupancy[2];
        make_move(board, move);
        if (before_occ == board.occupancy[2]) {
            std::cout << "CRITICAL: make_move did not update occupancy!" << std::endl;
        }
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