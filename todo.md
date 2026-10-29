## TODO:
### Bugs:
- Fix PV bug at depth 9 and 10 where PV stops being printed (TT issue?). Fixed in version 3.1.0 [done]

- Evaluate effectiveness of encouraging castling

### Features:

- Introduce UCI flag for opening variation - and have it randomly pick a first move (as white) from a list of valid opening moves and send that regardless of eval.

- UCI move overhead

- Teach engine about king safety

- Encourage opposite side castling to opponent (under a "personality" UCI flag?)

- Magic bitboards

- Evolutionary algorithm to find optimal PST values

- Make UCI say version in handshake