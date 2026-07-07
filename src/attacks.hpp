#pragma once
#include <cstdint>

namespace Attacks {
    // The raw bitwise math for a knight on an empty board. We do thios because knights are colour agnostic and dont care about being blocked. Any knight on a given square can always attack the same 8 squares.
    constexpr uint64_t calculate_knight_attacks(int square) {
        uint64_t b = 1ULL << square;
        uint64_t attacks = 0;

        // The 8 possible L-shapes, masked to prevent wrapping around the board edges
        attacks |= (b << 17) & 0xFEFEFEFEFEFEFEFEULL; // North-North-East (Not File A)
        attacks |= (b << 15) & 0x7F7F7F7F7F7F7F7FULL; // North-North-West (Not File H)
        attacks |= (b << 10) & 0xFCFCFCFCFCFCFCFCULL; // East-North-East (Not Files A, B)
        attacks |= (b <<  6) & 0x3F3F3F3F3F3F3F3FULL; // West-North-West (Not Files G, H)
        attacks |= (b >> 15) & 0xFEFEFEFEFEFEFEFEULL; // South-South-East (Not File A)
        attacks |= (b >> 17) & 0x7F7F7F7F7F7F7F7FULL; // South-South-West (Not File H)
        attacks |= (b >>  6) & 0xFCFCFCFCFCFCFCFCULL; // East-South-East (Not Files A, B)
        attacks |= (b >> 10) & 0x3F3F3F3F3F3F3F3FULL; // West-South-West (Not Files G, H)

        return attacks;
    }

    // The Compile-Time Generation Table
    struct KnightTable {
        uint64_t attacks[64];
        constexpr KnightTable() : attacks{0} {
            for (int i = 0; i < 64; ++i) {
                attacks[i] = calculate_knight_attacks(i);
            }
        }
    };

    // This array is baked into the executable memory at compile-time. Zero runtime cost.
    constexpr KnightTable KNIGHT = KnightTable();
    // And for kings. We mask out squares occupied by our own pieces later.
    // The raw bitwise math for a king on an empty board
    constexpr uint64_t calculate_king_attacks(int square) {
        uint64_t b = 1ULL << square;
        uint64_t attacks = 0;

        attacks |= (b << 8); // North
        attacks |= (b >> 8); // South
        attacks |= (b << 1) & 0xFEFEFEFEFEFEFEFEULL; // East (Mask out File A)
        attacks |= (b >> 1) & 0x7F7F7F7F7F7F7F7FULL; // West (Mask out File H)
        attacks |= (b << 9) & 0xFEFEFEFEFEFEFEFEULL; // North-East
        attacks |= (b << 7) & 0x7F7F7F7F7F7F7F7FULL; // North-West
        attacks |= (b >> 7) & 0xFEFEFEFEFEFEFEFEULL; // South-East
        attacks |= (b >> 9) & 0x7F7F7F7F7F7F7F7FULL; // South-West

        return attacks;
    }

    struct KingTable {
        uint64_t attacks[64];
        constexpr KingTable() : attacks{0} {
            for (int i = 0; i < 64; ++i) {
                attacks[i] = calculate_king_attacks(i);
            }
        }
    };

    // Baked into the executable memory at compile-time.
    constexpr KingTable KING = KingTable();

    // The Ray-Casting Table for sliding pieces (Bishops, Rooks, Queens)
    enum Direction { NORTH, SOUTH, EAST, WEST, NE, NW, SE, SW };

    constexpr uint64_t calculate_ray(int square, Direction dir) {
        uint64_t ray = 0;
        uint64_t b = 1ULL << square;

        while (true) {
            switch (dir) {
                case NORTH: b = (b << 8); break;
                case SOUTH: b = (b >> 8); break;
                case EAST:  b = (b << 1) & 0xFEFEFEFEFEFEFEFEULL; break; // Mask File A
                case WEST:  b = (b >> 1) & 0x7F7F7F7F7F7F7F7FULL; break; // Mask File H
                case NE:    b = (b << 9) & 0xFEFEFEFEFEFEFEFEULL; break;
                case NW:    b = (b << 7) & 0x7F7F7F7F7F7F7F7FULL; break;
                case SE:    b = (b >> 7) & 0xFEFEFEFEFEFEFEFEULL; break;
                case SW:    b = (b >> 9) & 0x7F7F7F7F7F7F7F7FULL; break;
            }
            if (b == 0) break; // Off the board
            ray |= b;
        }
        return ray;
    }

    struct RayTable {
        uint64_t rays[8][64];
        constexpr RayTable() : rays{0} {
            for (int sq = 0; sq < 64; ++sq) {
                rays[NORTH][sq] = calculate_ray(sq, NORTH);
                rays[SOUTH][sq] = calculate_ray(sq, SOUTH);
                rays[EAST][sq]  = calculate_ray(sq, EAST);
                rays[WEST][sq]  = calculate_ray(sq, WEST);
                rays[NE][sq]    = calculate_ray(sq, NE);
                rays[NW][sq]    = calculate_ray(sq, NW);
                rays[SE][sq]    = calculate_ray(sq, SE);
                rays[SW][sq]    = calculate_ray(sq, SW);
            }
        }
    };

    constexpr RayTable RAYS = RayTable();


    // Moved from MoveGen to be globally accessible
    inline uint64_t get_ray_attacks(int sq, Direction dir, uint64_t occ) {
        uint64_t ray = RAYS.rays[dir][sq];
        uint64_t blockers = ray & occ;

        if (blockers) {
            int blocker_sq;
            if (dir == NORTH || dir == EAST || dir == NE || dir == NW) {
                blocker_sq = __builtin_ctzll(blockers); // Positive Ray (LSB)
            } else {
                blocker_sq = 63 - __builtin_clzll(blockers); // Negative Ray (MSB)
            }
            ray ^= RAYS.rays[dir][blocker_sq];
        }
        return ray;
    }
    // Composite Attack Generators for Sliders
    inline uint64_t get_rook_attacks(int sq, uint64_t occ) {
        return get_ray_attacks(sq, NORTH, occ) |
               get_ray_attacks(sq, SOUTH, occ) |
               get_ray_attacks(sq, EAST, occ)  |
               get_ray_attacks(sq, WEST, occ);
    }

    inline uint64_t get_bishop_attacks(int sq, uint64_t occ) {
        return get_ray_attacks(sq, NE, occ) |
               get_ray_attacks(sq, NW, occ) |
               get_ray_attacks(sq, SE, occ) |
               get_ray_attacks(sq, SW, occ);
    }

    inline uint64_t get_queen_attacks(int sq, uint64_t occ) {
        return get_rook_attacks(sq, occ) | get_bishop_attacks(sq, occ);
    }
}