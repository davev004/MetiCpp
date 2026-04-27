# Janus

A UCI-compatible Java chess engine compiled to native binary via GraalVM. Named after the two-faced Roman god of foresight. Built to compete with (C)Epimetheus.

For full development roadmap see [ROADMAP.md](ROADMAP.md)

---

## Design Principles
- **Multithreading from the ground up:** Lazy SMP architecture, not retrofitted
- **Zero heap allocation in search:** primitives and arrays only in the hot path
- **Lockless shared state:** XOR hashing on the transposition table, no synchronisation overhead
- **Native binary:** GraalVM Native Image, `-march=native`

---

## Planned Features
- [ ] UCI compatibility
- [ ] Lazy SMP multithreaded search
- [ ] Classical evaluation: PSTs, pawn structure, king safety, mobility
- [ ] Magic / PEXT bitboards
- [ ] PVS, aspiration windows, LMR, null move, singular extensions
- [ ] NNUE evaluation
- [ ] Syzygy tablebase support
- [ ] Lichess deployment
- [ ] ELO-aware playstyle adjustment
- [ ] Janus commentator CLI

---

## Build
```bash
./gradlew nativeCompile
./build/native/nativeCompile/janus
```

---

## ELO Milestones

| Milestone                     | Target ELO |
|-------------------------------|------------|
| Phase 0 complete              | ~800       |
| Move ordering + quiescence    | ~1400      |
| Full classical eval + pruning | ~1800-2000 |
| Magic/PEXT + PVS + aspiration | ~2100-2200 |
| NNUE (initial)                | ~2500-2700 |
| NNUE (iterated)               | ~2800-3000 |

---

*"Forethought beats afterthought. Always."*