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

    // Bit-masking schema:
    // Bits 0-5:   From square (0-63)
    // Bits 6-11:  To square (0-63)
    // Bits 12-13: Move flag (00: Quiet, 01: Double Push, 10: Castling, 11: Capture)
    // Bits 14-15: Promotion piece (00: N, 01: B, 10: R, 11: Q)
    typedef uint16_t Move;
    
    constexpr uint16_t FROM_MASK = 0x3F;
    constexpr uint16_t TO_SHIFT = 6;
    constexpr uint16_t TO_MASK = 0x3F << TO_SHIFT;
    constexpr uint16_t FLAG_SHIFT = 12;
    constexpr uint16_t FLAG_MASK = 0x3 << FLAG_SHIFT;
    constexpr uint16_t PROM_SHIFT = 14;
    constexpr uint16_t PROM_MASK = 0x3 << PROM_SHIFT;

    // Extractors (These compile to simple bit-shifts/masks, zero function overhead)
    inline int get_from(Move m) { return m & FROM_MASK; }
    inline int get_to(Move m)   { return (m & TO_MASK) >> TO_SHIFT; }
    inline MoveFlag get_flag(Move m) { return static_cast<MoveFlag>((m & FLAG_MASK) >> FLAG_SHIFT); }
    inline PromotionPiece get_prom(Move m) { return static_cast<PromotionPiece>((m & PROM_MASK) >> PROM_SHIFT); }

    // Packer
    inline Move create_move(int from, int to, MoveFlag flag = MOVE_QUIET, PromotionPiece prom = PROMOTION_KNIGHT) {
        return (from) | (to << TO_SHIFT) | (flag << FLAG_SHIFT) | (prom << PROM_SHIFT);
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