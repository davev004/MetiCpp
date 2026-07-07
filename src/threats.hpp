#pragma once
#include "board.hpp"
#include "attacks.hpp"
#include "bitboard.hpp"

namespace Threats {
    
    // Checks if the given square is attacked by the specified colour
    inline bool is_square_attacked(const Board& board, int sq, Colour attacker_color) {
        uint64_t occ = board.occupancy[2];

        // 1. PAWNS (The Reverse Diagonal Check)
        // If Black is attacking, they attack DOWN the board. Therefore, we check 
        // UP the board (North-West and North-East) from the target square.
        uint64_t sq_bb = 1ULL << sq;
        if (attacker_color == WHITE) {
            uint64_t pawn_attacks = ((sq_bb & ~0x0101010101010101ULL) >> 9) | // SW (Mask File A)
                                    ((sq_bb & ~0x8080808080808080ULL) >> 7);  // SE (Mask File H)
            if (pawn_attacks & board.bitboards[W_PAWN]) return true;
        } else {
            uint64_t pawn_attacks = ((sq_bb & ~0x8080808080808080ULL) << 9) | // NE (Mask File H)
                                    ((sq_bb & ~0x0101010101010101ULL) << 7);  // NW (Mask File A)
            if (pawn_attacks & board.bitboards[B_PAWN]) return true;
        }

        // 2. KNIGHTS
        Piece attacker_knight = (attacker_color == WHITE) ? W_KNIGHT : B_KNIGHT;
        if (Attacks::KNIGHT.attacks[sq] & board.bitboards[attacker_knight]) return true;

        // 3. KINGS (Needed for validating legal King moves)
        Piece attacker_king = (attacker_color == WHITE) ? W_KING : B_KING;
        if (Attacks::KING.attacks[sq] & board.bitboards[attacker_king]) return true;

        // 4. BISHOPS & QUEENS (Diagonal Rays)
        Piece attacker_bishop = (attacker_color == WHITE) ? W_BISHOP : B_BISHOP;
        Piece attacker_queen  = (attacker_color == WHITE) ? W_QUEEN  : B_QUEEN;
        uint64_t diagonal_attackers = board.bitboards[attacker_bishop] | board.bitboards[attacker_queen];
        
        if (diagonal_attackers) {
            uint64_t attacks = 0;
            attacks |= Attacks::get_ray_attacks(sq, Attacks::NE, occ);
            attacks |= Attacks::get_ray_attacks(sq, Attacks::NW, occ);
            attacks |= Attacks::get_ray_attacks(sq, Attacks::SE, occ);
            attacks |= Attacks::get_ray_attacks(sq, Attacks::SW, occ);
            if (attacks & diagonal_attackers) return true;
        }

        // 5. ROOKS & QUEENS (Orthogonal Rays)
        Piece attacker_rook = (attacker_color == WHITE) ? W_ROOK : B_ROOK;
        uint64_t orthogonal_attackers = board.bitboards[attacker_rook] | board.bitboards[attacker_queen];
        
        if (orthogonal_attackers) {
            uint64_t attacks = 0;
            attacks |= Attacks::get_ray_attacks(sq, Attacks::NORTH, occ);
            attacks |= Attacks::get_ray_attacks(sq, Attacks::SOUTH, occ);
            attacks |= Attacks::get_ray_attacks(sq, Attacks::EAST,  occ);
            attacks |= Attacks::get_ray_attacks(sq, Attacks::WEST,  occ);
            if (attacks & orthogonal_attackers) return true;
        }

        return false; // Square is perfectly safe
    }
}