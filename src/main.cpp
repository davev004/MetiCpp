#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <perft.hpp>
#include "constants.hpp"
#include "types.hpp"
#include "zobrist.hpp"
#include "board.hpp"
#include "bitboard.hpp"
#include "fen.hpp"
#include "debug.hpp" 

int main() {
    // 1. Initialize Zobrist Keys (Must be done exactly once at startup)
    Zobrist::init();

    // 2. Open the FEN test suite
    std::ifstream fen_file("../testdata/testfens.txt");
    if (!fen_file.is_open()) {
        // Fallback in case you run the executable from inside the build directory
        fen_file.open("testdata/testfens.txt");
        if (!fen_file.is_open()) {
            std::cerr << "Error: Could not locate testfens.txt\n";
            return 1;
        }
    }

    std::string line;
    Board board;

    // 3. Parse and Print
    while (std::getline(fen_file, line)) {
        if (line.empty() || line[0] == '#') continue; // Skip blanks and comments

        std::cout << "FEN: " << line << "\n";
        FEN::parse(board, line);
        print_board(board);
        
        // Print the Zobrist Key to verify the mathematical XOR logic is working
        std::cout << "Zobrist Key: 0x" << std::hex << board.state.zobristKey << std::dec << "\n";
        std::cout << "--------------------------------------------------\n";
    }
    // Example: Test a specific position
    std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    FEN::parse(board, start_fen);

    int depth = 3;
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t nodes = Perft::run(board, depth);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> diff = end - start;
    std::cout << "Perft depth " << depth << ": " << nodes << " nodes in " << diff.count() << "s\n";

    return 0;

    

    return 0;
}