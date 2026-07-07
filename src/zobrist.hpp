#pragma once
#include <cstdint>
#include <random>
#include "constants.hpp"
#include "types.hpp"

namespace Zobrist {
    // The Hash Arrays (inline prevents linker errors in header-only definitions)
    inline uint64_t pieceKeys[Meti::NUM_PIECES][Meti::NUM_SQUARES];
    inline uint64_t enPassantKeys[8]; // One random number for each file
    inline uint64_t castleKeys[16];   // 16 possible castling right combinations
    inline uint64_t sideKey;          // Black to move

    // Called exactly once at engine startup
    inline void init() {
        // We strictly hardcode the seed. 
        // This guarantees the exact same Zobrist keys are generated on every run,
        // which is critical for debugging transpositions later.
        std::mt19937_64 rng(1804289383ULL); 
        std::uniform_int_distribution<uint64_t> dist;

        for (int p = 0; p < Meti::NUM_PIECES; p++) {
            for (int s = 0; s < Meti::NUM_SQUARES; s++) {
                pieceKeys[p][s] = dist(rng);
            }
        }

        for (int f = 0; f < 8; f++) {
            enPassantKeys[f] = dist(rng);
        }

        for (int c = 0; c < 16; c++) {
            castleKeys[c] = dist(rng);
        }

        sideKey = dist(rng);
    }
}