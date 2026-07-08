#pragma once

#include "board.hpp"
#include "move.hpp"
#include "threats.hpp"

void make_move(Board& board, Meti::Move move);
void unmake_move(Board& board, Meti::Move move);

inline bool is_legal(const Board& board, Meti::Move move) {
    Colour us = static_cast<Colour>(board.state.sideToMove ^ 1);
    Colour opponent = static_cast<Colour>(board.state.sideToMove);

    Piece our_king = (us == WHITE) ? W_KING : B_KING;
    uint64_t king_bb = board.bitboards[our_king];
    
    if (king_bb == 0) return false; 
    int king_sq = __builtin_ctzll(king_bb);

    // Dispatch to the branchless threat scanner
    bool in_check = (opponent == WHITE) ? 
                    Threats::is_square_attacked<WHITE>(board, king_sq) : 
                    Threats::is_square_attacked<BLACK>(board, king_sq);

    if (in_check) return false;

    if (Meti::get_flag(move) == Meti::MOVE_CASTLING) {
        int from = Meti::get_from(move);
        int transit = (from + Meti::get_to(move)) / 2;

        if (opponent == WHITE) {
            if (Threats::is_square_attacked<WHITE>(board, from)) return false;
            if (Threats::is_square_attacked<WHITE>(board, transit)) return false;
        } else {
            if (Threats::is_square_attacked<BLACK>(board, from)) return false;
            if (Threats::is_square_attacked<BLACK>(board, transit)) return false;
        }
    }

    return true;
}