#include <iostream>
#include "bitboard.hpp"
#include "board.hpp"

namespace Debug {

inline void print_board(const Board& board) {
    // Maps exactly to our Piece enum (0 to 11)
    const char piece_chars[] = "PNBRQKpnbrqk";

    std::cout << "\n";
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << rank + 1 << "  "; // Print rank labels (8 down to 1)
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            char sq_char = '.';

            // Check all 12 bitboards for a piece on this square
            for (int p = 0; p < Meti::NUM_PIECES; p++) {
                if (Bitboard::check_bit(board.bitboards[p], square)) {
                    sq_char = piece_chars[p];
                    break;
                }
            }
            std::cout << sq_char << " ";
        }
        std::cout << "\n";
    }
    std::cout << "   a b c d e f g h\n\n";
    
    // Optional: Print the Zobrist Key to verify hashing is working
    std::cout << "Zobrist Key: " << std::hex << board.state.zobristKey << std::dec << "\n";

    
}

inline void print_board_state(const Board& board) {
        std::cout << "\n--- BOARD STATE DIAGNOSTIC ---" << std::endl;
        std::cout << "Side to move: " << (board.state.sideToMove == WHITE ? "WHITE" : "BLACK") << std::endl;
        
        // Check for garbage values in bitboards
        std::cout << "White Pawn Bitboard: " << board.bitboards[W_PAWN] << std::endl;
        std::cout << "DEBUG: B_PAWN bitboard: " << board.bitboards[B_PAWN] << std::endl;
        
        // Scan the mailbox for the first 8 squares (Rank 1)
        std::cout << "Mailbox (Rank 1): ";
        for (int i = 0; i < 8; ++i) {
            std::cout << board.mailbox[i] << " ";
        }
        std::cout << "\n------------------------------\n" << std::endl;
    }
}