#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include "board.hpp"
#include "search.hpp"
#include "time.hpp"

// Parallelism is deliberately restricted to the root.  That keeps the
// recursive search stateless: every worker mutates only its own Board and its
// own node counter.  The only shared state while searching is the root work
// queue, the current best root result, the deadline, and the lock-free TT.
namespace SMP {
    constexpr int MAX_THREADS = 64;
    constexpr int MAX_WORKERS = MAX_THREADS - 1;

    struct alignas(64) RootWork {
        Board board;
        Meti::Move moves[256] = {};
        int count = 0;
        int depth = 0;
        std::atomic<int> next{0};
        std::atomic<int> legal_moves{0};
        std::atomic<uint64_t> best{0};
        std::atomic<uint64_t> nodes{0};
    };

    inline RootWork root_work;
    inline std::array<std::thread, MAX_WORKERS> workers;
    inline int worker_count = 0;
    inline int active_threads = 1;

    // These locks are used only while workers are asleep or while the caller
    // waits at an iterative-deepening barrier; no search node takes a lock.
    inline std::mutex pool_mutex;
    inline std::condition_variable wake_workers;
    inline std::condition_variable workers_done;
    inline std::atomic<bool> pool_running{false};
    inline std::atomic<uint32_t> work_generation{0};
    inline std::atomic<int> completed_workers{0};

    constexpr uint64_t RESULT_INDEX_MASK = 0xFFFFULL;

    inline uint64_t pack_result(int score, int index) {
        // An unsigned max selects the highest score and, on ties, the earlier
        // move in the ordered root list.
        const uint32_t sortable_score = static_cast<uint32_t>(score + Search::INF);
        return (static_cast<uint64_t>(sortable_score) << 32) |
               static_cast<uint64_t>(0xFFFF - index);
    }

    inline int result_score(uint64_t result) {
        return static_cast<int>(result >> 32) - Search::INF;
    }

    inline int result_index(uint64_t result) {
        return 0xFFFF - static_cast<int>(result & RESULT_INDEX_MASK);
    }

    inline void publish_result(int score, int index) {
        const uint64_t candidate = pack_result(score, index);
        uint64_t observed = root_work.best.load(std::memory_order_relaxed);
        while (candidate > observed &&
               !root_work.best.compare_exchange_weak(observed, candidate,
                                                      std::memory_order_relaxed,
                                                      std::memory_order_relaxed)) {
        }
    }

    inline void run_root_work() {
        Board board = root_work.board;
        uint64_t local_nodes = 0;

        while (!Time::time_up.load(std::memory_order_relaxed)) {
            const int index = root_work.next.fetch_add(1, std::memory_order_relaxed);
            if (index >= root_work.count) break;

            const Meti::Move move = root_work.moves[index];
            if (!is_castling_legal(board, move) || !make_move(board, move)) continue;
            if (!is_legal(board, move)) {
                unmake_move(board, move);
                continue;
            }

            root_work.legal_moves.fetch_add(1, std::memory_order_relaxed);

            // Principal-variation root splitting: test against the best score
            // seen so far, then only pay for a full window on a contender.
            const int alpha = result_score(root_work.best.load(std::memory_order_relaxed));
            Search::Result probe = Search::negamax(board, root_work.depth - 1,
                                                    -alpha - 1, -alpha, 1, local_nodes);
            unmake_move(board, move);
            if (probe.aborted) break;

            const int probe_score = -probe.score;
            if (probe_score <= alpha) continue;

            if (!make_move(board, move)) continue;
            Search::Result full = Search::negamax(board, root_work.depth - 1,
                                                   -Search::INF, Search::INF, 1, local_nodes);
            unmake_move(board, move);
            if (full.aborted) break;
            publish_result(-full.score, index);
        }

        root_work.nodes.fetch_add(local_nodes, std::memory_order_relaxed);
    }

    inline void worker_loop() {
        uint32_t observed_generation = 0;
        while (true) {
            std::unique_lock<std::mutex> lock(pool_mutex);
            wake_workers.wait(lock, [&] {
                return !pool_running.load(std::memory_order_acquire) ||
                       work_generation.load(std::memory_order_acquire) != observed_generation;
            });

            if (!pool_running.load(std::memory_order_acquire)) return;
            observed_generation = work_generation.load(std::memory_order_relaxed);
            lock.unlock();

            run_root_work();
            completed_workers.fetch_add(1, std::memory_order_release);
            workers_done.notify_one();
        }
    }

    inline void stop_workers() {
        if (worker_count == 0) return;
        pool_running.store(false, std::memory_order_release);
        wake_workers.notify_all();
        for (int i = 0; i < worker_count; ++i) {
            if (workers[i].joinable()) workers[i].join();
        }
        worker_count = 0;
    }

    inline void init() {
        active_threads = 1;
        worker_count = 0;
        pool_running.store(false, std::memory_order_relaxed);
    }

    inline void set_threads(int count) {
        if (count < 1) count = 1;
        if (count > MAX_THREADS) count = MAX_THREADS;
        if (count == active_threads) return;

        stop_workers();
        active_threads = count;
        worker_count = count - 1;
        if (worker_count == 0) return;

        pool_running.store(true, std::memory_order_release);
        for (int i = 0; i < worker_count; ++i) {
            workers[i] = std::thread(worker_loop);
        }
    }

    inline void stop_all() {
        stop_workers();
        active_threads = 1;
    }

    inline bool search_first_root_move(const Meti::MoveList& list, int depth,
                                       int& first_index, uint64_t& nodes) {
        Board board = root_work.board;
        for (int index = 0; index < list.count; ++index) {
            const Meti::Move move = list.moves[index];
            if (!is_castling_legal(board, move) || !make_move(board, move)) continue;
            if (!is_legal(board, move)) {
                unmake_move(board, move);
                continue;
            }

            root_work.legal_moves.store(1, std::memory_order_relaxed);
            Search::Result result = Search::negamax(board, depth - 1,
                                                     -Search::INF, Search::INF, 1, nodes);
            unmake_move(board, move);
            if (result.aborted) return false;
            publish_result(-result.score, index);
            first_index = index;
            return true;
        }
        return true;
    }

    inline void dispatch_workers() {
        completed_workers.store(0, std::memory_order_relaxed);
        work_generation.fetch_add(1, std::memory_order_release);
        wake_workers.notify_all();
    }

    inline void wait_for_workers() {
        std::unique_lock<std::mutex> lock(pool_mutex);
        workers_done.wait(lock, [] {
            return completed_workers.load(std::memory_order_acquire) == worker_count;
        });
    }

    inline Meti::Move search_parallel(Board& board, int max_depth) {
        Meti::Move best_move = 0;
        Meti::Move previous_best = 0;
        uint64_t total_nodes = 0;

        for (int depth = 1; depth <= max_depth; ++depth) {
            Meti::MoveList list;
            MoveGen::generate(board, list);
            MoveOrdering::sort_moves(board, list, previous_best);

            root_work.board = board;
            root_work.count = list.count;
            root_work.depth = depth;
            root_work.next.store(0, std::memory_order_relaxed);
            root_work.legal_moves.store(0, std::memory_order_relaxed);
            root_work.nodes.store(0, std::memory_order_relaxed);
            root_work.best.store(pack_result(-Search::INF, 0), std::memory_order_relaxed);
            for (int i = 0; i < list.count; ++i) root_work.moves[i] = list.moves[i];

            int first_index = -1;
            uint64_t first_nodes = 0;
            if (!search_first_root_move(list, depth, first_index, first_nodes) ||
                Time::time_up.load(std::memory_order_relaxed)) {
                break;
            }
            root_work.nodes.fetch_add(first_nodes, std::memory_order_relaxed);

            if (first_index < 0) return 0;

            root_work.next.store(first_index + 1, std::memory_order_relaxed);
            dispatch_workers();
            run_root_work();
            wait_for_workers();

            if (Time::time_up.load(std::memory_order_relaxed)) break;

            const uint64_t result = root_work.best.load(std::memory_order_relaxed);
            const int best_index = result_index(result);
            if (best_index < 0 || best_index >= list.count) break;

            best_move = list.moves[best_index];
            previous_best = best_move;
            total_nodes += root_work.nodes.load(std::memory_order_relaxed);

            Search::PVLine pv;
            Search::extract_pv(board, best_move, depth, pv);
            std::cout << "info depth " << depth << " score cp " << result_score(result)
                      << " nodes " << total_nodes;
            if (pv.length > 0) {
                std::cout << " pv";
                for (int pv_index = 0; pv_index < pv.length; ++pv_index) {
                    std::cout << ' ';
                    Search::print_move(pv.moves[pv_index]);
                }
            }
            std::cout << std::endl;
        }
        return best_move;
    }

    inline Meti::Move launch(Board& board, int max_depth, long long allocated_ms) {
        Time::init(allocated_ms);
        if (active_threads == 1) {
            uint64_t nodes = 0;
            return Search::search_root(board, max_depth, allocated_ms, nodes);
        }
        return search_parallel(board, max_depth);
    }
}
