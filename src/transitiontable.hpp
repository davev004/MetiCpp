#pragma once
#include <cstdint>
#include <atomic>
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
        std::atomic<uint64_t> key{0};
        std::atomic<uint64_t> payload{0};
    };

    inline Entry* table = nullptr;
    inline uint64_t num_entries = 0;

    inline void allocate(uint64_t megabytes) {
        if (table) delete[] table;
        num_entries = (megabytes * 1024 * 1024) / sizeof(Entry);
        table = new Entry[num_entries](); 
    }

    // Mathematical Signature Generator
    inline uint64_t pack(Meti::Move move, int16_t score, uint8_t depth, Bound bound) {
        return static_cast<uint64_t>(move) |
               (static_cast<uint64_t>(static_cast<uint16_t>(score)) << 24) |
               (static_cast<uint64_t>(depth) << 40) |
               (static_cast<uint64_t>(bound) << 48);
    }

    inline Meti::Move unpack_move(uint64_t payload) { return static_cast<Meti::Move>(payload & 0xFFFFFFULL); }
    inline int16_t unpack_score(uint64_t payload) { return static_cast<int16_t>((payload >> 24) & 0xFFFFULL); }
    inline uint8_t unpack_depth(uint64_t payload) { return static_cast<uint8_t>((payload >> 40) & 0xFFULL); }
    inline Bound unpack_bound(uint64_t payload) { return static_cast<Bound>((payload >> 48) & 0xFFULL); }

    constexpr int MATE_BOUND = 29000; 

    inline void store(uint64_t key, int depth, int ply, int score, Meti::Move move, Bound bound) {
        if (table == nullptr || num_entries == 0) return;
        Entry& tt_entry = table[key % num_entries];

        // Convert score to be independent of the current search depth
        int16_t stored_score = score;
        if (score > MATE_BOUND) stored_score += ply;
        else if (score < -MATE_BOUND) stored_score -= ply;

        uint64_t payload = pack(move, stored_score, static_cast<uint8_t>(depth), bound);
        tt_entry.payload.store(payload, std::memory_order_relaxed);
        tt_entry.key.store(key ^ payload, std::memory_order_release);
    }

    inline bool probe(uint64_t key, int depth, int ply, int alpha, int beta, int& out_score, Meti::Move& out_move) {
        if (table == nullptr || num_entries == 0) return false;
        Entry& tt_entry = table[key % num_entries];
        uint64_t encoded_key = tt_entry.key.load(std::memory_order_acquire);
        uint64_t payload = tt_entry.payload.load(std::memory_order_relaxed);
        uint64_t confirmed_key = tt_entry.key.load(std::memory_order_acquire);

        if (encoded_key == confirmed_key && (encoded_key ^ payload) == key) {
            Meti::Move move = unpack_move(payload);
            int16_t stored_score = unpack_score(payload);
            uint8_t stored_depth = unpack_depth(payload);
            Bound bound = unpack_bound(payload);
            out_move = move;

            bool is_mate_score = (stored_score > MATE_BOUND || stored_score < -MATE_BOUND);

            if (stored_depth >= depth || is_mate_score) {
                int score = stored_score;

                // Re-adjust mate score relative to current ply
                if (score > MATE_BOUND) score -= ply;
                else if (score < -MATE_BOUND) score += ply;

                if (bound == BOUND_EXACT) { out_score = score; return true; }
                if (bound == BOUND_LOWER && score >= beta) { out_score = score; return true; }
                if (bound == BOUND_UPPER && score <= alpha) { out_score = score; return true; }
            }
        }
        return false;
    }

    inline bool peek_move(uint64_t key, Meti::Move& out_move) {
        if (table == nullptr || num_entries == 0) return false;
        Entry& tt_entry = table[key % num_entries];
        uint64_t encoded_key = tt_entry.key.load(std::memory_order_acquire);
        uint64_t payload = tt_entry.payload.load(std::memory_order_relaxed);
        uint64_t confirmed_key = tt_entry.key.load(std::memory_order_acquire);
        if (encoded_key != confirmed_key || (encoded_key ^ payload) != key) return false;
        out_move = unpack_move(payload);
        return out_move != 0;
    }
}