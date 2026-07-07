#pragma once
#include <cstdint>

// Strongly-typed enums for array indexing
enum Colour : uint8_t { WHITE = 0, BLACK = 1 };

enum Piece : uint8_t {
    W_PAWN = 0, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN,     B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NONE
};

namespace Castling {
    constexpr uint8_t WK = 0b0001; // 1: White Kingside
    constexpr uint8_t WQ = 0b0010; // 2: White Queenside
    constexpr uint8_t BK = 0b0100; // 4: Black Kingside
    constexpr uint8_t BQ = 0b1000; // 8: Black Queenside

    constexpr uint8_t ALL  = WK | WQ | BK | BQ; // 0b1111 (All rights available)
    constexpr uint8_t NONE = 0b0000;            // No rights available
}

// The packed GameState (Transient History Only)
struct GameState {
    uint64_t zobristKey;        // 8 bytes
    uint16_t fullMoveNumber;    // 2 bytes
    uint8_t castlingRights;     // 1 byte  (4 bits used) ()
    uint8_t enPassantSquare;    // 1 byte  (0-63, or 64 for none)
    uint8_t sideToMove;         // 1 byte  (colour enum)
    uint8_t halfMoveClock;      // 1 byte  (50-move rule tracker)
    uint8_t capturedPiece;      // 1 byte  (Piece enum)
    
    // Total size: 15 bytes (Compiler will pad to 16 bytes for ultra-fast copying)
};