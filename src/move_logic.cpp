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

    // 1. Snapshot State
    board.history[board.ply++] = board.state;

    // 2. Identify Pieces
    Piece moving = get_piece_at(board, from);
    Piece captured = get_piece_at(board, to);

    // 3. Zobrist: Remove moving piece, side-to-move, and old EP/Castle states
    board.state.zobristKey ^= Zobrist::pieceKeys[moving][from];
    board.state.zobristKey ^= Zobrist::sideKey;
    board.state.zobristKey ^= Zobrist::castleKeys[board.state.castlingRights];
    if (board.state.enPassantSquare != Meti::SQ_NONE) 
        board.state.zobristKey ^= Zobrist::enPassantKeys[board.state.enPassantSquare % 8];

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
        board.bitboards[captured] ^= (1ULL << to);
        board.occupancy[us ^ 1] ^= (1ULL << to);
        board.state.zobristKey ^= Zobrist::pieceKeys[captured][to];
    }

    // Handle Special Move Flags
    if (flag == 1) { // Double Push
        board.state.enPassantSquare = (us == WHITE) ? (to - 8) : (to + 8);
    } else if (flag == 2) { // Castling
        // Manually move the rook based on king 'to' square
        // (Implementation omitted for brevity, logic identical to above)
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

    // 1. Revert Move (Bitboard XOR is its own inverse)
    Piece moving = get_piece_at(board, to);
    uint64_t move_mask = (1ULL << from) | (1ULL << to);
    
    board.bitboards[moving] ^= move_mask;
    board.occupancy[board.state.sideToMove ^ 1] ^= move_mask; // Revert occupancy

    // 2. Restore Captured Piece
    if (board.state.capturedPiece != PIECE_NONE) {
        board.bitboards[board.state.capturedPiece] ^= (1ULL << to);
        board.occupancy[board.state.sideToMove] ^= (1ULL << to);
    }

    // 3. Restore State (Implicitly restores Zobrist Key, Castling, EP, Turn)
    board.state = board.history[--board.ply];
    board.occupancy[2] = board.occupancy[WHITE] | board.occupancy[BLACK];
}