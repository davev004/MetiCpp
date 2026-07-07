#pragma once
#include <cstdint>
#include "constants.hpp"
#include "types.hpp"

struct Board {
    // The core LERF bitboards (A1 = LSB)
    // Indexed natively by the Piece enum (0 to 11)
    uint64_t bitboards[Meti::NUM_PIECES] = {0};

    //Occupancy Bitboards (0 = White, 1 = Black, 2 = Both)
    uint64_t occupancy[3] = {0};

    // The current transient state of the game
    GameState state;

    // Zero-allocation History Tracking
    GameState history[Meti::MAX_PLY];
    int ply = 0;

    // Core functionality (To be implemented)
    // void load_fen(const char* fen);
    // void print_board();
};