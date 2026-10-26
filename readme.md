# MetiC++

MetiC++ is a UCI chess engine written in C++20. It supports standard chess
position setup, time/depth-limited search, transposition-table hashing, and UCI
principal-variation output for analysis tools such as En Croissant.

## Build

Requirements:

- CMake 3.20 or newer
- A C++20 compiler

Using Make:

```sh
make build
```

Using CMake directly:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

For host-specific CPU optimization, enable the optional native flag:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMETI_NATIVE=ON
cmake --build build --parallel
```

The engine binary is `build/meticpp`.

## Tests

Run the built-in regression suite with:

```sh
ctest --test-dir build --output-on-failure
```

The tests cover start-position perft through depth 3, state restoration,
castling, en passant, promotion, FEN validation, and UCI move parsing.

## Running

MetiC++ communicates through standard UCI input/output:

```sh
./build/meticpp
```

Initial handshake:

```text
uci
isready
```

The engine responds with `uciok` and `readyok`.

## UCI Commands

### Position setup

Load the standard starting position:

```text
position startpos
```

Load a starting position and apply moves in long algebraic notation:

```text
position startpos moves e2e4 e7e5 g1f3
```

Load a FEN position. A FEN must contain all six fields:

```text
position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
```

Promotion moves use `q`, `r`, `b`, or `n`:

```text
position fen 4k3/P7/8/8/8/8/8/4K3 w - - 0 1 moves a7a8q
```

### Search by depth

Search a fixed number of plies:

```text
go depth 12
```

### Search with clock time

Supply the side clocks and optional increments in milliseconds:

```text
go wtime 300000 btime 300000 winc 2000 binc 2000
```

The engine allocates approximately one twentieth of the remaining clock plus
half the increment.

### Stop and quit

```text
stop
quit
```

### Options

Set the transposition-table size in megabytes:

```text
setoption name Hash value 128
```

The accepted hash range is 1 to 16384 MB. `Threads` controls a persistent pool
of search workers and defaults to 1. Set it before `go`:

```text
setoption name Threads value 4
```

The engine supports 1 through 64 search threads. With one thread, the engine
uses the regular single-threaded search path. With multiple threads, the main
search thread takes the first move after move ordering and the worker pool
searches the remaining root moves concurrently at each iterative-deepening
depth. Parallelism is deliberately limited to the root, so every worker keeps
an independent board and history stack while sharing the lock-free
transposition table, deadline, and root work queue. Exact root results are
merged atomically, and changing `Threads` stops and recreates the worker pool.

## Analysis Output

During iterative search the engine emits standard UCI information lines:

```text
info depth 3 score cp 62 nodes 2418 pv d2d4 c7c6 c2c4
```

The `pv` sequence is the current principal variation, or best line found at
that depth. En Croissant and other UCI analysis interfaces can display these
lines automatically. The final result is reported as:

```text
bestmove d2d4
```

For checkmate or stalemate positions with no legal move, the engine reports:

```text
bestmove 0000
```

## Release

The current release is `v3.0.0`, including parallel root search with persistent
worker threads.
