#pragma once
#include <charconv>
#include <string>
#include <sstream>
#include <cctype>
#include "board.hpp"
#include "bitboard.hpp"
#include "zobrist.hpp"

namespace FEN {
        inline bool parse_number(const std::string& value, unsigned long& result) {
            if (value.empty()) return false;
            const char* first = value.data();
            const char* last = first + value.size();
            auto conversion = std::from_chars(first, last, result);
            return conversion.ec == std::errc() && conversion.ptr == last;
        }

        inline bool parse(Board& board, const std::string& fen) {
            std::istringstream ss(fen);
            std::string placement, color, castling, en_passant, half_move, full_move, extra;
            if (!(ss >> placement >> color >> castling >> en_passant >> half_move >> full_move) || ss >> extra) {
                return false;
            }
            if (color != "w" && color != "b") return false;
            if (castling.empty()) return false;

            unsigned long half = 0;
            unsigned long full = 0;
            if (!parse_number(half_move, half) || !parse_number(full_move, full) ||
                half > 255 || full == 0 || full > 65535) return false;

            Board candidate;
            candidate.state = {};
            int rank = 7;
            int file = 0;
            int kings[2] = {0, 0};

            for (char c : placement) {
                if (c == '/') {
                    if (file != 8 || rank == 0) return false;
                    --rank;
                    file = 0;
                    continue;
                }
                if (std::isdigit(static_cast<unsigned char>(c))) {
                    int gap = c - '0';
                    if (gap < 1 || gap > 8 || file + gap > 8) return false;
                    file += gap;
                    continue;
                }

                Piece piece = PIECE_NONE;
                switch (c) {
                    case 'P': piece = W_PAWN; break; case 'p': piece = B_PAWN; break;
                    case 'N': piece = W_KNIGHT; break; case 'n': piece = B_KNIGHT; break;
                    case 'B': piece = W_BISHOP; break; case 'b': piece = B_BISHOP; break;
                    case 'R': piece = W_ROOK; break; case 'r': piece = B_ROOK; break;
                    case 'Q': piece = W_QUEEN; break; case 'q': piece = B_QUEEN; break;
                    case 'K': piece = W_KING; ++kings[WHITE]; break;
                    case 'k': piece = B_KING; ++kings[BLACK]; break;
                    default: return false;
                }
                if (file >= 8 || rank < 0) return false;
                int square = rank * 8 + file++;
                Bitboard::set_bit(candidate.bitboards[piece], square);
                candidate.mailbox[square] = piece;
                Colour piece_colour = (piece <= W_KING) ? WHITE : BLACK;
                Bitboard::set_bit(candidate.occupancy[piece_colour], square);
                candidate.state.zobristKey ^= Zobrist::pieceKeys[piece][square];
            }
            if (rank != 0 || file != 8 || kings[WHITE] != 1 || kings[BLACK] != 1) return false;
            candidate.occupancy[2] = candidate.occupancy[WHITE] | candidate.occupancy[BLACK];

            candidate.state.sideToMove = (color == "b") ? BLACK : WHITE;
            if (candidate.state.sideToMove == BLACK) candidate.state.zobristKey ^= Zobrist::sideKey;

            candidate.state.castlingRights = Castling::NONE;
            if (castling != "-") {
                for (char c : castling) {
                    if (c == 'K') candidate.state.castlingRights |= Castling::WK;
                    else if (c == 'Q') candidate.state.castlingRights |= Castling::WQ;
                    else if (c == 'k') candidate.state.castlingRights |= Castling::BK;
                    else if (c == 'q') candidate.state.castlingRights |= Castling::BQ;
                    else return false;
                }
            }
            if ((candidate.state.castlingRights & Castling::WK) &&
                (candidate.mailbox[4] != W_KING || candidate.mailbox[7] != W_ROOK)) return false;
            if ((candidate.state.castlingRights & Castling::WQ) &&
                (candidate.mailbox[4] != W_KING || candidate.mailbox[0] != W_ROOK)) return false;
            if ((candidate.state.castlingRights & Castling::BK) &&
                (candidate.mailbox[60] != B_KING || candidate.mailbox[63] != B_ROOK)) return false;
            if ((candidate.state.castlingRights & Castling::BQ) &&
                (candidate.mailbox[60] != B_KING || candidate.mailbox[56] != B_ROOK)) return false;
            candidate.state.zobristKey ^= Zobrist::castleKeys[candidate.state.castlingRights];

            candidate.state.enPassantSquare = Meti::SQ_NONE;
            if (en_passant != "-") {
                if (en_passant.size() != 2 || en_passant[0] < 'a' || en_passant[0] > 'h' ||
                    en_passant[1] < '1' || en_passant[1] > '8') return false;
                int ep_file = en_passant[0] - 'a';
                int ep_rank = en_passant[1] - '1';
                int ep_square = ep_rank * 8 + ep_file;
                int capture_square = ep_square ^ 8;
                Piece pawn = (candidate.state.sideToMove == WHITE) ? B_PAWN : W_PAWN;
                uint64_t sources = candidate.bitboards[(candidate.state.sideToMove == WHITE) ? W_PAWN : B_PAWN];
                uint64_t ep_mask = 1ULL << ep_square;
                uint64_t source_mask = candidate.state.sideToMove == WHITE
                    ? (((ep_mask & ~0x8080808080808080ULL) >> 7) |
                       ((ep_mask & ~0x0101010101010101ULL) >> 9))
                    : (((ep_mask & ~0x0101010101010101ULL) << 7) |
                       ((ep_mask & ~0x8080808080808080ULL) << 9));
                if (candidate.mailbox[ep_square] != PIECE_NONE || candidate.mailbox[capture_square] != pawn ||
                    (ep_rank != ((candidate.state.sideToMove == WHITE) ? 5 : 2)) || !(sources & source_mask)) return false;
                candidate.state.enPassantSquare = static_cast<uint8_t>(ep_square);
                candidate.state.zobristKey ^= Zobrist::enPassantKeys[ep_file];
            }

            candidate.state.halfMoveClock = static_cast<uint8_t>(half);
            candidate.state.fullMoveNumber = static_cast<uint16_t>(full);
            board = candidate;
            return true;
        }
}