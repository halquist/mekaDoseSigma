# mekaDoseSigma — design bible

Living document for the mecha suit combat game on a **240×240 round touch screen**.
Update this when design decisions change. Cursor agents should read this via `.cursor/rules/mekadose-sigma.mdc`.

---

## Documentation

Do not reference this unless it is very relevant. You can view the github code here: https://github.com/CubeCoders/Jet and the API documentation here: https://cubecoders.github.io/Jet/

## Vision

Top-down or over-the-shoulder mecha arena combat tuned for **one-thumb play** on a circular display.
Player pilots a mech; enemies are other mechs or drones. Sessions are short (1–3 min).

---

## Hard constraints

| Constraint | Implication |
|------------|-------------|
| 240×240 round | Usable UI is a circle; corners are clipped — keep HUD radial or top-arc only |
| Finger occludes center | Do not require aiming at small targets under the thumb |
| No physical buttons | Every action is touch; no chorded multi-touch unless proven on hardware |
| ESP32-S3 + Jet | Prefer simple meshes, low object count, UNLIT UI unless otherwise specified, no expensive post-FX early |
|FPS performance is very important on this limited hardware and all elements should be optimized when possible|

---

## Input philosophy

**Problem:** Touch controls that need visual precision fail because the finger hides the target.

**Mitigations (pick and stick to one scheme per milestone):**

1. **Zone-based (recommended v1)**  
   - Left third: move/turn left  
   - Right third: move/turn right  
   - Center third: fire / melee (with dead zones between zones)  
   - Same pattern as `basic_engine_test_game` `TouchZones` — extend, do not reinvent.

2. **Hold-to-act**  
   - Hold center = fire beam; release = stop.  
   - Steer only from left/right strips while not holding center.

3. **Auto-aim assist**  
   - Nearest enemy in forward cone gets lock-on; player only chooses when to fire.  
   - Essential if projectiles require precision.

**Never:** separate tiny buttons for jump, dash, weapon swap, and aim on first playable build.

**Consider:** complex inputs like directional swipes, circles, other shape inputs, or rythm tapping if it makes sense for gameplay elements

---

## Combat readability

- Player mech: high-contrast silhouette (2–3 colors max).
- Enemy: distinct color from player and from projectiles.
- Player shots: one color; enemy shots: another (reuse projectile pool material fix from reference).
- Damage: screen-edge red flash + numeric HP bar (top arc), not floating damage numbers.
- Death: hide mesh immediately; particle burst at last position.

---

## Camera & world

- Camera follows player in third-person chase camera; player stays near screen center bottom.
- Arena smaller than infinite runner — bounded or wrapping grid so orientation stays clear.

---

## Milestones

### M0 — Engine port
- [x] ESP-IDF project builds and flashes from `mekaDoseSigma/`
- [x] Display + touch + Jet frame loop (copy patterns from `basic_engine_test_game`)
- [x] Empty scene: grid + one test cube

### M1 — Playable movement
- [x] Mech moves/turns with zone touch input
- [x] Touch zones documented in `input.hpp` with dead zones
- [x] HUD: HP only, top of circle

### M2 — Combat loop
- [x] One enemy, one weapon, hit feedback
- [x] Auto-aim or generous hit boxes (swept collision for fast projectiles)
- [x] Win/lose state + restart tap

### M3 — Environment & polish
- [x] Natural outdoor palette (sky, grass, military mech tones)
- [x] Low-poly procedural terrain (rolling hills, snaps with player)
- [x] Hover units follow terrain height + clearance; ground tank enemy
- [ ] Mech visual polish (reference multi-part ship detail)
- [ ] SFX hooks if time

---

## World generation (`map_config.hpp`, `worldgen.cpp`)

Low-overhead **seeded procedural** content — no stored map, O(1) per cell:

| API | Purpose |
|-----|---------|
| `MapConfig` | `worldSeed`, `MapTheme`, obstacle/enemy rates and distances |
| `WorldGen::terrainHeight` | Theme-specific hills (RURAL default; DESERT/INDUSTRIAL/CITY stubs) |
| `WorldGen::sampleObstacle` | Deterministic pine tree per grid cell from seed |
| `WorldGen::sampleEnemySpawn` | Full-ring spawn 520–720u, not player-forward |

Obstacles use a **cell registry**: cells load when entering range and unload when leaving (+1 cell margin). Same `(seed, cellX, cellZ)` always yields the same prop. Scale is baked into mesh vertices (Jet ignores `transformScale`).

Future map modules: extend `MapConfig` with `difficulty`, `victoryKillCount`, `bossId`; add theme palettes and prop sets in `sampleObstacle`.

---

## Reference code map (`basic_engine_test_game/`)

| Area | Files |
|------|--------|
| Display + main loop | `main/main.cpp` |
| Touch I2C + rotation | `main/game/input.cpp`, `input.hpp` |
| Touch zones | `input.hpp` → `TouchZones` |
| Game loop | `main/game/game.cpp` |
| Projectiles + swept hits | `main/game/projectile.cpp` |
| Jet config | `managed_components/jet/src/JetConfig.hpp` |

---

## Open questions (resolve before M2)

- [x] Ranged v1 with center-strip fire + auto-aim cone
- [x] Single arena (enemy respawns ahead when outrun)

---

## Changelog

| Date | Change |
|------|--------|
| 2026-06-04 | M3 terrain: grass hills, hover/ground enemies, natural palette |
| 2026-06-04 | M2 combat: enemy, auto-aim shots, win/lose + tap restart |
| 2026-06-04 | M1 mech movement, touch zones, HP HUD, low-screen camera |
| 2026-06-04 | M0 engine port complete (grid, test cube, touch spin) |
| 2026-06-04 | Initial design bible + Cursor rule |
