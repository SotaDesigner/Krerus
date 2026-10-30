# Warrior multiplayer integration probe

Fresh single-process listen-server PIE, two players. RPC requests originate from the owning client. Fixture changes affect PIE worlds only; stop PIE after running.

- PASS: Distinct authoritative server pawn and owning client pawn
- PASS: Distinct replicated enemy instances
- PASS: Three real montage impacts yield exactly +15 rage; server/client 15.00/15.00
- PASS: Autoattack damage applied once per impact; delta 30.000, expected 30.000
- PASS: Autoattack rage, enemy health and armor replicate to owner
- PASS: First client strike consumes exactly 15 rage
- PASS: First strike deals 10 damage against armor 50; delta 10.000
- PASS: Fracture reduces armor from 50 to 42.5
- PASS: Strike cost, health and fractured armor replicate to owner
- PASS: Reapplication does not stack armor reduction
- PASS: Second strike uses current fractured armor for mitigation
- PASS: Reapplication and second cost replicate to owner
- PASS: Fracture remains active past original 10-second expiry on server and client
- PASS: Armor returns to 50 after refreshed duration on server and client
- PASS: Server rejects client strike with insufficient rage without damage/debuff
- PASS: Server rejects out-of-range client strike without resource cost

Result: PASS (0 failures).
