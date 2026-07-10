#pragma once
#include "board.hpp"
#include "types.hpp"



    namespace Eval {

    // Piece values (Centipawns)
    constexpr int VALUE_P = 100;
    constexpr int VALUE_N = 320;
    constexpr int VALUE_B = 330;
    constexpr int VALUE_R = 500;
    constexpr int VALUE_Q = 900;

    // LERF Mapped Knight PST (Index 0 is a1, Index 63 is h8)
    constexpr int PST_KNIGHT[64] = {
        -50, -40, -30, -30, -30, -30, -40, -50,  // Rank 1 (Indices 0-7)   - White's back rank
        -40, -20,   0,   5,   5,   0, -20, -40,  // Rank 2 (Indices 8-15)
        -30,   5,  10,  15,  15,  10,   5, -30,  // Rank 3 (Indices 16-23) - Natural development (c3/f3)
        -30,   0,  15,  20,  20,  15,   0, -30,  // Rank 4 (Indices 24-31) - Strong central control
        -30,   5,  15,  20,  20,  15,   5, -30,  // Rank 5 (Indices 32-39)
        -30,   0,  10,  15,  15,  10,   0, -30,  // Rank 6 (Indices 40-47)
        -40, -20,   0,   0,   0,   0, -20, -40,  // Rank 7 (Indices 48-55)
        -50, -40, -30, -30, -30, -30, -40, -50   // Rank 8 (Indices 56-63)  - Black's back rank
    };

    // Pawn PST: Encourages advancing and controlling the center
    // LERF Mapped Pawn PST (Index 0 is a1, Index 63 is h8)
    constexpr int PST_PAWN[64] = {
          0,   0,   0,   0,   0,   0,   0,   0,  // Rank 1 (Indices 0-7)
          5,  10,  -20, -5, -5,  10,  10,   5,  // Rank 2 (Indices 8-15) - discourage e2/d2 slightly to open bishop/queen lanes
          5,  -5, -10,   0,   0, -10,  -5,   5,  // Rank 3 (Indices 16-23)
          0,   0,   30,  15,  214,   0,   0,   0,  // Rank 4 (Indices 24-31) - Encourage center pushes
          5,   5,  10,  25,  25,  10,   5,   5,  // Rank 5 (Indices 32-39)
         10,  10,  20,  30,  30,  20,  10,  10,  // Rank 6 (Indices 40-47)
         50,  50,  50,  50,  50,  50,  50,  50,  // Rank 7 (Indices 48-55) - Massive reward for getting pawns to the 7th rank!
          0,   0,   0,   0,   0,   0,   0,   0   // Rank 8 (Indices 56-63)
    };
    
    constexpr int PST_BISHOP[64] = {
        -20, -10, -10, -10, -10, -10, -10, -20,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -10,   0,   5,  10,  10,   5,   0, -10,
        -10,   5,   5,  10,  10,   5,   5, -10,
        -10,   0,  10,  10,  10,  10,   0, -10,
        -10,   5,   5,   5,   5,   5,   5, -10,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -20, -10, -10, -10, -10, -10, -10, -20
    };

    constexpr int PST_ROOK[64] = {
        0, 0, 5, 10, 10, 5, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        20, 20, 20, 20, 20, 20, 20, 20,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    constexpr int PST_QUEEN[64] = {
        -20, -10, -10,  -5,  -5, -10, -10, -20,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -10,   0,   5,   5,   5,   5,   0, -10,
         -5,   0,   5,   5,   5,   5,   0,  -5,
          0,   0,   5,   5,   5,   5,   0,  -5,
        -10,   5,   5,   5,   5,   5,   0, -10,
        -10,   0,   5,   0,   0,   0,   0, -10,
        -20, -10, -10,  -5,  -5, -10, -10, -20
    };

    // LERF Mapped King PST (Index 0 is a1, Index 63 is h8)
    constexpr int PST_KING[64] = {
         20,  30,  10,   0,   0,  10,  30,  20,  // Rank 1 (Indices 0-7)   - Safe on g1/c1, okay on h1/a1
         20,  20,   0,   0,   0,   0,  20,  20,  // Rank 2 (Indices 8-15)  - Walking up is dangerous
        -10, -20, -20, -20, -20, -20, -20, -10,  // Rank 3 (Indices 16-23)
        -20, -30, -30, -40, -40, -30, -30, -20,  // Rank 4 (Indices 24-31) - Absolute death zone
        -30, -40, -40, -50, -50, -40, -40, -30,  // Rank 5 (Indices 32-39)
        -30, -40, -40, -50, -50, -40, -40, -30,  // Rank 6 (Indices 40-47)
        -30, -40, -40, -50, -50, -40, -40, -30,  // Rank 7 (Indices 48-55)
        -30, -40, -40, -50, -50, -40, -40, -30   // Rank 8 (Indices 56-63)
    };

    // LERF Mapped King ENDGAME PST (Index 0 is a1, Index 63 is h8)
    constexpr int PST_KING_ENDGAME[64] = {
        -50, -30, -30, -30, -30, -30, -30, -50,  // Rank 1
        -30, -10,   0,   0,   0,   0, -10, -30,  // Rank 2
        -30,   0,  20,  30,  30,  20,   0, -30,  // Rank 3
        -30,   0,  30,  40,  40,  30,   0, -30,  // Rank 4 - Active in the center!
        -30,   0,  30,  40,  40,  30,   0, -30,  // Rank 5 - Active in the center!
        -30,   0,  20,  30,  30,  20,   0, -30,  // Rank 6
        -30, -10,   0,   0,   0,   0, -10, -30,  // Rank 7
        -50, -30, -30, -30, -30, -30, -30, -50   // Rank 8
    };

    // Helper to flip the board for Black
    inline int mirror(int sq) {
        return sq ^ 56; 
    }



    inline int evaluate(const Board& board) {
        // 1. Calculate Game Phase (For Tapered King Evaluation)
        int total_phase = 24; 
        int current_phase = 0;

        current_phase += __builtin_popcountll(board.bitboards[W_KNIGHT] | board.bitboards[B_KNIGHT]) * 1;
        current_phase += __builtin_popcountll(board.bitboards[W_BISHOP] | board.bitboards[B_BISHOP]) * 1;
        current_phase += __builtin_popcountll(board.bitboards[W_ROOK]   | board.bitboards[B_ROOK])   * 2;
        current_phase += __builtin_popcountll(board.bitboards[W_QUEEN]  | board.bitboards[B_QUEEN])  * 4;

        if (current_phase > total_phase) current_phase = total_phase;

        // Scale phase smoothly from 0 (Pure Endgame) to 256 (Pure Middlegame)
        int mg_weight = (current_phase * 256) / total_phase;
        int eg_weight = 256 - mg_weight;

        int mg_score = 0;
        int eg_score = 0;

        // 2. MACRO: Helper to keep our material loops clean
        #define EVAL_PIECE(COLOR, PIECE_TYPE, VALUE, PST, IS_WHITE) \
            { \
                uint64_t bb = board.bitboards[PIECE_TYPE]; \
                while (bb) { \
                    int sq = __builtin_ctzll(bb); \
                    int table_val = IS_WHITE ? PST[sq] : PST[mirror(sq)]; \
                    int val = VALUE + table_val; \
                    if (IS_WHITE) { mg_score += val; eg_score += val; } \
                    else          { mg_score -= val; eg_score -= val; } \
                    bb &= bb - 1; \
                } \
            }

        // --- Evaluate Standard Pieces ---
        EVAL_PIECE(WHITE, W_PAWN,   VALUE_P, PST_PAWN,   true)
        EVAL_PIECE(BLACK, B_PAWN,   VALUE_P, PST_PAWN,   false)
        
        EVAL_PIECE(WHITE, W_KNIGHT, VALUE_N, PST_KNIGHT, true)
        EVAL_PIECE(BLACK, B_KNIGHT, VALUE_N, PST_KNIGHT, false)
        
        EVAL_PIECE(WHITE, W_BISHOP, VALUE_B, PST_BISHOP, true)
        EVAL_PIECE(BLACK, B_BISHOP, VALUE_B, PST_BISHOP, false)
        
        EVAL_PIECE(WHITE, W_ROOK,   VALUE_R, PST_ROOK,   true)
        EVAL_PIECE(BLACK, B_ROOK,   VALUE_R, PST_ROOK,   false)
        
        EVAL_PIECE(WHITE, W_QUEEN,  VALUE_Q, PST_QUEEN,  true)
        EVAL_PIECE(BLACK, B_QUEEN,  VALUE_Q, PST_QUEEN,  false)

        #undef EVAL_PIECE

        // 3. --- Tapered King Evaluation ---
        uint64_t wk = board.bitboards[W_KING];
        if (wk) {
            int sq = __builtin_ctzll(wk);
            mg_score += PST_KING[sq];
            eg_score += PST_KING_ENDGAME[sq];
        }

        uint64_t bk = board.bitboards[B_KING];
        if (bk) {
            int sq = __builtin_ctzll(bk);
            mg_score -= PST_KING[mirror(sq)];
            eg_score -= PST_KING_ENDGAME[mirror(sq)];
        }

        // 4. --- Castling Rights Bonus ---
        // (Assuming standard 1=WK, 2=WQ, 4=BK, 8=BQ in your board state)
        int castle_bonus = 0;
        if (board.state.castlingRights & 1) castle_bonus += 30;
        if (board.state.castlingRights & 2) castle_bonus += 30;
        if (board.state.castlingRights & 4) castle_bonus -= 30;
        if (board.state.castlingRights & 8) castle_bonus -= 30;

        mg_score += castle_bonus;
        eg_score += castle_bonus;

        // 5. --- Final Interpolation ---
        int final_score = ((mg_score * mg_weight) + (eg_score * eg_weight)) / 256;

        // Alpha-Beta requires the score to be from the perspective of the side to move
        return (board.state.sideToMove == WHITE) ? final_score : -final_score;
        
        }
    }