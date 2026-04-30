# Spec: Logos Battleship

## Objective

Build a Battleship game as a Logos basecamp module, demonstrating the Logos modular
desktop platform. Phase 1 is a local game against a simple AI. Phase 2 (future)
adds P2P multiplayer via the delivery module with a commit-reveal scheme.

The game is shown on a livestream, so the UI should expose "under the hood" info
(game state, turn count, ship health) and be visually clear at screen-share
resolution.

**User story:** A user opens Logos Basecamp, clicks the Battleship icon in the
sidebar, sees two 10x10 grids (their fleet + attack board), and plays a
turn-based game against an AI opponent.

## Game Rules (Standard Battleship)

- Two players: Human and AI.
- Each player has a 10x10 grid.
- Each player places 5 ships (no overlap, no out-of-bounds):

  | Ship       | Size |
  |------------|------|
  | Carrier    | 5    |
  | Battleship | 4    |
  | Cruiser    | 3    |
  | Submarine  | 3    |
  | Destroyer  | 2    |

- Ships are placed horizontally or vertically.
- Players alternate turns. Human fires first.
- On each turn, a player selects a cell on the opponent's grid to attack.
- Result is **hit** (ship occupies that cell) or **miss** (water).
- When all cells of a ship are hit, the ship is **sunk**. The opponent is notified
  which ship was sunk.
- When all 5 ships of a player are sunk, the other player **wins**.

### Phase 1 simplifications

- Ship placement is **random** for both human and AI (no manual placement).
- AI targeting is **random** (picks a random un-attacked cell).
- No multiplayer, no delivery module dependency.

## Tech Stack

- **C99**: `libbattleship` -- pure game logic library (no Qt, no I/O)
- **C++17 / Qt 6**: `battleship_plugin` -- Logos core module wrapping the C library
- **QML**: `battleship_ui_qml` -- declarative UI calling the core module via `logos.callModule()`
- **Nix flakes**: reproducible builds via `logos-module-builder/tutorial-v1`
- **CMake**: C/C++ build, using `logos_module()` macro
- **logos-scaffold-basecamp**: `lgs basecamp` for build/install/launch

## Project Structure

```
logos-battleship/
  docs/
    spec.md                        # This file
  scaffold.toml                    # lgs basecamp config (generated)
  battleship/                      # Core module (C lib + C++ plugin)
    metadata.json
    flake.nix
    CMakeLists.txt
    lib/
      libbattleship.h              # C API: opaque handles, enums, functions
      libbattleship.c              # C implementation: boards, ships, AI, hit/miss
    src/
      battleship_interface.h       # Abstract Qt interface (Q_INVOKABLE methods)
      battleship_plugin.h          # Plugin class header
      battleship_plugin.cpp        # Plugin implementation
    tests/
      CMakeLists.txt               # Standalone test build
      test_libbattleship.c         # C-level unit tests
  battleship-ui-qml/               # QML UI module
    metadata.json
    flake.nix
    Main.qml                       # Full UI
    icons/battleship.png
```

## C Library API (`libbattleship.h`)

### Data Model

```c
// Cell state as seen by the opponent (attack board)
typedef enum {
    BS_CELL_UNKNOWN = 0,   // not yet attacked
    BS_CELL_MISS    = 1,   // attacked, no ship
    BS_CELL_HIT     = 2,   // attacked, ship present
    BS_CELL_SUNK    = 3    // attacked, ship fully sunk (all cells hit)
} BsAttackCell;

// Cell state on own board
typedef enum {
    BS_OWN_WATER = 0,      // no ship
    BS_OWN_SHIP  = 1,      // ship present, not hit
    BS_OWN_HIT   = 2       // ship present, hit
} BsOwnCell;

typedef enum {
    BS_SHIP_CARRIER    = 0,  // size 5
    BS_SHIP_BATTLESHIP = 1,  // size 4
    BS_SHIP_CRUISER    = 2,  // size 3
    BS_SHIP_SUBMARINE  = 3,  // size 3
    BS_SHIP_DESTROYER  = 4   // size 2
} BsShipType;
#define BS_NUM_SHIPS 5

typedef enum {
    BS_STATUS_PLACING   = 0,  // ships being placed
    BS_STATUS_PLAYING   = 1,  // game in progress
    BS_STATUS_WON       = 2,  // this player won
    BS_STATUS_LOST      = 3   // this player lost
} BsGameStatus;

typedef enum {
    BS_SHOT_MISS = 0,
    BS_SHOT_HIT  = 1,
    BS_SHOT_SUNK = 2      // hit that sinks the ship
} BsShotResult;

typedef enum {
    BS_OK                     = 0,
    BS_ERR_NULL_GAME          = 1,
    BS_ERR_INVALID_POSITION   = 2,  // row/col out of [0,9]
    BS_ERR_ALREADY_ATTACKED   = 3,  // cell was already targeted
    BS_ERR_GAME_OVER          = 4,
    BS_ERR_NOT_YOUR_TURN      = 5,
    BS_ERR_PLACEMENT_OVERLAP  = 6,
    BS_ERR_PLACEMENT_OOB      = 7   // ship goes out of bounds
} BsError;

typedef enum {
    BS_DIR_HORIZONTAL = 0,
    BS_DIR_VERTICAL   = 1
} BsDirection;

// Opaque game handle
typedef struct BsGame BsGame;
```

### Functions

```c
// Lifecycle
BsGame*      bs_new(void);                              // allocate game
void         bs_free(BsGame* game);                     // deallocate
void         bs_reset(BsGame* game);                    // reset to initial state

// Ship placement
BsError      bs_place_ship(BsGame* game, int player,    // player: 0=human, 1=AI
                           BsShipType ship,
                           int row, int col,
                           BsDirection dir);
void         bs_place_ships_random(BsGame* game,         // random valid placement
                                   int player);
int          bs_all_ships_placed(BsGame* game);          // 1 if both players done

// Gameplay
void         bs_start(BsGame* game);                     // transition to PLAYING
BsError      bs_attack(BsGame* game,                     // fire at opponent
                       int row, int col,
                       BsShotResult* result);             // out: result of shot
int          bs_current_player(const BsGame* game);      // 0=human, 1=AI
BsGameStatus bs_status(const BsGame* game);

// AI
void         bs_ai_attack(BsGame* game,                  // AI picks & fires
                          int* out_row, int* out_col,
                          BsShotResult* out_result);

// Board queries
BsOwnCell    bs_get_own_cell(const BsGame* game,         // human's own board
                              int row, int col);
BsAttackCell bs_get_attack_cell(const BsGame* game,      // human's view of AI board
                                 int row, int col);

// Ship queries
int          bs_ship_size(BsShipType ship);               // static: ship size
int          bs_ship_sunk(const BsGame* game,             // is ship sunk?
                          int player, BsShipType ship);
int          bs_ships_remaining(const BsGame* game,       // count alive ships
                                int player);

// Stats
int          bs_turn_count(const BsGame* game);
int          bs_hit_count(const BsGame* game, int player);
int          bs_miss_count(const BsGame* game, int player);
```

## Core Module Interface (`battleship_interface.h`)

All values passed as `int` to avoid QML needing C headers.

```
Q_INVOKABLE void newGame()             -- reset + random place + start
Q_INVOKABLE int  attack(int row, int col)  -- human attacks; returns BsError
Q_INVOKABLE int  lastShotResult()      -- BsShotResult of last human attack
Q_INVOKABLE int  aiAttackRow()         -- row of last AI attack
Q_INVOKABLE int  aiAttackCol()         -- col of last AI attack
Q_INVOKABLE int  aiShotResult()        -- BsShotResult of last AI attack
Q_INVOKABLE int  status()              -- BsGameStatus
Q_INVOKABLE int  currentPlayer()       -- 0=human, 1=AI
Q_INVOKABLE int  getOwnCell(int r, int c)     -- BsOwnCell
Q_INVOKABLE int  getAttackCell(int r, int c)  -- BsAttackCell
Q_INVOKABLE int  shipSunk(int player, int shipType)  -- 0/1
Q_INVOKABLE int  shipsRemaining(int player)
Q_INVOKABLE int  turnCount()
Q_INVOKABLE int  hitCount(int player)
Q_INVOKABLE int  missCount(int player)
```

**`newGame()` flow:**
1. `bs_reset(game)`
2. `bs_place_ships_random(game, 0)` (human)
3. `bs_place_ships_random(game, 1)` (AI)
4. `bs_start(game)`

**`attack(row, col)` flow:**
1. Validate it's human's turn
2. `bs_attack(game, row, col, &result)` -- human fires
3. Store `lastShotResult`
4. If game not over and AI's turn: `bs_ai_attack(game, &r, &c, &res)` -- AI fires back
5. Store AI attack info
6. Emit `eventResponse("attacked", [row, col, result])`
7. If AI attacked, emit `eventResponse("aiAttacked", [aiRow, aiCol, aiResult])`
8. Return error code

## QML UI Layout

```
┌────────────────────────────────────────────────────────┐
│                    BATTLESHIP                           │
│                                                        │
│   Your Fleet              Enemy Waters                 │
│  ┌──────────┐            ┌──────────┐                  │
│  │ 10x10    │            │ 10x10    │                  │
│  │ grid     │            │ grid     │                  │
│  │ showing  │            │ showing  │                  │
│  │ own      │            │ attack   │                  │
│  │ ships +  │            │ results  │                  │
│  │ hits     │            │ (click   │                  │
│  │          │            │  to fire)│                  │
│  └──────────┘            └──────────┘                  │
│                                                        │
│  Status: Your turn / AI's turn / You win! / You lose!  │
│                                                        │
│  ── Under the Hood ──────────────────────────          │
│  Turn: 12  |  Your hits: 5/17  |  AI hits: 3/17       │
│  Your ships: 3/5 alive  |  AI ships: 4/5 alive        │
│  [New Game]                                            │
└────────────────────────────────────────────────────────┘
```

**Grid cells:**
- Own board: water=dark blue, ship=gray, hit=red, miss=impossible (own board)
- Attack board: unknown=dark, miss=white dot, hit=red X, sunk=red filled

**Colors (dark theme matching tictactoe):**
- Background: `#1a1a1a`
- Grid cell: `#2a2a2a` border `#555`
- Water/unknown: `#1a2a3a` (slight blue tint)
- Ship (own): `#4a4a4a`
- Hit: `#ff4444`
- Miss: `#666` with dot
- Sunk: `#ff2222` filled
- Text: `#e0e0e0`

## Commands

```bash
# Build & install all modules into alice/bob profiles
lgs basecamp install

# Launch basecamp (two terminals for future P2P testing)
lgs basecamp launch alice

# Build a single module during development
nix build ./battleship
nix build ./battleship-ui-qml

# Run C library tests (no Logos toolchain needed)
cd battleship/tests && mkdir -p build && cd build && cmake .. && make && ./test_libbattleship
```

## Testing Strategy

- **C library tests** (`battleship/tests/test_libbattleship.c`):
  standalone, no Qt dependency. Covers: placement validation, attack
  hit/miss/sunk, win detection, turn enforcement, out-of-bounds, AI never
  attacks same cell twice, random placement produces valid boards.
- **Manual testing**: launch basecamp, play through a full game, verify all
  states visually.

## Boundaries

### Always
- Run C tests before committing changes to libbattleship
- Keep game logic in pure C with no Qt/platform dependency
- Follow tictactoe's Nix/CMake patterns exactly

### Ask first
- Adding delivery_module dependency (Phase 2)
- Changing the C API surface
- Adding new ship types or grid sizes

### Never
- Put game logic in the QML UI or the C++ plugin (plugin is a thin wrapper)
- Hardcode grid size (use `#define BS_GRID_SIZE 10`, even though we won't change it in Phase 1)
- Commit secrets or credentials

## Success Criteria

- [ ] `lgs basecamp install` succeeds (both modules build and install)
- [ ] `lgs basecamp launch alice` shows Battleship in the sidebar
- [ ] Clicking a cell on the attack grid fires and shows hit/miss
- [ ] AI responds with its own attack immediately after human's turn
- [ ] Sunk ships are visually distinguishable from regular hits
- [ ] Game correctly detects win/loss when all 5 ships sunk
- [ ] New Game resets everything
- [ ] "Under the hood" stats update live
- [ ] C library tests all pass

## Open Questions

- None blocking Phase 1. Phase 2 questions (delivery protocol, commit-reveal
  wire format, session scoping) are deferred.

## Phase 2 Preview (Not in Scope)

- Add `delivery_module` dependency to core module
- Add protobuf wire format for attacks and results
- Implement commit-reveal: players hash their board at game start, exchange
  hashes, reveal boards at end for verification
- Add multiplayer UI controls (connect/disconnect, delivery status)
- Session scoping via unique game ID in the content topic
