# MetiC++ Codebase Audit

Audit date: 2026-10-26

## Scope and method

Reviewed the CMake and Make build files, README, every file under `src/`, and
the FEN corpus under `testdata/`. The current tree now includes a focused CTest
target and GitHub Actions workflows. The build was compiled with GCC and
exercised with perft, sanitizer, and UCI checks.

Observed baseline:

- `cmake --build build --parallel 2` succeeds.
- The executable answers `uciok` and `readyok`.
- The release build and focused regression test pass locally.
- ASan/UBSan and TSan test configurations also pass locally.

Severity uses **Critical**, **High**, **Medium**, and **Low**. Findings are
ordered by likely effect on playing strength or correctness.

## Findings

### Critical: negamax sign convention is inconsistent with evaluation

`Eval::evaluate` returns a white-positive score: material and white piece-square
terms are added, while black terms are subtracted, and it never changes the
sign based on `sideToMove` ([src/eval.hpp](src/eval.hpp#L150-L192)). The search
uses negamax and negates every child result ([src/search.hpp](src/search.hpp#L219-L226)).
At a depth-one white search, the leaf is evaluated after White's move and then
negated, causing the root to prefer positions that are worse for White. The
same mismatch affects black-root searches and all odd/even search depths.

Choose one convention and apply it everywhere. The smallest likely repair is
to return the evaluation from the side-to-move perspective, e.g. negate the
white-positive score when Black is to move, while keeping negamax's child
negation. Add depth-one and depth-two material-only tests for both sides before
changing search heuristics.

### Critical: castling transit-square legality is checked on the wrong position

Moves are made before legality is checked ([src/perft.hpp](src/perft.hpp#L19-L30),
[src/search.hpp](src/search.hpp#L219-L226)). `is_legal` correctly checks the
post-move king square, but for castling it checks the origin and transit squares
after the king and rook have already moved ([src/move_logic.hpp](src/move_logic.hpp#L20-L43)).
The rook on f1/f8 can block an attack that should make the transit square
illegal, and the moved king no longer represents the original occupancy.
This can accept illegal castles or reject valid ones in x-ray positions.

Check the origin and transit squares on the pre-move board, or explicitly
remove/move only the king while testing each square. Keep the final destination
check on the post-move board. Add FIDE castling positions with attacks on the
origin, transit, and destination squares.

### Critical: shared transposition-table entries have C++ data races

The TT is global and all search threads read and write the same `Entry`
without a lock or atomic representation ([src/transitiontable.hpp](src/transitiontable.hpp#L20-L32),
[src/transitiontable.hpp](src/transitiontable.hpp#L36-L80)). `volatile` in
`probe` does not make the multi-field read/write operation atomic or defined;
the writer can update fields while another thread copies them. The signature
reduces the chance of accepting a torn entry but does not remove undefined
behavior. ThreadSanitizer should report this under multi-threaded search.

Use an atomic packed entry with a documented publication protocol, or protect
replacement/read with a lock. A practical design is an atomic 128-bit payload
where supported, or an atomic key/version written last with acquire/release
and a retrying reader. Also clear or version the table when its layout or
hash-generation policy changes.

### Critical: search timeout can store incomplete results in the TT

`Time::check` sets a global timeout flag ([src/time.hpp](src/time.hpp#L16-L24)),
and `negamax` returns `0` immediately when it observes it ([src/search.hpp](src/search.hpp#L85-L91)). A parent can treat that `0` as a completed child,
then continue to terminal/bound calculation and store the partial result
([src/search.hpp](src/search.hpp#L168-L198]). The next search can probe this
entry and use a bound produced by an aborted subtree.

Propagate an explicit abort result, or check `time_up` immediately after every
child and return without storing a TT entry. Only completed nodes should be
classified as exact, lower, or upper bounds.

### High: history and move application have no capacity guard

`make_move` writes `board.history[board.ply++]` without checking
`Meti::MAX_PLY` ([src/move_logic.cpp](src/move_logic.cpp#L17-L27)). Perft does not
enforce the limit, and the search guard is only reached after entering a node
([src/search.hpp](src/search.hpp#L93-L94)). A sufficiently long legal game or
an unusually deep search can write past the fixed history array. Add a checked
push or make the limit an invariant enforced before every move; return a draw
or abort search at the limit rather than corrupting memory.

### High: FEN parsing accepts malformed input and can invoke undefined behavior

The parser assumes six fields, valid placement dimensions, valid piece letters,
and a two-character en-passant square ([src/fen.hpp](src/fen.hpp#L20-L42),
[src/fen.hpp](src/fen.hpp#L80-L92)). It does not validate rank/file bounds, so
bad placement can index bitboards/mailbox outside 0..63. An empty or malformed
clock throws from `std::stoi`, and malformed en-passant text can index with an
invalid file/rank. UCI `position fen` therefore can terminate the engine or
corrupt state on input that should be rejected.

Make parsing return success/failure, validate all six fields and exact board
shape before mutating `Board`, validate numeric ranges before conversion, and
reject invalid FEN with a protocol-safe response or retained position.

### High: castling rights are trusted without verifying the king and rook

The FEN parser sets rights solely from the rights token
([src/fen.hpp](src/fen.hpp#L68-L78)), and move generation emits a castle when
the rights bit and empty transit mask are present ([src/movegen.hpp](src/movegen.hpp#L168-L190)).
It never requires the king on e1/e8 or the corresponding rook on a/h files.
FENs with stale rights, and any state produced by an external caller, can
generate a castle with a nonexistent rook; `make_move` then toggles the rook
bitboard and mailbox at assumed squares ([src/move_logic.cpp](src/move_logic.cpp#L92-L114)).

Either reject inconsistent FEN rights or require the expected king and rook in
the castling generator and transition code. Add tests for each of the four
missing-rook/missing-king cases.

### High: TT lifecycle and sizing are not defensive

`probe` and `store` use `table[key % num_entries]` without checking that the
table is allocated and nonempty ([src/transitiontable.hpp](src/transitiontable.hpp#L34-L43)).
The current main path allocates 64 MB, but the API is globally callable and an
allocation failure is not handled. `allocate` also deletes and replaces the
table while worker searches could be active if `setoption Hash` is accepted at
the wrong time ([src/uci.hpp](src/uci.hpp#L199-L217)).

Guard zero/uninitialized tables, validate allocation size and overflow, and
only resize after stopping/joining search workers. Prefer RAII ownership over
raw global storage.

### High: time control is process-global and shared by all searches

`Time::start_time`, `allocated_ms`, and `time_up` are global state
([src/time.hpp](src/time.hpp#L5-L14)). In SMP mode every worker reads the same
timer, which is intentional for a shared deadline, but `Time::init` and
`time_up` can race with a new search if a command starts while the prior worker
set is still winding down. The code relies on UCI joins for sequencing and has
no generation/token associated with a search. A stale worker can therefore
observe or publish state for a later search.

Use an immutable per-search context containing deadline and cancellation
generation, pass it through search calls, and reject stale worker results.

### High: SMP worker results are discarded while adding shared-state risk

Workers perform full independent searches ([src/smp.hpp](src/smp.hpp#L50-L60]),
but `launch` returns only the main thread's result ([src/smp.hpp](src/smp.hpp#L128-L139)).
The extra searches do not improve the selected move, while they do contend on
the TT, logger, and global timeout. Either implement a synchronized result/PV
aggregation strategy or remove the worker searches until parallel search is
correct.

### Medium: no legal-move result is converted to UCI checkmate/stalemate output

If the root has no legal moves, `search_root` returns zero
([src/search.hpp](src/search.hpp#L204-L244)). `async_search_worker` then converts
square zero to `a1` twice ([src/uci.hpp](src/uci.hpp#L55-L76)), producing
`bestmove a1a1` instead of the UCI-required `bestmove 0000`.

Return a distinct no-move value and serialize it as `0000`. Add checkmate and
stalemate root tests.

### Medium: UCI move parsing accepts malformed promotion suffixes

`parse_move` treats any five-character suffix as a promotion and silently
defaults unknown letters to a knight; lengths other than four or five are not
rejected before matching generated moves ([src/uci.hpp](src/uci.hpp#L25-L53)).
This can accept commands such as `e7e8x` as a knight promotion and makes
protocol error handling ambiguous.

Require exactly four characters for non-promotion and exactly five with one of
`q`, `r`, `b`, or `n` for promotion. Validate coordinate ranges before creating
or comparing moves.

### Medium: en-passant hash state is copied from FEN without canonicalization

The parser hashes any supplied en-passant square
([src/fen.hpp](src/fen.hpp#L80-L88)), even when the side to move has no pawn
that can capture there. This can prevent repetition detection for equivalent
positions that differ only by a stale FEN en-passant field. The move generator
also trusts the square and does not verify the capturable pawn before creating
an en-passant move ([src/movegen.hpp](src/movegen.hpp#L62-L85)).

Validate the square, the side-to-move pawn source, and the opposing pawn on the
capture square. Decide and document whether the hash uses raw FEN state or the
canonical legal-move state; for repetition, canonicalizing unavailable EP is
usually preferable.

### Medium: logger writes are unsynchronized from worker threads

The global `std::ofstream` is written by the UCI thread and multiple workers
without a mutex ([src/logger.hpp](src/logger.hpp#L5-L20),
[src/smp.hpp](src/smp.hpp#L32-L60)). Concurrent stream mutation is undefined
behavior and can interleave or corrupt the log. Add a mutex, use a single
logging consumer, or disable logging in production search.

### Medium: build is host-specific and flags are injected globally

The build unconditionally uses `-march=native`, `-O3`, LTO, and `-DNDEBUG`
([CMakeLists.txt](CMakeLists.txt#L7-L15)). A binary built on one host may fail
to execute on another CPU, and consumers cannot select a portable release
profile. Directly modifying `CMAKE_CXX_FLAGS` also affects every target and
makes toolchain configuration harder.

Use target-scoped options and a selectable Release profile; make native
optimization opt-in and provide a baseline target. Add a CI build without
`-march=native` and with sanitizers.

### Medium: no executable regression tests cover the engine

`testdata/testfens.txt` is only a list of positions; there is no CTest target,
test executable, or command-line perft interface. The existing `main` enters
the UCI loop immediately ([src/main.cpp](src/main.cpp#L15-L30)). This leaves
move counts, make/unmake restoration, hashes, castling, EP, promotion, mate,
and TT behavior unprotected.

Add a test target with known perft positions (start position through at least
depth 4, plus Kiwipete, EP, castling, and promotion positions), state/hash
round-trip tests, FEN rejection tests, and UCI protocol tests. Run ASan,
UBSan, and TSan configurations in CI.

### Low: checked-in build artifacts obscure reproducibility

The workspace includes a populated `build/` tree and generated compile files,
while the Makefile's `clean` removes it. Generated artifacts should be ignored
and recreated by documented commands; otherwise stale flags and binaries can
be mistaken for current source behavior.

### Low: source warnings and stale comments indicate unfinished paths

The build reports unused `our_pawn`, unused `Board` in move scoring, and unused
`allocated_ms` in root search. There are also comments claiming branchless or
fully optimized behavior while the implementation still has ordinary branches.
Remove dead parameters/variables or implement their intended use, and keep
comments aligned with invariants rather than optimization claims.

## Recommended repair order

1. Fix the evaluation/search sign convention and add material search tests.
2. Fix castling legality and validate king/rook/EP preconditions.
3. Make timeout aborts explicit and prevent incomplete TT stores.
4. Remove or redesign shared TT/logger concurrency; make search context local.
5. Add history/input bounds checks and strict FEN/UCI validation.
6. Add perft, round-trip, sanitizer, and protocol tests before further tuning.
7. Make the build portable and clean up warnings and generated artifacts.

## Validation limitations

The regression suite covers start-position perft through depth 3, state
restoration, special moves, strict FEN parsing, and UCI move parsing. It does
not yet cover the full standard Kiwipete position, mate/stalemate root search,
or parallel search because parallel search is intentionally disabled.

## Remediation status

The audit findings were addressed in the current working tree as follows:

- Evaluation is side-to-move relative, and timed-out search nodes now propagate
  an explicit abort without storing incomplete TT results.
- Castling now validates the original king/rook position and attacked origin
  and transit squares before mutation. En-passant and history bounds are also
  validated.
- FEN parsing is transactional and rejects malformed placement, clocks,
  castling rights, and unavailable en-passant squares. UCI coordinates and
  promotion suffixes are strict, and terminal search returns `bestmove 0000`.
- TT payloads and publication keys use atomics without mutexes in the search
  path. Hash resizing is serialized with search termination.
- The unfinished parallel search path is disabled by design (`Threads max 1`)
  until worker result aggregation exists; this avoids discarded results and
  stale shared timer state.
- A CTest target now covers start-position perft depth 1/2/3, state restoration,
  special moves, strict UCI parsing, and malformed FEN rejection. The build
  also has an opt-in `METI_NATIVE` optimization flag instead of requiring
  `-march=native`.
- GitHub Actions now runs Release CTest, ASan/UBSan, TSan, and release-artifact
  verification before publishing a release.

The remaining deliberate limitation is that multi-threaded search is not yet
implemented. The Kiwipete fixture from the original audit should be restored
once its exact expected divide is independently verified; it is not used as a
passing regression until then.