#pragma once

namespace Meti {
    // History & Search Limits
    constexpr int MAX_PLY = 256;         // Guarantees zero heap allocation for history
    
    // Board Geometry (LERF Mapping)
    constexpr int NUM_SQUARES = 64;
    constexpr int NUM_COLOURS = 2;
    constexpr int NUM_PIECE_TYPES = 6;
    constexpr int NUM_PIECES = 12;       // 1D array size for the bitboards
    
    // Square Constants (A1 = LSB)
    constexpr int SQ_NONE = 64;          // Used for null en-passant squares
}