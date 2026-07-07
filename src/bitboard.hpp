#pragma once
#include <cstdint>

namespace Bitboard {
    
    // Set a 1 at the given square index (0-63)
    inline void set_bit(uint64_t& bitboard, int square) {
        bitboard |= (1ULL << square);
    }

    // Set a 0 at the given square index
    inline void clear_bit(uint64_t& bitboard, int square) {
        bitboard &= ~(1ULL << square);
    }

    // Check if a bit is 1 at the given square index
    inline bool check_bit(uint64_t bitboard, int square) {
        return (bitboard & (1ULL << square)) != 0;
    }

    // Extracts the index of the Least Significant Bit (LSB) and clears it from the board
    inline int pop_lsb(uint64_t& bitboard) {
        int square = __builtin_ctzll(bitboard);
        bitboard &= bitboard - 1; // Lightning-fast bitwise clear of the LSB
        return square;
    }

    // Extracts the index of the Most Significant Bit (MSB)
    inline int get_msb(uint64_t bitboard) {
        // __builtin_clzll counts leading zeroes from the left. 
        // 63 - leading zeroes gives us the exact square index.
        return 63 - __builtin_clzll(bitboard);
    }

    // Returns the index without clearing the bit
    inline int get_lsb(uint64_t bitboard) {
        return __builtin_ctzll(bitboard);
    }
}