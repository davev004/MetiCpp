#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include "board.hpp"
#include "fen.hpp"
#include "search.hpp"
#include "movegen.hpp"
#include "move_logic.hpp"

namespace UCI {

    // Helper to safely parse a string like "e2e4" or "e7e8q" into our 32-bit format
    inline Meti::Move parse_move(const Board& board, const std::string& move_str) {
        if (move_str.length() < 4) return 0;

        int from_file = move_str[0] - 'a';
        int from_rank = move_str[1] - '1';
        int to_file   = move_str[2] - 'a';
        int to_rank   = move_str[3] - '1';

        int from = from_rank * 8 + from_file;
        int to   = to_rank * 8 + to_file;

        // Check for promotion character
        Meti::PromotionPiece prom = Meti::PROMOTION_KNIGHT; // Default fallback
        bool is_prom = false;
        if (move_str.length() == 5) {
            is_prom = true;
            if (move_str[4] == 'q') prom = Meti::PROMOTION_QUEEN;
            else if (move_str[4] == 'r') prom = Meti::PROMOTION_ROOK;
            else if (move_str[4] == 'b') prom = Meti::PROMOTION_BISHOP;
            else if (move_str[4] == 'n') prom = Meti::PROMOTION_KNIGHT;
        }

        // Generate all moves and find the exact match to extract the flags
        Meti::MoveList list;
        MoveGen::generate(board, list);

        for (int i = 0; i < list.count; i++) {
            Meti::Move move = list.moves[i];
            if (Meti::get_from(move) == from && Meti::get_to(move) == to) {
                // If it's a promotion, ensure the promotion piece matches too
                if (is_prom) {
                    if (Meti::get_prom(move) == prom) return move;
                } else {
                    return move;
                }
            }
        }
        return 0; // Invalid move
    }

    inline void loop() {
        Board board;
        std::string line, token;

        FEN::parse(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        while (std::getline(std::cin, line)) {
            std::istringstream ss(line);
            ss >> token;

            if (token == "quit") {
                break;
            } 
            else if (token == "uci") {
                std::cout << "id name MetiC++\n";
                std::cout << "id author David vaughan\n";
                std::cout << "uciok\n";
            } 
            else if (token == "isready") {
                std::cout << "readyok\n";
            } 
            else if (token == "position") {
                ss >> token;
                if (token == "startpos") {
                    FEN::parse(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                    ss >> token; // Consume "moves"
                } else if (token == "fen") {
                    std::string fen;
                    for (int i = 0; i < 6; ++i) { 
                        ss >> token;
                        fen += token + " ";
                    }
                    FEN::parse(board, fen);
                    ss >> token; // Consume "moves"
                }
                
                // --- THE NEW PARSER INTEGRATION ---
                while (ss >> token) {
                    Meti::Move parsed_move = parse_move(board, token);
                    if (parsed_move != 0) {
                        make_move(board, parsed_move);
                    }
                }
            } 
            else if (token == "go") {
                int depth = 5; 
                while (ss >> token) {
                    if (token == "depth") ss >> depth;
                }
                
                Meti::Move best_move = Search::search_root(board, depth);
                
                char from[3], to[3];
                Meti::square_to_coord(Meti::get_from(best_move), from);
                Meti::square_to_coord(Meti::get_to(best_move), to);
                
                std::string prom = "";
                Piece moving = Meti::get_moving(best_move);
                
                if ((moving == W_PAWN || moving == B_PAWN) && ((1ULL << Meti::get_to(best_move)) & 0xFF000000000000FFULL)) {
                    Meti::PromotionPiece p = Meti::get_prom(best_move);
                    if (p == Meti::PROMOTION_QUEEN) prom = "q";
                    else if (p == Meti::PROMOTION_ROOK) prom = "r";
                    else if (p == Meti::PROMOTION_BISHOP) prom = "b";
                    else prom = "n";
                }

                std::cout << "bestmove " << from << to << prom << std::endl;
            }
        }
    }
}