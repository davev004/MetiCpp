#pragma once
#include "board.hpp"
#include "move.hpp"

namespace MoveOrdering {

    // The MVV-LVA scoring matrix [Victim][Attacker]
    // Indexed by PieceType (0 to 5) where: P=0, N=1, B=2, R=3, Q=4, K=5
    constexpr int MVV_LVA[6][6] = {
        {105, 104, 103, 102, 101, 100}, // Victim P
        {305, 304, 303, 302, 301, 300}, // Victim N
        {305, 304, 303, 302, 301, 300}, // Victim B
        {505, 504, 503, 502, 501, 500}, // Victim R
        {905, 904, 903, 902, 901, 900}, // Victim Q
        {0,   0,   0,   0,   0,   0  }  // Victim K (Should not happen)
    };

    // Fast, stateless move scorer
    inline int score_move(const Board& board, Meti::Move move) {
        int score = 0;
        Meti::MoveFlag flag = Meti::get_flag(move);
        Piece moving = Meti::get_moving(move);
        int to = Meti::get_to(move);

        // 1. Score Captures (MVV-LVA)
        if (flag == Meti::MOVE_CAPTURE) {
            Piece attacker = moving;
            Piece victim = Meti::get_captured(move);

            // Convert raw Piece (0-11) to PieceType (0-5) branchlessly
            int a_type = attacker >= 6 ? attacker - 6 : attacker;
            int v_type = victim >= 6 ? victim - 6 : victim;

            // Offset by 10000 to guarantee captures are always sorted before quiet moves
            score = 10000 + MVV_LVA[v_type][a_type];
        }

        // 2. Score Promotions (Queen promotion is basically a massive material gain)
        if ((moving == W_PAWN || moving == B_PAWN) && (to < 8 || to > 55)) {
            if (Meti::get_prom(move) == Meti::PROMOTION_QUEEN) {
                score += 9000;
            } else {
                score += 3000; // Knights/Bishops/Rooks
            }
        }

        return score;
    }

    // Zero-allocation stack sort (Insertion Sort is highly cache-friendly for small arrays < 256)
    inline void sort_moves(const Board& board, Meti::MoveList& list) {
        int scores[256];
        
        // Score all moves
        for (int i = 0; i < list.count; i++) {
            scores[i] = score_move(board, list.moves[i]);
        }

        // Sort them descending
        for (int i = 1; i < list.count; i++) {
            int key_score = scores[i];
            Meti::Move key_move = list.moves[i];
            int j = i - 1;

            while (j >= 0 && scores[j] < key_score) {
                scores[j + 1] = scores[j];
                list.moves[j + 1] = list.moves[j];
                j--;
            }
            scores[j + 1] = key_score;
            list.moves[j + 1] = key_move;
        }
    }
}