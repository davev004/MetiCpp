# Janus

A UCI-compatible Java chess engine compiled to native binary via GraalVM. Named after the two-faced Roman god of foresight.

Built to compete with (C)Epimetheus.

For full development roadmap see [ROADMAP.md](ROADMAP.md)
---

## Design Principles

- **Multithreading from the ground up:** Lazy SMP architecture from day one, no retrofitting
- **Zero heap allocation in search:** no object creation per node, primitives and arrays only
- **Lockless shared state:** transposition table uses XOR hashing, no synchronisation overhead
- **Self-configuring concurrency:** thread count from `availableProcessors()`, optimal on any hardware
- **Native binary:** GraalVM Native Image, `-march=native`, behaves like a C binary

---

## Deliverables

### Phase 0 - Foundation
- [ ] Gradle + GraalVM Native Build Tools scaffolding
- [ ] `Board` - 12 bitboard representation
- [ ] `MoveGenerator` - stateless, thread-safe
- [ ] `MoveEncoder` - moves as primitives
- [ ] Perft test suite
- [ ] `Evaluator` - material only
- [ ] `Negamax` - single-threaded alpha-beta
- [ ] `UCIHandler` - `uci`, `isready`, `position`, `go`, `stop`, `quit`
- [ ] `Main` - stdin/stdout loop
- [ ] CuteChess handshake verified

---

### Phase 1 - Multithreaded Search

#### Lazy SMP Infrastructure
- [ ] `SearchManager` - thread pool, spun up at startup, alive for match duration
- [ ] `SearchWorker` - one per thread
- [ ] `SearchContext` - thread-local: killers, history table, ply stack
- [ ] `TranspositionTable` - flat `long[]`, lockless XOR hashing, power-of-two size
- [ ] `volatile boolean stopFlag` - checked every node
- [ ] Thread count from `availableProcessors()`
- [ ] TT size via UCI `setoption name Hash`

#### Search Algorithms
- [ ] Iterative deepening
- [ ] Principal variation search (PVS)
- [ ] Aspiration windows
- [ ] Null move pruning
- [ ] Late move reductions (LMR)
- [ ] Futility pruning
- [ ] Quiescence search
- [ ] Singular extensions

#### Move Ordering
- [ ] MVV-LVA
- [ ] Killer move heuristic
- [ ] History heuristic
- [ ] Countermove heuristic
- [ ] History malus

#### Evaluation
- [ ] Piece-square tables
- [ ] Tapered eval
- [ ] Pawn structure: passed, isolated, doubled, chains
- [ ] King safety: attack units, pawn shield, open files
- [ ] Mobility
- [ ] Tempo bonus

---

### Phase 2 - Performance
- [ ] Magic bitboards
- [ ] PEXT bitboards (BMI2, fallback to magics)
- [ ] GraalVM native binary: `--no-fallback`, `-O3`, `-march=native`
- [ ] ARM64 build for Pi 5
- [ ] Time management: soft/hard limits
- [ ] Lichess deployment via lichess-bot

---

### Phase 3 - NNUE
- [ ] Self-play position generation pipeline
- [ ] HalfKP / HalfKAv2 feature set
- [ ] Network architecture: sparse input (~256-512 neurons), small dense layers (~32 neurons)
- [ ] Incremental accumulator updates
- [ ] PyTorch training pipeline
- [ ] Weight export to binary format
- [ ] NNUE inference in Java: no framework, GraalVM-safe
- [ ] Iterative self-improvement loop
- [ ] OpenBench integration

---

### Phase 4 - Tablebases
- [ ] Syzygy tablebase probing
- [ ] 5-piece minimum, 7-piece if storage allows
- [ ] UCI `setoption name SyzygyPath`
- [ ] Pi 5 SSD mount

---

### Phase 5 - Personality
- [ ] Contempt factor via UCI `setoption`
- [ ] ELO-aware playstyle adjustment
  - [ ] Drawish vs stronger opponents
  - [ ] Chaotic vs weaker opponents
- [ ] Janus commentator CLI
  - [ ] PGN input
  - [ ] Anthropic API: arrogant Roman god system prompt
  - [ ] Optional Lichess study posting

---

## Architecture

```
src/main/java/engine/
├── Main.java
├── board/
│   ├── Board.java
│   ├── MoveGenerator.java
│   └── MoveEncoder.java
├── search/
│   ├── SearchManager.java
│   ├── SearchWorker.java
│   ├── SearchContext.java
│   └── Negamax.java
├── tt/
│   └── TranspositionTable.java
├── eval/
│   ├── Evaluator.java
│   └── NNUE.java
├── uci/
│   └── UCIHandler.java
└── util/
    └── Bits.java
```

---

## Build

```bash
# JVM (development)
./gradlew run

# Native binary
./gradlew nativeCompile

# Output
./build/native/nativeCompile/janus
```

---

## Deployment

**CuteChess:** point directly at native binary.

**Lichess:** lichess-bot on host machine, UCI over stdin/stdout.

**Pi 5:** compile on-device or SCP ARM64 binary. Set TT to 256-512MB via UCI options.

---

## ELO Milestones

| Milestone                      | Target ELO  |
|--------------------------------|-------------|
| Phase 0 complete               | ~800        |
| Move ordering + quiescence     | ~1400       |
| Full classical eval + pruning  | ~1800-2000  |
| Magic/PEXT + PVS + aspiration  | ~2100-2200  |
| NNUE (initial)                 | ~2500-2700  |
| NNUE (iterated)                | ~2800-3000  |

---

*"Forethought beats afterthought. Always."*