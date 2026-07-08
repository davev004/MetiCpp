#pragma once
#include "board.hpp"
#include "attacks.hpp"
#include "bitboard.hpp"

namespace Threats {
    
    // Checks if the given square is attacked by the specified colour
    template <Colour Attacker>
    inline bool is_square_attacked(const Board& board, int sq) {
        uint64_t occ = board.occupancy[2];
        uint64_t sq_bb = 1ULL << sq;

        // 1. PAWNS (Compile-time evaluation)
        if constexpr (Attacker == WHITE) {
            uint64_t pawn_attacks = ((sq_bb & ~0x0101010101010101ULL) >> 9) | 
                                    ((sq_bb & ~0x8080808080808080ULL) >> 7);
            if (pawn_attacks & board.bitboards[W_PAWN]) return true;
        } else {
            uint64_t pawn_attacks = ((sq_bb & ~0x8080808080808080ULL) << 9) | 
                                    ((sq_bb & ~0x0101010101010101ULL) << 7);
            if (pawn_attacks & board.bitboards[B_PAWN]) return true;
        }

        // 2. KNIGHTS
        constexpr Piece knight = (Attacker == WHITE) ? W_KNIGHT : B_KNIGHT;
        if (Attacks::KNIGHT.attacks[sq] & board.bitboards[knight]) return true;

        // 3. KINGS
        constexpr Piece king = (Attacker == WHITE) ? W_KING : B_KING;
        if (Attacks::KING.attacks[sq] & board.bitboards[king]) return true;

        // 4. BISHOPS & QUEENS (Using our newly optimised composite getters)
        constexpr Piece bishop = (Attacker == WHITE) ? W_BISHOP : B_BISHOP;
        constexpr Piece queen  = (Attacker == WHITE) ? W_QUEEN  : B_QUEEN;
        
        uint64_t diagonal_attackers = board.bitboards[bishop] | board.bitboards[queen];
        if (diagonal_attackers && (Attacks::get_bishop_attacks(sq, occ) & diagonal_attackers)) return true;

        // 5. ROOKS & QUEENS
        constexpr Piece rook = (Attacker == WHITE) ? W_ROOK : B_ROOK;
        uint64_t orthogonal_attackers = board.bitboards[rook] | board.bitboards[queen];
        
        if (orthogonal_attackers && (Attacks::get_rook_attacks(sq, occ) & orthogonal_attackers)) return true;

        return false;
    }
}