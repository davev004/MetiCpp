#pragma once

#include "board.hpp"
#include "move.hpp"
#include "threats.hpp"

bool make_move(Board& board, Meti::Move move);
void unmake_move(Board& board, Meti::Move move);

inline bool is_castling_legal(const Board& board, Meti::Move move) {
        if (Meti::get_flag(move) != Meti::MOVE_CASTLING) return true;

        Colour us = static_cast<Colour>(board.state.sideToMove);
        int from = Meti::get_from(move);
        int to = Meti::get_to(move);
        int transit = (from + to) / 2;
        Piece king = (us == WHITE) ? W_KING : B_KING;
        Piece rook = (us == WHITE) ? W_ROOK : B_ROOK;
        int rook_from = (to > from) ? from + 3 : from - 4;
        uint8_t right = 0;

        if (us == WHITE) right = (to > from) ? Castling::WK : Castling::WQ;
        else right = (to > from) ? Castling::BK : Castling::BQ;

        if (board.mailbox[from] != king || board.mailbox[rook_from] != rook) return false;
        if ((board.state.castlingRights & right) == 0) return false;

        return (us == WHITE)
                ? !Threats::is_square_attacked<BLACK>(board, from) &&
                    !Threats::is_square_attacked<BLACK>(board, transit)
                : !Threats::is_square_attacked<WHITE>(board, from) &&
                    !Threats::is_square_attacked<WHITE>(board, transit);
}

inline bool is_legal(const Board& board, Meti::Move) {
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

    return true;
}

inline bool is_valid_tt_move(const Board& board, Meti::Move move) {
        if (move == 0) return false;

        int from = Meti::get_from(move);
        int to = Meti::get_to(move);
        
        // 1. Boundary Protection: Prevents array out-of-bounds
        if (from < 0 || from > 63 || to < 0 || to > 63) return false;
        
        // 2. Physical Consistency: Does the moving piece actually exist here?
        Piece moving = Meti::get_moving(move);
        if (board.mailbox[from] != moving) return false;
        
        // 3. Ownership: Does this piece belong to the side to move?
        Colour us = static_cast<Colour>(board.state.sideToMove);
        Colour piece_colour = (moving <= W_KING) ? WHITE : BLACK;
        if (us != piece_colour) return false;

        // 4. Capture Consistency: If the move claims a capture, is the victim actually there?
        // (Bypass for En Passant, as the victim is not on the 'to' square)
        Piece captured = Meti::get_captured(move);
        Meti::MoveFlag flag = Meti::get_flag(move);
        
        if (flag == Meti::MOVE_CAPTURE && captured != PIECE_NONE) {
             if (board.mailbox[to] != captured) return false;
        }

    return true;
}