#pragma once
#include <cstdint>

namespace Meti {
    enum MoveFlag : uint16_t {
        MOVE_QUIET = 0,
        MOVE_DOUBLE_PUSH = 1,
        MOVE_CASTLING = 2,
        MOVE_CAPTURE = 3
    };

    enum PromotionPiece : uint16_t {
        PROMOTION_KNIGHT = 0,
        PROMOTION_BISHOP = 1,
        PROMOTION_ROOK = 2,
        PROMOTION_QUEEN = 3
    };

    // Bit-masking schema (24 bits used):
    // Bits 0-5:   From square (0-63)
    // Bits 6-11:  To square (0-63)
    // Bits 12-13: Move flag (00: Quiet, 01: Double Push, 10: Castling, 11: Capture)
    // Bits 14-15: Promotion piece (00: N, 01: B, 10: R, 11: Q)
    // Bits 16-19: Moving Piece (0-11)
    // Bits 20-23: Captured Piece (0-12)
    typedef uint32_t Move;

    constexpr uint32_t FROM_MASK = 0x3F;
    constexpr uint32_t TO_SHIFT = 6;
    constexpr uint32_t TO_MASK = 0x3F << TO_SHIFT;
    constexpr uint32_t FLAG_SHIFT = 12;
    constexpr uint32_t FLAG_MASK = 0x3 << FLAG_SHIFT;
    constexpr uint32_t PROM_SHIFT = 14;
    constexpr uint32_t PROM_MASK = 0x3 << PROM_SHIFT;
    constexpr uint32_t MOVING_SHIFT = 16;
    constexpr uint32_t MOVING_MASK = 0xF << MOVING_SHIFT;
    constexpr uint32_t CAPTURED_SHIFT = 20;
    constexpr uint32_t CAPTURED_MASK = 0xF << CAPTURED_SHIFT;

    // Extractors (These compile to simple bit-shifts/masks, zero function overhead)
    inline int get_from(Move m) { return m & FROM_MASK; }
    inline int get_to(Move m)   { return (m & TO_MASK) >> TO_SHIFT; }
    inline MoveFlag get_flag(Move m) { return static_cast<MoveFlag>((m & FLAG_MASK) >> FLAG_SHIFT); }
    inline PromotionPiece get_prom(Move m) { return static_cast<PromotionPiece>((m & PROM_MASK) >> PROM_SHIFT); }
    inline Piece get_moving(Move m) { return static_cast<Piece>((m & MOVING_MASK) >> MOVING_SHIFT); }
    inline Piece get_captured(Move m) { return static_cast<Piece>((m & CAPTURED_MASK) >> CAPTURED_SHIFT); }

    // Packer
    inline Move create_move(int from, int to, Piece moving, Piece captured = PIECE_NONE, MoveFlag flag = MOVE_QUIET, PromotionPiece prom = PROMOTION_KNIGHT) {
        return (from) | (to << TO_SHIFT) | (flag << FLAG_SHIFT) | (prom << PROM_SHIFT) | (moving << MOVING_SHIFT) | (captured << CAPTURED_SHIFT);
    }

    // Converts a square index (0 = a1, 63 = h8) into a coordinate string.
    // The caller provides a 3-byte buffer: [file, rank, '\0'].
    inline void square_to_coord(int square, char out[3]) {
        out[0] = static_cast<char>('a' + (square & 7));
        out[1] = static_cast<char>('1' + (square >> 3));
        out[2] = '\0';
    }



    // A pre-allocated list to guarantee zero dynamic memory allocation
    struct MoveList {
        Move moves[256]; // 256 is the absolute mathematical maximum of possible moves in a chess position
        int count = 0;

        inline void add(Move move) {
            moves[count++] = move;
        }
    };
}