# Warrior multiplayer integration probe

Fresh single-process listen-server PIE, two players. RPC requests originate from the owning client. Fixture changes affect PIE worlds only; stop PIE after running.

- PASS: Distinct authoritative server pawn and owning client pawn
- PASS: Distinct replicated enemy instances
- FAIL: Stage 2 timed out; server rage 5.00, client rage 5.00, server HP 9990.00, client HP 9990.00

Result: FAIL (1 failures).
