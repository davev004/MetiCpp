#pragma once
#include <cstdint>
#include "constants.hpp"
#include "types.hpp"

namespace Zobrist {
    // The Hash Arrays (inline prevents linker errors in header-only definitions)
    inline uint64_t pieceKeys[Meti::NUM_PIECES][Meti::NUM_SQUARES];
    inline uint64_t enPassantKeys[8]; // One random number for each file
    inline uint64_t castleKeys[16];   // 16 possible castling right combinations
    inline uint64_t sideKey;          // Black to move

    inline uint64_t next_random(uint64_t& state) {
        state += 0x9E3779B97F4A7C15ULL;
        uint64_t value = state;
        value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31);
    }

    // Called exactly once at engine startup
    inline void init() {
        // We strictly hardcode the seed. 
        // This guarantees the exact same Zobrist keys are generated on every run,
        // which is critical for debugging transpositions later.
        uint64_t state = 1804289383ULL;

        for (int p = 0; p < Meti::NUM_PIECES; p++) {
            for (int s = 0; s < Meti::NUM_SQUARES; s++) {
                pieceKeys[p][s] = next_random(state);
            }
        }

        for (int f = 0; f < 8; f++) {
            enPassantKeys[f] = next_random(state);
        }

        for (int c = 0; c < 16; c++) {
            castleKeys[c] = next_random(state);
        }

        sideKey = next_random(state);
    }
}