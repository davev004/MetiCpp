#pragma once
#include <string>
#include <sstream>
#include <cctype>
#include "board.hpp"
#include "bitboard.hpp"
#include "zobrist.hpp"

namespace FEN {
    inline void parse(Board& board, const std::string& fen) {
        // 1. Clear ALL bitboards and occupancy
        for (int i = 0; i < Meti::NUM_PIECES; i++) board.bitboards[i] = 0;
        for (int i = 0; i < 64; i++) board.mailbox[i] = PIECE_NONE;
        board.occupancy[WHITE] = 0;
        board.occupancy[BLACK] = 0;
        board.occupancy[2] = 0; 
        board.state = {}; 
        board.ply = 0;

        std::istringstream ss(fen);
        std::string placement, color, castling, enPassant, halfMove, fullMove;
        ss >> placement >> color >> castling >> enPassant >> halfMove >> fullMove;

        // 2. Piece Placement
        int rank = 7, file = 0;
        for (char c : placement) {
            if (c == '/') {
                rank--;
                file = 0;
            } else if (std::isdigit(c)) {
                file += (c - '0'); // Skip empty squares
            } else {
                int square = rank * 8 + file;
                Piece p = PIECE_NONE;
                switch (c) {
                    case 'P': p = W_PAWN; break; case 'p': p = B_PAWN; break;
                    case 'N': p = W_KNIGHT; break; case 'n': p = B_KNIGHT; break;
                    case 'B': p = W_BISHOP; break; case 'b': p = B_BISHOP; break;
                    case 'R': p = W_ROOK; break; case 'r': p = B_ROOK; break;
                    case 'Q': p = W_QUEEN; break; case 'q': p = B_QUEEN; break;
                    case 'K': p = W_KING; break; case 'k': p = B_KING; break;
                }
                
                if (p != PIECE_NONE) {
                    // Set the piece bitboard
                    Bitboard::set_bit(board.bitboards[p], square);

                    board.mailbox[square] = p;
                    
                    // Update colour-specific occupancy
                    Colour pieceColour = (p <= W_KING) ? WHITE : BLACK;
                    Bitboard::set_bit(board.occupancy[pieceColour], square);

                    // XOR the Zobrist hash
                    board.state.zobristKey ^= Zobrist::pieceKeys[p][square];
                }
                file++;
            }
        }

        // 3. The Unification (O(1) derivation instead of loop overhead)
        board.occupancy[2] = board.occupancy[WHITE] | board.occupancy[BLACK];

        // 4. Active Colour
        board.state.sideToMove = (color == "b") ? BLACK : WHITE;
        if (board.state.sideToMove == BLACK) board.state.zobristKey ^= Zobrist::sideKey;

        // 5. Castling Rights
        board.state.castlingRights = Castling::NONE;
        if (castling != "-") {
            for (char c : castling) {
                if (c == 'K') board.state.castlingRights |= Castling::WK;
                if (c == 'Q') board.state.castlingRights |= Castling::WQ;
                if (c == 'k') board.state.castlingRights |= Castling::BK;
                if (c == 'q') board.state.castlingRights |= Castling::BQ;
            }
        }
        board.state.zobristKey ^= Zobrist::castleKeys[board.state.castlingRights];

        // 6. En Passant
        if (enPassant != "-") {
            int ep_file = enPassant[0] - 'a';
            int ep_rank = enPassant[1] - '1';
            board.state.enPassantSquare = ep_rank * 8 + ep_file;
            board.state.zobristKey ^= Zobrist::enPassantKeys[ep_file];
        } else {
            board.state.enPassantSquare = Meti::SQ_NONE;
        }

        // 7 & 8. Clocks
        board.state.halfMoveClock = std::stoi(halfMove);
        board.state.fullMoveNumber = std::stoi(fullMove);
    }
}