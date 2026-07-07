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
    constexpr uint64_t RANK_1 = 0x00000000000000FFULL;
    constexpr uint64_t RANK_8 = 0xFF00000000000000ULL;

    inline void add_promotion_moves(Meti::MoveList& list, int from_square, int to_square, Meti::MoveFlag flag) {
        list.add(Meti::create_move(from_square, to_square, flag, Meti::PROMOTION_KNIGHT));
        list.add(Meti::create_move(from_square, to_square, flag, Meti::PROMOTION_BISHOP));
        list.add(Meti::create_move(from_square, to_square, flag, Meti::PROMOTION_ROOK));
        list.add(Meti::create_move(from_square, to_square, flag, Meti::PROMOTION_QUEEN));
    }

    template <Colour Us>
    inline void generate_pawns(const Board& board, Meti::MoveList& list) {
        constexpr Colour Them = (Us == WHITE) ? BLACK : WHITE;
        constexpr Piece OurPawn = (Us == WHITE) ? W_PAWN : B_PAWN;
        constexpr uint64_t PromotionRank = (Us == WHITE) ? RANK_8 : RANK_1;
        
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

        auto add_pawn_move = [&](int from_square, int to_square, Meti::MoveFlag flag) {
            list.add(Meti::create_move(from_square, to_square, flag));
        };

        if (board.state.enPassantSquare != Meti::SQ_NONE) {
            int ep_square = board.state.enPassantSquare;
            uint64_t ep_mask = 1ULL << ep_square;

            if constexpr (Us == WHITE) {
                uint64_t ep_sources = 0;
                if ((ep_mask & ~FILE_H) != 0) ep_sources |= (ep_mask >> 7) & pawns;
                if ((ep_mask & ~FILE_A) != 0) ep_sources |= (ep_mask >> 9) & pawns;

                while (ep_sources) {
                    int from_square = Bitboard::pop_lsb(ep_sources);
                    add_pawn_move(from_square, ep_square, Meti::MOVE_CAPTURE);
                }
            } else {
                uint64_t ep_sources = 0;
                if ((ep_mask & ~FILE_A) != 0) ep_sources |= (ep_mask << 7) & pawns;
                if ((ep_mask & ~FILE_H) != 0) ep_sources |= (ep_mask << 9) & pawns;

                while (ep_sources) {
                    int from_square = Bitboard::pop_lsb(ep_sources);
                    add_pawn_move(from_square, ep_square, Meti::MOVE_CAPTURE);
                }
            }
        }

        while (single_pushes) {
            int to_square = Bitboard::pop_lsb(single_pushes);
            int from_square = (Us == WHITE) ? (to_square - 8) : (to_square + 8);
            if (Bitboard::check_bit(PromotionRank, to_square)) {
                add_promotion_moves(list, from_square, to_square, Meti::MOVE_QUIET);
            } else {
                add_pawn_move(from_square, to_square, Meti::MOVE_QUIET);
            }
        }

        while (double_pushes) {
            int to_square = Bitboard::pop_lsb(double_pushes);
            int from_square = (Us == WHITE) ? (to_square - 16) : (to_square + 16);
            add_pawn_move(from_square, to_square, Meti::MOVE_DOUBLE_PUSH);
        }

        while (attacks_left) {
            int to_square = Bitboard::pop_lsb(attacks_left);
            int from_square = (Us == WHITE) ? (to_square - 7) : (to_square + 7);
            if (Bitboard::check_bit(pawns, from_square)) {
                if (Bitboard::check_bit(PromotionRank, to_square)) {
                    add_promotion_moves(list, from_square, to_square, Meti::MOVE_CAPTURE);
                } else {
                    add_pawn_move(from_square, to_square, Meti::MOVE_CAPTURE);
                }
            }
        }

        while (attacks_right) {
            int to_square = Bitboard::pop_lsb(attacks_right);
            int from_square = (Us == WHITE) ? (to_square - 9) : (to_square + 9);
            if (Bitboard::check_bit(pawns, from_square)) {
                if (Bitboard::check_bit(PromotionRank, to_square)) {
                    add_promotion_moves(list, from_square, to_square, Meti::MOVE_CAPTURE);
                } else {
                    add_pawn_move(from_square, to_square, Meti::MOVE_CAPTURE);
                }
            }
        }
    }

    template <Colour Us>
    inline void generate_knights(const Board& board, Meti::MoveList& list) {
        constexpr Piece OurKnight = (Us == WHITE) ? W_KNIGHT : B_KNIGHT;
        constexpr Colour Them = (Us == WHITE) ? BLACK : WHITE;
        
        uint64_t knights = board.bitboards[OurKnight];
        
        // A knight can land anywhere EXCEPT on a square occupied by our own colour
        uint64_t valid_squares = ~board.occupancy[Us]; 

        while (knights) {
            int from_square = Bitboard::pop_lsb(knights);
            
            // O(1) Array Lookup + Collision Filter
            uint64_t attacks = Attacks::KNIGHT.attacks[from_square] & valid_squares;

            while (attacks) {
                int to_square = Bitboard::pop_lsb(attacks);
                Meti::MoveFlag flag = Bitboard::check_bit(board.occupancy[Them], to_square)
                    ? Meti::MOVE_CAPTURE
                    : Meti::MOVE_QUIET;
                list.add(Meti::create_move(from_square, to_square, flag));
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
            Meti::MoveFlag flag = Bitboard::check_bit(board.occupancy[Enemy], to_square)
                ? Meti::MOVE_CAPTURE
                : Meti::MOVE_QUIET;
            list.add(Meti::create_move(from_square, to_square, flag));
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
                list.add(Meti::create_move(4, 6, Meti::MOVE_CASTLING)); // E1 -> G1
            }

            if (can_castle(Castling::WQ, 0x0EULL, 4, 3, 2)) {
                list.add(Meti::create_move(4, 2, Meti::MOVE_CASTLING)); // E1 -> C1
            }
        } else {
            if (can_castle(Castling::BK, 0x6000000000000000ULL, 60, 61, -1)) {
                list.add(Meti::create_move(60, 62, Meti::MOVE_CASTLING)); // E8 -> G8
            }

            if (can_castle(Castling::BQ, 0x0E00000000000000ULL, 60, 59, 58)) {
                list.add(Meti::create_move(60, 58, Meti::MOVE_CASTLING)); // E8 -> C8
            }
        }
    }

    template <Colour Us>
    inline void generate_sliders(const Board& board, Meti::MoveList& list) {
        constexpr Piece OurRook   = (Us == WHITE) ? W_ROOK   : B_ROOK;
        constexpr Piece OurBishop = (Us == WHITE) ? W_BISHOP : B_BISHOP;
        constexpr Piece OurQueen  = (Us == WHITE) ? W_QUEEN  : B_QUEEN;
        constexpr Colour Them = (Us == WHITE) ? BLACK : WHITE;

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
            while (attacks) {
                int to_square = Bitboard::pop_lsb(attacks);
                Meti::MoveFlag flag = Bitboard::check_bit(board.occupancy[Them], to_square)
                    ? Meti::MOVE_CAPTURE
                    : Meti::MOVE_QUIET;
                list.add(Meti::create_move(sq, to_square, flag));
            }
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
            while (attacks) {
                int to_square = Bitboard::pop_lsb(attacks);
                Meti::MoveFlag flag = Bitboard::check_bit(board.occupancy[Them], to_square)
                    ? Meti::MOVE_CAPTURE
                    : Meti::MOVE_QUIET;
                list.add(Meti::create_move(sq, to_square, flag));
            }
        }
    }


    // The templated core logic (Branchless colour evaluation)
    template <Colour Us>
    inline void generate_pseudo_legal_moves(const Board& board, Meti::MoveList& list) {
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