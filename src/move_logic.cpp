#include "move_logic.hpp"
#include "zobrist.hpp"
#include "bitboard.hpp"
#include "constants.hpp"

// Helper: Fast piece lookup (only used when moving piece isn't cached in the Move)
inline Piece get_piece_at(const Board& board, int sq) {
    for (int p = 0; p < Meti::NUM_PIECES; ++p) {
        if (Bitboard::check_bit(board.bitboards[p], sq)) return static_cast<Piece>(p);
    }
    return PIECE_NONE;
}

void make_move(Board& board, Meti::Move move) {
    int from = Meti::get_from(move);
    int to = Meti::get_to(move);
    int flag = Meti::get_flag(move);
    int prom = Meti::get_prom(move);
    uint8_t previous_en_passant = board.state.enPassantSquare;

    // 1. Snapshot State
    board.history[board.ply++] = board.state;

    // 2. Identify Pieces
    Piece moving = get_piece_at(board, from);
    Piece captured = get_piece_at(board, to);
    bool is_en_passant = (flag == Meti::MOVE_CAPTURE && captured == PIECE_NONE && previous_en_passant == to);
    int capture_sq = to;
    if (is_en_passant) {
        capture_sq = (moving <= W_KING) ? (to - 8) : (to + 8);
        captured = get_piece_at(board, capture_sq);
    }

    // 3. Zobrist: Remove moving piece, side-to-move, and old EP/Castle states
    board.state.zobristKey ^= Zobrist::pieceKeys[moving][from];
    board.state.zobristKey ^= Zobrist::sideKey;
    board.state.zobristKey ^= Zobrist::castleKeys[board.state.castlingRights];
    if (board.state.enPassantSquare != Meti::SQ_NONE) 
        board.state.zobristKey ^= Zobrist::enPassantKeys[board.state.enPassantSquare % 8];

    // --- NEW: Branchless Castling Rights Update ---
    board.state.castlingRights &= Meti::CASTLE_RIGHTS_UPDATE[from];
    board.state.castlingRights &= Meti::CASTLE_RIGHTS_UPDATE[to];

    // 4. Update State Flags
    board.state.capturedPiece = captured;
    board.state.enPassantSquare = Meti::SQ_NONE; // Reset by default
    
    // 5. Apply Move (XOR Updates)
    uint64_t from_to_mask = (1ULL << from) | (1ULL << to);
    board.bitboards[moving] ^= from_to_mask;
    
    Colour us = (moving <= W_KING) ? WHITE : BLACK;
    board.occupancy[us] ^= from_to_mask;

    // Handle Capture
    if (captured != PIECE_NONE) {
        board.bitboards[captured] ^= (1ULL << capture_sq);
        board.occupancy[us ^ 1] ^= (1ULL << capture_sq);
        board.state.zobristKey ^= Zobrist::pieceKeys[captured][capture_sq];
    }

    // Handle Special Move Flags
    if (flag == 1) { // Double Push
        board.state.enPassantSquare = (us == WHITE) ? (to - 8) : (to + 8);
    } else if (flag == 2) { // Castling
        // King is already moved by the primary from_to_mask. 
        // We only need to move the Rook.
        int rook_from, rook_to;
        
        // Determine Rook coordinates based on King's destination
        if (to == 6)       { rook_from = 7;  rook_to = 5;  } // White Kingside (g1)
        else if (to == 2)  { rook_from = 0;  rook_to = 3;  } // White Queenside (c1)
        else if (to == 62) { rook_from = 63; rook_to = 61; } // Black Kingside (g8)
        else               { rook_from = 56; rook_to = 59; } // Black Queenside (c8)

        Piece rook = (us == WHITE) ? W_ROOK : B_ROOK;
        uint64_t rook_mask = (1ULL << rook_from) | (1ULL << rook_to);

        // Apply XOR to Bitboards and Occupancy
        board.bitboards[rook] ^= rook_mask;
        board.occupancy[us] ^= rook_mask;

        // Apply XOR to Zobrist Keys
        board.state.zobristKey ^= Zobrist::pieceKeys[rook][rook_from];
        board.state.zobristKey ^= Zobrist::pieceKeys[rook][rook_to];
    }

    // 6. Final Zobrist Update
    board.state.zobristKey ^= Zobrist::pieceKeys[moving][to];
    board.state.zobristKey ^= Zobrist::castleKeys[board.state.castlingRights];
    if (board.state.enPassantSquare != Meti::SQ_NONE) 
        board.state.zobristKey ^= Zobrist::enPassantKeys[board.state.enPassantSquare % 8];
    
    board.state.sideToMove ^= 1;
    board.occupancy[2] = board.occupancy[WHITE] | board.occupancy[BLACK];
}

void unmake_move(Board& board, Meti::Move move) {
    int from = Meti::get_from(move);
    int to = Meti::get_to(move);
    int flag = Meti::get_flag(move);
    GameState previous_state = board.history[board.ply - 1];

    // 1. Revert Move (Bitboard XOR is its own inverse)
    Piece moving = get_piece_at(board, to);
    uint64_t move_mask = (1ULL << from) | (1ULL << to);
    
    board.bitboards[moving] ^= move_mask;
    board.occupancy[board.state.sideToMove ^ 1] ^= move_mask; // Revert occupancy

    // 2. Restore Captured Piece
    if (board.state.capturedPiece != PIECE_NONE) {
        int capture_sq = to;
        if (flag == Meti::MOVE_CAPTURE && previous_state.enPassantSquare == to) {
            capture_sq = (board.state.sideToMove == BLACK) ? (to - 8) : (to + 8);
        }
        board.bitboards[board.state.capturedPiece] ^= (1ULL << capture_sq);
        board.occupancy[board.state.sideToMove] ^= (1ULL << capture_sq);
    } else if (flag == Meti::MOVE_CASTLING) {
        int rook_from, rook_to;
        if (to == 6)       { rook_from = 7;  rook_to = 5;  } 
        else if (to == 2)  { rook_from = 0;  rook_to = 3;  } 
        else if (to == 62) { rook_from = 63; rook_to = 61; } 
        else               { rook_from = 56; rook_to = 59; } 

        // In unmake_move, the side that made the move is sideToMove ^ 1
        Colour us = (board.state.sideToMove == WHITE) ? BLACK : WHITE;
        Piece rook = (us == WHITE) ? W_ROOK : B_ROOK;
        uint64_t rook_mask = (1ULL << rook_from) | (1ULL << rook_to);

        board.bitboards[rook] ^= rook_mask;
        board.occupancy[us] ^= rook_mask;
    }
    
    // 3. Restore State (Implicitly restores Zobrist Key, Castling, EP, Turn)
    board.state = previous_state;
    --board.ply;
    board.occupancy[2] = board.occupancy[WHITE] | board.occupancy[BLACK];
}