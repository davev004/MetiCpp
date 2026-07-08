#pragma once
#include <cstdint>
#include "types.hpp"
#include "move.hpp"

namespace TT {
    enum Bound : uint8_t {
        BOUND_NONE,
        BOUND_EXACT,
        BOUND_LOWER,
        BOUND_UPPER
    };

    struct Entry {
        uint64_t key;      // 8 bytes (Stores: Zobrist Key ^ Payload Signature)
        Meti::Move move;   // 4 bytes
        int16_t score;     // 2 bytes
        uint8_t depth;     // 1 byte
        Bound bound;       // 1 byte
    };

    inline Entry* table = nullptr;
    inline uint64_t num_entries = 0;

    inline void allocate(uint64_t megabytes) {
        if (table) delete[] table;
        num_entries = (megabytes * 1024 * 1024) / sizeof(Entry);
        table = new Entry[num_entries](); 
    }

    // Mathematical Signature Generator
    inline uint64_t make_signature(Meti::Move move, int16_t score, uint8_t depth, Bound bound) {
        // Cast everything to uint64_t before shifting to prevent data loss
        return static_cast<uint64_t>(move) ^
               (static_cast<uint64_t>(static_cast<uint16_t>(score)) << 32) ^
               (static_cast<uint64_t>(depth) << 48) ^
               (static_cast<uint64_t>(bound) << 56);
    }

    inline void store(uint64_t key, int depth, int ply, int score, Meti::Move move, Bound bound) {
        Entry& tt_entry = table[key % num_entries];

        uint64_t signature = make_signature(move, static_cast<int16_t>(score), static_cast<uint8_t>(depth), bound);

        // We write the payload data first
        tt_entry.move = move;
        tt_entry.score = static_cast<int16_t>(score);
        tt_entry.depth = static_cast<uint8_t>(depth);
        tt_entry.bound = bound;
        
        // Then we seal it with the XOR signature. 
        // A torn read will almost certainly pull mismatched data vs key.
        tt_entry.key = key ^ signature;
    }

    inline bool probe(uint64_t key, int depth, int ply, int alpha, int beta, int& out_score, Meti::Move& out_move) {
        // Volatile copy forces the compiler to pull the 16 bytes exactly as they sit in memory right now
        volatile Entry temp = table[key % num_entries];
        Entry tt_entry = *const_cast<Entry*>(&temp);

        // Reconstruct the signature from the data we just pulled
        uint64_t signature = make_signature(tt_entry.move, tt_entry.score, tt_entry.depth, tt_entry.bound);

        // If the Key ^ Signature matches our queried Zobrist key, the read was perfectly atomic!
        if ((tt_entry.key ^ signature) == key) {
            out_move = tt_entry.move;

            if (tt_entry.depth >= depth) {
                int score = tt_entry.score;

                if (tt_entry.bound == BOUND_EXACT) {
                    out_score = score;
                    return true;
                }
                if (tt_entry.bound == BOUND_LOWER && score >= beta) {
                    out_score = score;
                    return true;
                }
                if (tt_entry.bound == BOUND_UPPER && score <= alpha) {
                    out_score = score;
                    return true;
                }
            }
        }
        return false;
    }
}