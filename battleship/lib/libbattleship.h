#ifndef LIBBATTLESHIP_H
#define LIBBATTLESHIP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BS_GRID_SIZE 10
#define BS_NUM_SHIPS 5

/* ── Cell enums ─────────────────────────────────────────────────── */

/** Cell state on the attack board (what we know about the opponent). */
typedef enum {
    BS_CELL_UNKNOWN = 0,
    BS_CELL_MISS    = 1,
    BS_CELL_HIT     = 2,
    BS_CELL_SUNK    = 3
} BsAttackCell;

/** Cell state on own board. */
typedef enum {
    BS_OWN_WATER = 0,
    BS_OWN_SHIP  = 1,
    BS_OWN_HIT   = 2
} BsOwnCell;

/* ── Ship types ─────────────────────────────────────────────────── */

typedef enum {
    BS_SHIP_CARRIER    = 0,   /* size 5 */
    BS_SHIP_BATTLESHIP = 1,   /* size 4 */
    BS_SHIP_CRUISER    = 2,   /* size 3 */
    BS_SHIP_SUBMARINE  = 3,   /* size 3 */
    BS_SHIP_DESTROYER  = 4    /* size 2 */
} BsShipType;

typedef enum {
    BS_DIR_HORIZONTAL = 0,
    BS_DIR_VERTICAL   = 1
} BsDirection;

/* ── Game status ────────────────────────────────────────────────── */

typedef enum {
    BS_STATUS_PLACING = 0,
    BS_STATUS_PLAYING = 1,
    BS_STATUS_WON     = 2,   /* human won  */
    BS_STATUS_LOST    = 3    /* human lost */
} BsGameStatus;

/* ── Shot result ────────────────────────────────────────────────── */

typedef enum {
    BS_SHOT_MISS = 0,
    BS_SHOT_HIT  = 1,
    BS_SHOT_SUNK = 2
} BsShotResult;

/* ── Error codes ────────────────────────────────────────────────── */

typedef enum {
    BS_OK                    = 0,
    BS_ERR_NULL_GAME         = 1,
    BS_ERR_INVALID_POSITION  = 2,
    BS_ERR_ALREADY_ATTACKED  = 3,
    BS_ERR_GAME_OVER         = 4,
    BS_ERR_NOT_YOUR_TURN     = 5,
    BS_ERR_PLACEMENT_OVERLAP = 6,
    BS_ERR_PLACEMENT_OOB     = 7
} BsError;

/* ── Opaque game handle ─────────────────────────────────────────── */

typedef struct BsGame BsGame;

/* ── Lifecycle ──────────────────────────────────────────────────── */

BsGame* bs_new(void);
void    bs_free(BsGame* game);
void    bs_reset(BsGame* game);

/* ── Ship placement ─────────────────────────────────────────────── */

/**
 * Place a ship for the given player.
 * @param player  0 = human, 1 = AI
 */
BsError bs_place_ship(BsGame* game, int player,
                      BsShipType ship, int row, int col,
                      BsDirection dir);

/** Randomly place all 5 ships for the given player. */
void bs_place_ships_random(BsGame* game, int player);

/** Returns 1 if both players have all ships placed. */
int bs_all_ships_placed(const BsGame* game);

/* ── Gameplay ───────────────────────────────────────────────────── */

/** Transition from PLACING to PLAYING. */
void bs_start(BsGame* game);

/**
 * Human attacks the AI's board at (row, col).
 * On success, *result is set to the shot outcome.
 */
BsError bs_attack(BsGame* game, int row, int col, BsShotResult* result);

/** Returns whose turn it is: 0 = human, 1 = AI. */
int bs_current_player(const BsGame* game);

/** Returns the overall game status. */
BsGameStatus bs_status(const BsGame* game);

/* ── AI ─────────────────────────────────────────────────────────── */

/**
 * AI picks a random un-attacked cell on the human's board and fires.
 * Outputs the chosen row, col, and shot result.
 */
void bs_ai_attack(BsGame* game, int* out_row, int* out_col,
                  BsShotResult* out_result);

/* ── Board queries ──────────────────────────────────────────────── */

/** Query the human's own board (shows ships and incoming hits). */
BsOwnCell bs_get_own_cell(const BsGame* game, int row, int col);

/** Query the human's attack board (what human knows about AI's grid). */
BsAttackCell bs_get_attack_cell(const BsGame* game, int row, int col);

/* ── Ship queries ───────────────────────────────────────────────── */

/** Return the size of a ship type. */
int bs_ship_size(BsShipType ship);

/** Returns 1 if the given ship is sunk. player: 0=human, 1=AI. */
int bs_ship_sunk(const BsGame* game, int player, BsShipType ship);

/** Count of ships still alive. player: 0=human, 1=AI. */
int bs_ships_remaining(const BsGame* game, int player);

/* ── Stats ──────────────────────────────────────────────────────── */

int bs_turn_count(const BsGame* game);
int bs_hit_count(const BsGame* game, int player);
int bs_miss_count(const BsGame* game, int player);

#ifdef __cplusplus
}
#endif

#endif /* LIBBATTLESHIP_H */
