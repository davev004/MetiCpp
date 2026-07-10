#pragma once
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <vector>
#include "board.hpp"
#include "search.hpp"
#include "time.hpp"

namespace SMP {
    constexpr int MAX_THREADS = 128;
    inline int active_threads = 1;

    // Thread pool state
    inline std::vector<std::thread> workers; // Dynamically scales now!
    inline std::mutex smp_mutex;
    inline std::condition_variable cv_wake;
    inline std::condition_variable cv_done;

    // Search parameters shared to workers
    inline Board root_board;
    inline int search_depth = 0;
    inline long long search_time = 0;
    
    // Lock-free state management
    inline std::atomic<int> threads_running{0};
    inline std::atomic<bool> engine_running{true};
    inline std::atomic<int> current_search_id{0}; 

    inline void worker_search(int thread_id) {
        int local_search_id = 0;

        while (engine_running.load(std::memory_order_relaxed)) {
            // 1. Sleep until a new search starts or the engine shuts down
            std::unique_lock<std::mutex> lock(smp_mutex);
            cv_wake.wait(lock, [&] { 
                return current_search_id.load(std::memory_order_relaxed) != local_search_id 
                    || !engine_running.load(std::memory_order_relaxed); 
            });
            lock.unlock();

            if (!engine_running.load(std::memory_order_relaxed)) break;

            // Synchronise the search ID so we don't spin-loop
            local_search_id = current_search_id.load(std::memory_order_relaxed);

            // Only participate if this thread ID is within the active count
            if (thread_id < active_threads) {
                // --- HOT PATH: COMPLETELY LOCK-FREE ---
                Board local_board = root_board;
                uint64_t local_nodes = 0;
                int depth_offset = thread_id % 3; 
                Search::search_root(local_board, search_depth, search_time, local_nodes, depth_offset, false);

                // 2. Signal that this thread has finished its work safely
                threads_running.fetch_sub(1, std::memory_order_release);
                cv_done.notify_all();
            }
        }
    }

    inline void init() {
        engine_running.store(true, std::memory_order_relaxed);
        active_threads = 1;
        threads_running.store(0, std::memory_order_relaxed);
        workers.clear();
        // Threads are NO LONGER spawned here. We wait for the GUI to request them.
    }

    inline void set_threads(int count) {
        if (count < 1) count = 1;
        if (count > MAX_THREADS) count = MAX_THREADS;
        
        active_threads = count;

        // Lazily spawn native background threads up to the requested count
        while ((int)workers.size() < (active_threads - 1)) {
            int new_thread_id = (int)workers.size() + 1;
            workers.emplace_back(worker_search, new_thread_id);
        }
    }

    inline void stop_all() {
        if (engine_running.load(std::memory_order_relaxed)) {
            engine_running.store(false, std::memory_order_relaxed);
            
            {
                std::lock_guard<std::mutex> lock(smp_mutex);
                current_search_id.fetch_add(1, std::memory_order_relaxed); 
            }
            cv_wake.notify_all();

            for (auto& worker : workers) {
                if (worker.joinable()) worker.join();
            }
            workers.clear();
        }
    }

    inline Meti::Move launch(Board& board, int max_depth, long long allocated_ms) {
        int worker_count = active_threads - 1;
        
        if (worker_count <= 0) {
            // Fast path for 1 thread (No SMP overhead)
            Time::init(allocated_ms);
            uint64_t main_nodes = 0;
            return Search::search_root(board, max_depth, allocated_ms, main_nodes, 0, true);
        }

        // Setup parameters for the worker threads
        root_board = board; 
        search_depth = max_depth;
        search_time = allocated_ms;
        
        Time::init(allocated_ms);

        // Wake the exact number of workers we need
        threads_running.store(worker_count, std::memory_order_release);
        current_search_id.fetch_add(1, std::memory_order_release);
        cv_wake.notify_all();

        // Main thread (Thread 0) searches alongside them
        uint64_t main_nodes = 0;
        Meti::Move best_move = Search::search_root(board, max_depth, allocated_ms, main_nodes, 0, true);

        // Time is up or depth reached; force workers to abort
        Time::time_up.store(true, std::memory_order_relaxed);

        // Sleep the main thread until all workers safely exit the search tree
        std::unique_lock<std::mutex> lock(smp_mutex);
        cv_done.wait(lock, [] { return threads_running.load(std::memory_order_acquire) == 0; });

        return best_move;
    }
}