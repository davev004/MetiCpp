#include <thread>
#include <vector>
#include "board.hpp"
#include "search.hpp"
#include "move.hpp"

namespace SMP {
    constexpr int MAX_THREADS = 128; // Absolute maximum ceiling
    inline int active_threads = 1;   // Default to standard single-threaded

    inline void worker_search(Board local_board, int max_depth, long long allocated_ms, int thread_id) {
        uint64_t local_nodes = 0;
        
        // Offset starting depths to force threads into different branches
        // Thread 0 (Main) searches 1, 2, 3...
        // Thread 1 searches 2, 3, 4...
        // Thread 2 searches 3, 4, 5...
        int depth_offset = thread_id % 3; 
        
        // You will need to modify search_root to accept a starting depth
        Search::search_root(local_board, max_depth, allocated_ms, local_nodes, depth_offset, false);
    }

    inline Meti::Move launch(Board& root_board, int max_depth, long long allocated_ms) {
        int worker_count = active_threads - 1;
        if (worker_count <= 0) {
            // Fast path for 1 thread (No SMP overhead)
            Time::init(allocated_ms);
            uint64_t main_nodes = 0;
            return Search::search_root(root_board, max_depth, allocated_ms, main_nodes, 0);
        }

        Time::init(allocated_ms);
        std::thread workers[MAX_THREADS];
        
        for (int i = 0; i < worker_count; ++i) {
            workers[i] = std::thread(worker_search, root_board, max_depth, allocated_ms, i + 1);
        }

        uint64_t main_nodes = 0;
        Meti::Move best_move = Search::search_root(root_board, max_depth, allocated_ms, main_nodes, 0, true);

        Time::time_up = true;

        for (int i = 0; i < worker_count; ++i) {
            if (workers[i].joinable()) workers[i].join();
        }

        return best_move;
    }
}