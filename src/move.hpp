#pragma once
#include <cstdint>

namespace Meti {
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
    inline int get_flag(Move m) { return (m & FLAG_MASK) >> FLAG_SHIFT; }
    inline int get_prom(Move m) { return (m & PROM_MASK) >> PROM_SHIFT; }

    // Packer
    inline Move create_move(int from, int to, int flag = 0, int prom = 0) {
        return (from) | (to << TO_SHIFT) | (flag << FLAG_SHIFT) | (prom << PROM_SHIFT);
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