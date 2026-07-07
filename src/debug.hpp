#include <iostream>
#include "bitboard.hpp"
#include "board.hpp"

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