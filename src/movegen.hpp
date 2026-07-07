#pragma once
#include "board.hpp"
#include "move.hpp"
#include "attacks.hpp"
#include "threats.hpp"
#include "bitboard.hpp"

namespace MoveGen {
    
    // File and Rank Masks to prevent bit-shifting off the board
    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    constexpr uint64_t FILE_H = 0x8080808080808080ULL;
    constexpr uint64_t RANK_3 = 0x0000000000FF0000ULL;
    constexpr uint64_t RANK_6 = 0x0000FF0000000000ULL;

    template <Colour Us>
    inline void generate_pawns(const Board& board, Meti::MoveList& list) {
        constexpr Colour Them = (Us == WHITE) ? BLACK : WHITE;
        constexpr Piece OurPawn = (Us == WHITE) ? W_PAWN : B_PAWN;
        
        uint64_t pawns = board.bitboards[OurPawn];
        uint64_t empty = ~board.occupancy[2];
        uint64_t enemies = board.occupancy[Them];

        uint64_t single_pushes = 0, double_pushes = 0;
        uint64_t attacks_left = 0, attacks_right = 0;

        // Compile-time evaluation guarantees zero runtime branching
        if constexpr (Us == WHITE) {
            single_pushes = (pawns << 8) & empty;
            // Only pawns that successfully moved to Rank 3 can push again to Rank 4
            double_pushes = ((single_pushes & RANK_3) << 8) & empty;
            
            // Mask out File A and File H before shifting to prevent wrapping around the board
            attacks_left  = ((pawns & ~FILE_A) << 7) & enemies;
            attacks_right = ((pawns & ~FILE_H) << 9) & enemies;
        } else {
            single_pushes = (pawns >> 8) & empty;
            // Only pawns that successfully moved to Rank 6 can push again to Rank 5
            double_pushes = ((single_pushes & RANK_6) >> 8) & empty;
            
            attacks_left  = ((pawns & ~FILE_H) >> 7) & enemies;
            attacks_right = ((pawns & ~FILE_A) >> 9) & enemies;
        }

        // --- Serialization Loop ---
        // (For Phase 0, we are registering the 'To' squares. Full 16-bit Move packing 
        // including 'From' squares and Promotion flags will be wired next).
        
        uint64_t pushes = single_pushes | double_pushes;
        while (pushes) {
            int to_square = Bitboard::pop_lsb(pushes);
            list.add(to_square); // Temporarily storing just the destination
        }

        uint64_t attacks = attacks_left | attacks_right;
        while (attacks) {
            int to_square = Bitboard::pop_lsb(attacks);
            list.add(to_square);
        }
    }

    template <Colour Us>
    inline void generate_knights(const Board& board, Meti::MoveList& list) {
        constexpr Piece OurKnight = (Us == WHITE) ? W_KNIGHT : B_KNIGHT;
        
        uint64_t knights = board.bitboards[OurKnight];
        
        // A knight can land anywhere EXCEPT on a square occupied by our own colour
        uint64_t valid_squares = ~board.occupancy[Us]; 

        while (knights) {
            int from_square = Bitboard::pop_lsb(knights);
            
            // O(1) Array Lookup + Collision Filter
            uint64_t attacks = Attacks::KNIGHT.attacks[from_square] & valid_squares;

            while (attacks) {
                int to_square = Bitboard::pop_lsb(attacks);
                list.add(to_square);
            }
        }
    }
    template <Colour Us>
    inline void generate_kings(const Board& board, Meti::MoveList& list) {
        constexpr Piece OurKing = (Us == WHITE) ? W_KING : B_KING;
        constexpr Colour Enemy = (Us == WHITE) ? BLACK : WHITE;
        
        // __builtin_ctzll safely extracts the single King's square index in one CPU cycle
        int from_square = __builtin_ctzll(board.bitboards[OurKing]); 
        
        // 1. Standard Step Attacks
        uint64_t valid_squares = ~board.occupancy[Us]; 
        uint64_t attacks = Attacks::KING.attacks[from_square] & valid_squares;

        while (attacks) {
            int to_square = Bitboard::pop_lsb(attacks);
            list.add(to_square);
        }

        // 2. Castling
        auto can_castle = [&](uint8_t rights,
                              uint64_t empty_mask,
                              int king_sq,
                              int through_sq1,
                              int through_sq2) {
            return (board.state.castlingRights & rights) &&
                   !(board.occupancy[2] & empty_mask) &&
                   !Threats::is_square_attacked(board, king_sq, Enemy) &&
                   !Threats::is_square_attacked(board, through_sq1, Enemy) &&
                   (through_sq2 < 0 || !Threats::is_square_attacked(board, through_sq2, Enemy));
        };

        if constexpr (Us == WHITE) {
            if (can_castle(Castling::WK, 0x60ULL, 4, 5, -1)) {
                list.add(6); // G1
            }

            if (can_castle(Castling::WQ, 0x0EULL, 4, 3, 2)) {
                list.add(2); // C1
            }
        } else {
            if (can_castle(Castling::BK, 0x6000000000000000ULL, 60, 61, -1)) {
                list.add(62); // G8
            }

            if (can_castle(Castling::BQ, 0x0E00000000000000ULL, 60, 59, 58)) {
                list.add(58); // C8
            }
        }
    }

    template <Colour Us>
    inline void generate_sliders(const Board& board, Meti::MoveList& list) {
        constexpr Piece OurRook   = (Us == WHITE) ? W_ROOK   : B_ROOK;
        constexpr Piece OurBishop = (Us == WHITE) ? W_BISHOP : B_BISHOP;
        constexpr Piece OurQueen  = (Us == WHITE) ? W_QUEEN  : B_QUEEN;

        uint64_t occ = board.occupancy[2];
        uint64_t valid_squares = ~board.occupancy[Us]; // We can capture enemies, but not our own

        // ROOKS (Orthogonal)
        uint64_t rooks = board.bitboards[OurRook] | board.bitboards[OurQueen];
        while (rooks) {
            int sq = Bitboard::pop_lsb(rooks);
            uint64_t attacks = 0;
            attacks |= get_ray_attacks(sq, Attacks::NORTH, occ);
            attacks |= get_ray_attacks(sq, Attacks::SOUTH, occ);
            attacks |= get_ray_attacks(sq, Attacks::EAST,  occ);
            attacks |= get_ray_attacks(sq, Attacks::WEST,  occ);
            
            attacks &= valid_squares;
            while (attacks) list.add(Bitboard::pop_lsb(attacks));
        }

        // BISHOPS (Diagonal)
        uint64_t bishops = board.bitboards[OurBishop] | board.bitboards[OurQueen];
        while (bishops) {
            int sq = Bitboard::pop_lsb(bishops);
            uint64_t attacks = 0;
            attacks |= get_ray_attacks(sq, Attacks::NE, occ);
            attacks |= get_ray_attacks(sq, Attacks::NW, occ);
            attacks |= get_ray_attacks(sq, Attacks::SE, occ);
            attacks |= get_ray_attacks(sq, Attacks::SW, occ);

            attacks &= valid_squares;
            while (attacks) list.add(Bitboard::pop_lsb(attacks));
        }
    }


    // The templated core logic (Branchless colour evaluation)
    template <Colour Us>
    inline void generate_pseudo_legal_moves(const Board& board, Meti::MoveList& list) {
        // We will define constexpr opposing colour at compile time
        constexpr Colour Them = (Us == WHITE) ? BLACK : WHITE;

        // 1. Pawn Pushes & Captures
        generate_pawns<Us>(board, list);
        // 2. Knight Step-Attacks
        generate_knights<Us>(board, list);
        // 3. King Step-Attacks & Castling
        generate_kings<Us>(board, list);
        // 4. Sliding Pieces (Ray-casting for Bishops, Rooks, Queens)
        generate_sliders<Us>(board, list);
    }

    // The runtime router: This is the only if/else branch the CPU has to evaluate.
    inline void generate(const Board& board, Meti::MoveList& list) {
        if (board.state.sideToMove == WHITE) {
            generate_pseudo_legal_moves<WHITE>(board, list);
        } else {
            generate_pseudo_legal_moves<BLACK>(board, list);
        }
    }
}