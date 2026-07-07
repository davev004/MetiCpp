#include "move_logic.hpp"
#include "zobrist.hpp"
#include "bitboard.hpp"
#include "constants.hpp"

void make_move(Board& board, Meti::Move move) {
    int from = Meti::get_from(move);
    int to = Meti::get_to(move);
    int flag = Meti::get_flag(move);
    
    // O(1) extraction from the 32-bit move
    Piece moving = Meti::get_moving(move);
    Piece captured = Meti::get_captured(move); 

    // 1. Snapshot State
    board.history[board.ply++] = board.state;
    //clocks
    board.state.halfMoveClock++;
    if (moving == W_PAWN || moving == B_PAWN || captured != PIECE_NONE) {
        board.state.halfMoveClock = 0; // Reset on pawn push or capture
    }

    if (board.state.sideToMove == BLACK) {
        board.state.fullMoveNumber++; // Increment full move after Black plays
    }

    // 2. Mathematical En Passant Capture Square (Branchless)
    // If to = 42 (c6), to ^ 8 = 34 (c5). If to = 18 (c3), to ^ 8 = 26 (c4).
    bool is_en_passant = (flag == Meti::MOVE_CAPTURE && captured == PIECE_NONE);
    int capture_sq = is_en_passant ? (to ^ 8) : to;
    if (is_en_passant) captured = board.mailbox[capture_sq]; // Get the actual pawn

    // 3. Zobrist: Remove moving piece, side-to-move, and old EP/Castle states
    board.state.zobristKey ^= Zobrist::pieceKeys[moving][from];
    board.state.zobristKey ^= Zobrist::sideKey;
    board.state.zobristKey ^= Zobrist::castleKeys[board.state.castlingRights];
    if (board.state.enPassantSquare != Meti::SQ_NONE) 
        board.state.zobristKey ^= Zobrist::enPassantKeys[board.state.enPassantSquare % 8];

    // --- Branchless Castling Rights Update ---
    board.state.castlingRights &= Meti::CASTLE_RIGHTS_UPDATE[from] & Meti::CASTLE_RIGHTS_UPDATE[to];

    board.state.capturedPiece = captured;
    board.state.enPassantSquare = Meti::SQ_NONE; 
    
    // 4. Apply Move (Universal XOR Updates)
    uint64_t from_to_mask = (1ULL << from) | (1ULL << to);
    board.bitboards[moving] ^= from_to_mask;
    
    Colour us = static_cast<Colour>(board.state.sideToMove);
    board.occupancy[us] ^= from_to_mask;
    board.occupancy[2]  ^= from_to_mask; // XOR Rule fixed

    board.mailbox[from] = PIECE_NONE;
    board.mailbox[to] = moving;

    // Handle Capture
    if (captured != PIECE_NONE) {
        board.bitboards[captured] ^= (1ULL << capture_sq);
        board.occupancy[us ^ 1] ^= (1ULL << capture_sq);
        board.occupancy[2] ^= (1ULL << capture_sq); // Remove captured piece from total
        board.mailbox[capture_sq] = (is_en_passant) ? PIECE_NONE : board.mailbox[capture_sq];
        board.state.zobristKey ^= Zobrist::pieceKeys[captured][capture_sq];
    }

    // Handle Special Move Flags
    if (flag == Meti::MOVE_DOUBLE_PUSH) { 
        // Branchless EP square calculation
        board.state.enPassantSquare = to ^ 8; 
    } else if (flag == Meti::MOVE_CASTLING) { 
        int rook_from, rook_to;
        if (to == 6)       { rook_from = 7;  rook_to = 5;  } 
        else if (to == 2)  { rook_from = 0;  rook_to = 3;  } 
        else if (to == 62) { rook_from = 63; rook_to = 61; } 
        else               { rook_from = 56; rook_to = 59; } 

        Piece rook = (us == WHITE) ? W_ROOK : B_ROOK;
        uint64_t rook_mask = (1ULL << rook_from) | (1ULL << rook_to);

        board.bitboards[rook] ^= rook_mask;
        board.occupancy[us] ^= rook_mask;
        board.occupancy[2] ^= rook_mask;
        
        board.mailbox[rook_from] = PIECE_NONE;
        board.mailbox[rook_to] = rook;

        board.state.zobristKey ^= Zobrist::pieceKeys[rook][rook_from];
        board.state.zobristKey ^= Zobrist::pieceKeys[rook][rook_to];
    }

    // 5. Final Zobrist Update
    board.state.zobristKey ^= Zobrist::pieceKeys[moving][to];
    board.state.zobristKey ^= Zobrist::castleKeys[board.state.castlingRights];
    if (board.state.enPassantSquare != Meti::SQ_NONE) 
        board.state.zobristKey ^= Zobrist::enPassantKeys[board.state.enPassantSquare % 8];
    
    board.state.sideToMove ^= 1;

    // If a new EP square was created this turn, hash it in!
    if (board.state.enPassantSquare != Meti::SQ_NONE) {
        board.state.zobristKey ^= Zobrist::enPassantKeys[board.state.enPassantSquare % 8];
    }
    
    board.state.sideToMove ^= 1;
}

void unmake_move(Board& board, Meti::Move move) {
    int from = Meti::get_from(move);
    int to = Meti::get_to(move);
    int flag = Meti::get_flag(move);
    
    Piece moving = Meti::get_moving(move);
    Piece captured = Meti::get_captured(move); 

    // 1. Restore the historical state (Instantly reverts Zobrist, Castling, Turn, & EP)
    board.state = board.history[--board.ply];

    // The side that actually made the move is the newly restored sideToMove
    Colour us = static_cast<Colour>(board.state.sideToMove); 

    // 2. Mathematical En Passant Capture Square
    bool is_en_passant = (flag == Meti::MOVE_CAPTURE && captured == PIECE_NONE);
    int capture_sq = is_en_passant ? (to ^ 8) : to;
    
    // If it was an EP, the move struct doesn't hold the captured piece (it holds NONE). 
    // We must mathematically deduce the captured pawn's colour.
    if (is_en_passant) {
        captured = (us == WHITE) ? B_PAWN : W_PAWN; 
    }

    // If the moving piece was a pawn and it landed on Rank 1 or Rank 8
    if ((moving == W_PAWN || moving == B_PAWN) && ((1ULL << to) & 0xFF000000000000FFULL)) {
        Meti::PromotionPiece prom = Meti::get_prom(move);
        Piece promoted_to = static_cast<Piece>((us == WHITE ? W_KNIGHT : B_KNIGHT) + static_cast<int>(prom));

        // XOR out the Promoted piece, XOR the Pawn back in
        board.bitboards[promoted_to] ^= (1ULL << to);
        board.bitboards[moving] ^= (1ULL << to);
        
        // Zobrist is already handled by the state snapshot, no need to touch it!
    }

    // 3. Revert the primary piece movement (XOR is perfectly symmetrical)
    uint64_t from_to_mask = (1ULL << from) | (1ULL << to);
    board.bitboards[moving] ^= from_to_mask;
    board.occupancy[us] ^= from_to_mask;
    board.occupancy[2]  ^= from_to_mask;

    // Restore Mailbox for the primary piece
    board.mailbox[from] = moving;
    board.mailbox[to] = PIECE_NONE; // Will be safely overwritten below if it was a capture

    // 4. Restore Captured Piece
    if (captured != PIECE_NONE) {
        uint64_t cap_mask = 1ULL << capture_sq;
        board.bitboards[captured] ^= cap_mask;
        board.occupancy[us ^ 1] ^= cap_mask; // Re-add to opponent's occupancy
        board.occupancy[2] ^= cap_mask;      // Re-add to total occupancy
        
        board.mailbox[capture_sq] = captured;
    }

    // 5. Revert Castling Rook Movement
    if (flag == Meti::MOVE_CASTLING) {
        int rook_from, rook_to;
        if (to == 6)       { rook_from = 7;  rook_to = 5;  } // White Kingside
        else if (to == 2)  { rook_from = 0;  rook_to = 3;  } // White Queenside
        else if (to == 62) { rook_from = 63; rook_to = 61; } // Black Kingside
        else               { rook_from = 56; rook_to = 59; } // Black Queenside

        Piece rook = (us == WHITE) ? W_ROOK : B_ROOK;
        uint64_t rook_mask = (1ULL << rook_from) | (1ULL << rook_to);

        board.bitboards[rook] ^= rook_mask;
        board.occupancy[us] ^= rook_mask;
        board.occupancy[2] ^= rook_mask;
        
        board.mailbox[rook_from] = rook;
        board.mailbox[rook_to] = PIECE_NONE;
    }
}