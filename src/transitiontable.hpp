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

    constexpr int MATE_BOUND = 29000; 

    inline void store(uint64_t key, int depth, int ply, int score, Meti::Move move, Bound bound) {
        Entry& tt_entry = table[key % num_entries];

        // Convert score to be independent of the current search depth
        int16_t stored_score = score;
        if (score > MATE_BOUND) stored_score += ply;
        else if (score < -MATE_BOUND) stored_score -= ply;

        uint64_t signature = make_signature(move, stored_score, static_cast<uint8_t>(depth), bound);

        tt_entry.move = move;
        tt_entry.score = stored_score;
        tt_entry.depth = static_cast<uint8_t>(depth);
        tt_entry.bound = bound;
        tt_entry.key = key ^ signature;
    }

    inline bool probe(uint64_t key, int depth, int ply, int alpha, int beta, int& out_score, Meti::Move& out_move) {
        volatile Entry temp = table[key % num_entries];
        Entry tt_entry = *const_cast<Entry*>(&temp);
        uint64_t signature = make_signature(tt_entry.move, tt_entry.score, tt_entry.depth, tt_entry.bound);

        if ((tt_entry.key ^ signature) == key) {
            out_move = tt_entry.move;

            bool is_mate_score = (tt_entry.score > MATE_BOUND || tt_entry.score < -MATE_BOUND);

            if (tt_entry.depth >= depth || is_mate_score) {
                int score = tt_entry.score;

                // Re-adjust mate score relative to current ply
                if (score > MATE_BOUND) score -= ply;
                else if (score < -MATE_BOUND) score += ply;

                if (tt_entry.bound == BOUND_EXACT) { out_score = score; return true; }
                if (tt_entry.bound == BOUND_LOWER && score >= beta) { out_score = score; return true; }
                if (tt_entry.bound == BOUND_UPPER && score <= alpha) { out_score = score; return true; }
            }
        }
        return false;
    }
}