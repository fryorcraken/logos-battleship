#include "libbattleship.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Ship sizes ─────────────────────────────────────────────────── */

static const int SHIP_SIZES[BS_NUM_SHIPS] = { 5, 4, 3, 3, 2 };

/* ── Internal data structures ───────────────────────────────────── */

typedef struct {
    BsShipType type;
    int        row, col;
    BsDirection dir;
    int        size;
    int        hits;       /* number of cells hit */
    int        placed;     /* 1 if placed on the board */
} Ship;

typedef struct {
    /* own_board: what is physically on this player's grid.
     * 0 = water, ship_index+1 = ship present (1..5) */
    int own_board[BS_GRID_SIZE][BS_GRID_SIZE];

    /* own_hits: cells where the opponent has attacked this player.
     * 0 = not attacked, 1 = attacked */
    int own_hits[BS_GRID_SIZE][BS_GRID_SIZE];

    /* attack_board: what this player knows about the opponent's grid.
     * BsAttackCell values. */
    BsAttackCell attack_board[BS_GRID_SIZE][BS_GRID_SIZE];

    Ship ships[BS_NUM_SHIPS];
    int  ships_placed;  /* count of ships placed */

    int  hits;          /* shots by this player that hit */
    int  misses;        /* shots by this player that missed */
} PlayerState;

struct BsGame {
    PlayerState players[2];   /* 0 = human, 1 = AI */
    BsGameStatus status;
    int current_player;       /* 0 = human, 1 = AI */
    int turn_count;
    int rng_seeded;
};

/* ── Helpers ────────────────────────────────────────────────────── */

static int in_bounds(int row, int col) {
    return row >= 0 && row < BS_GRID_SIZE &&
           col >= 0 && col < BS_GRID_SIZE;
}

static void ensure_rng(BsGame* game) {
    if (!game->rng_seeded) {
        srand((unsigned)time(NULL));
        game->rng_seeded = 1;
    }
}

static void init_player(PlayerState* p) {
    memset(p->own_board, 0, sizeof(p->own_board));
    memset(p->own_hits, 0, sizeof(p->own_hits));
    memset(p->attack_board, 0, sizeof(p->attack_board));
    for (int i = 0; i < BS_NUM_SHIPS; i++) {
        p->ships[i].type = (BsShipType)i;
        p->ships[i].size = SHIP_SIZES[i];
        p->ships[i].hits = 0;
        p->ships[i].placed = 0;
        p->ships[i].row = 0;
        p->ships[i].col = 0;
        p->ships[i].dir = BS_DIR_HORIZONTAL;
    }
    p->ships_placed = 0;
    p->hits = 0;
    p->misses = 0;
}

/**
 * After sinking a ship, mark all its cells as SUNK on the attacker's
 * attack_board so the UI can distinguish sunk from merely hit.
 */
static void mark_sunk_on_attack_board(BsAttackCell attack_board[BS_GRID_SIZE][BS_GRID_SIZE],
                                       const Ship* ship) {
    for (int i = 0; i < ship->size; i++) {
        int r = ship->row + (ship->dir == BS_DIR_VERTICAL   ? i : 0);
        int c = ship->col + (ship->dir == BS_DIR_HORIZONTAL ? i : 0);
        attack_board[r][c] = BS_CELL_SUNK;
    }
}

/* ── Lifecycle ──────────────────────────────────────────────────── */

BsGame* bs_new(void) {
    BsGame* game = (BsGame*)calloc(1, sizeof(BsGame));
    if (!game) return NULL;
    init_player(&game->players[0]);
    init_player(&game->players[1]);
    game->status = BS_STATUS_PLACING;
    game->current_player = 0;
    game->turn_count = 0;
    game->rng_seeded = 0;
    return game;
}

void bs_free(BsGame* game) {
    free(game);
}

void bs_reset(BsGame* game) {
    if (!game) return;
    int seeded = game->rng_seeded;
    init_player(&game->players[0]);
    init_player(&game->players[1]);
    game->status = BS_STATUS_PLACING;
    game->current_player = 0;
    game->turn_count = 0;
    game->rng_seeded = seeded;
}

/* ── Ship placement ─────────────────────────────────────────────── */

int bs_ship_size(BsShipType ship) {
    if (ship < 0 || ship >= BS_NUM_SHIPS) return 0;
    return SHIP_SIZES[ship];
}

BsError bs_place_ship(BsGame* game, int player,
                      BsShipType ship, int row, int col,
                      BsDirection dir) {
    if (!game) return BS_ERR_NULL_GAME;
    if (player < 0 || player > 1) return BS_ERR_INVALID_POSITION;
    if (ship < 0 || ship >= BS_NUM_SHIPS) return BS_ERR_INVALID_POSITION;

    PlayerState* p = &game->players[player];
    Ship* s = &p->ships[ship];
    int size = SHIP_SIZES[ship];

    /* Check bounds */
    if (!in_bounds(row, col)) return BS_ERR_PLACEMENT_OOB;
    int end_row = row + (dir == BS_DIR_VERTICAL   ? size - 1 : 0);
    int end_col = col + (dir == BS_DIR_HORIZONTAL ? size - 1 : 0);
    if (!in_bounds(end_row, end_col)) return BS_ERR_PLACEMENT_OOB;

    /* Check overlap */
    for (int i = 0; i < size; i++) {
        int r = row + (dir == BS_DIR_VERTICAL   ? i : 0);
        int c = col + (dir == BS_DIR_HORIZONTAL ? i : 0);
        if (p->own_board[r][c] != 0) return BS_ERR_PLACEMENT_OVERLAP;
    }

    /* Place ship */
    /* If ship was already placed, clear old position first */
    if (s->placed) {
        for (int i = 0; i < s->size; i++) {
            int r = s->row + (s->dir == BS_DIR_VERTICAL   ? i : 0);
            int c = s->col + (s->dir == BS_DIR_HORIZONTAL ? i : 0);
            p->own_board[r][c] = 0;
        }
    } else {
        p->ships_placed++;
    }

    s->row = row;
    s->col = col;
    s->dir = dir;
    s->placed = 1;
    s->hits = 0;

    for (int i = 0; i < size; i++) {
        int r = row + (dir == BS_DIR_VERTICAL   ? i : 0);
        int c = col + (dir == BS_DIR_HORIZONTAL ? i : 0);
        p->own_board[r][c] = (int)ship + 1;  /* 1-indexed */
    }

    return BS_OK;
}

void bs_place_ships_random(BsGame* game, int player) {
    if (!game) return;
    if (player < 0 || player > 1) return;

    ensure_rng(game);
    PlayerState* p = &game->players[player];

    /* Clear any existing placement */
    memset(p->own_board, 0, sizeof(p->own_board));
    for (int i = 0; i < BS_NUM_SHIPS; i++) {
        p->ships[i].placed = 0;
    }
    p->ships_placed = 0;

    for (int i = 0; i < BS_NUM_SHIPS; i++) {
        BsShipType stype = (BsShipType)i;
        BsError err;
        do {
            int row = rand() % BS_GRID_SIZE;
            int col = rand() % BS_GRID_SIZE;
            BsDirection dir = (rand() % 2 == 0) ? BS_DIR_HORIZONTAL : BS_DIR_VERTICAL;
            err = bs_place_ship(game, player, stype, row, col, dir);
        } while (err != BS_OK);
    }
}

int bs_all_ships_placed(const BsGame* game) {
    if (!game) return 0;
    return game->players[0].ships_placed == BS_NUM_SHIPS &&
           game->players[1].ships_placed == BS_NUM_SHIPS;
}

/* ── Gameplay ───────────────────────────────────────────────────── */

void bs_start(BsGame* game) {
    if (!game) return;
    if (bs_all_ships_placed(game)) {
        game->status = BS_STATUS_PLAYING;
        game->current_player = 0;
        game->turn_count = 0;
    }
}

/**
 * Internal: player `attacker` fires at (row,col) on `defender`'s board.
 */
static BsError do_attack(BsGame* game, int attacker, int defender,
                          int row, int col, BsShotResult* result) {
    if (!game) return BS_ERR_NULL_GAME;
    if (!in_bounds(row, col)) return BS_ERR_INVALID_POSITION;
    if (game->status != BS_STATUS_PLAYING) return BS_ERR_GAME_OVER;

    PlayerState* atk = &game->players[attacker];
    PlayerState* def = &game->players[defender];

    /* Already attacked? */
    if (atk->attack_board[row][col] != BS_CELL_UNKNOWN)
        return BS_ERR_ALREADY_ATTACKED;

    int cell = def->own_board[row][col];
    if (cell == 0) {
        /* Miss */
        atk->attack_board[row][col] = BS_CELL_MISS;
        def->own_hits[row][col] = 1;
        atk->misses++;
        if (result) *result = BS_SHOT_MISS;
    } else {
        /* Hit — cell value is ship_index + 1 */
        int ship_idx = cell - 1;
        Ship* ship = &def->ships[ship_idx];
        ship->hits++;
        def->own_hits[row][col] = 1;
        atk->hits++;

        if (ship->hits >= ship->size) {
            /* Sunk! */
            mark_sunk_on_attack_board(atk->attack_board, ship);
            if (result) *result = BS_SHOT_SUNK;

            /* Check win */
            int all_sunk = 1;
            for (int i = 0; i < BS_NUM_SHIPS; i++) {
                if (def->ships[i].hits < def->ships[i].size) {
                    all_sunk = 0;
                    break;
                }
            }
            if (all_sunk) {
                game->status = (attacker == 0) ? BS_STATUS_WON : BS_STATUS_LOST;
            }
        } else {
            atk->attack_board[row][col] = BS_CELL_HIT;
            if (result) *result = BS_SHOT_HIT;
        }
    }

    return BS_OK;
}

BsError bs_attack(BsGame* game, int row, int col, BsShotResult* result) {
    if (!game) return BS_ERR_NULL_GAME;
    if (game->current_player != 0) return BS_ERR_NOT_YOUR_TURN;

    BsError err = do_attack(game, 0, 1, row, col, result);
    if (err != BS_OK) return err;

    game->turn_count++;

    /* Switch turn to AI (unless game is over) */
    if (game->status == BS_STATUS_PLAYING) {
        game->current_player = 1;
    }

    return BS_OK;
}

int bs_current_player(const BsGame* game) {
    if (!game) return 0;
    return game->current_player;
}

BsGameStatus bs_status(const BsGame* game) {
    if (!game) return BS_STATUS_PLACING;
    return game->status;
}

/* ── AI ─────────────────────────────────────────────────────────── */

void bs_ai_attack(BsGame* game, int* out_row, int* out_col,
                  BsShotResult* out_result) {
    if (!game) return;
    if (game->status != BS_STATUS_PLAYING) return;
    if (game->current_player != 1) return;

    ensure_rng(game);
    PlayerState* ai = &game->players[1];

    /* Collect un-attacked cells */
    int candidates[BS_GRID_SIZE * BS_GRID_SIZE][2];
    int count = 0;
    for (int r = 0; r < BS_GRID_SIZE; r++) {
        for (int c = 0; c < BS_GRID_SIZE; c++) {
            if (ai->attack_board[r][c] == BS_CELL_UNKNOWN) {
                candidates[count][0] = r;
                candidates[count][1] = c;
                count++;
            }
        }
    }

    if (count == 0) return;  /* should never happen in a valid game */

    int idx = rand() % count;
    int row = candidates[idx][0];
    int col = candidates[idx][1];

    BsShotResult result = BS_SHOT_MISS;
    do_attack(game, 1, 0, row, col, &result);

    if (out_row) *out_row = row;
    if (out_col) *out_col = col;
    if (out_result) *out_result = result;

    /* Switch turn back to human (unless game is over) */
    if (game->status == BS_STATUS_PLAYING) {
        game->current_player = 0;
    }
}

/* ── Board queries ──────────────────────────────────────────────── */

BsOwnCell bs_get_own_cell(const BsGame* game, int row, int col) {
    if (!game || !in_bounds(row, col)) return BS_OWN_WATER;
    const PlayerState* human = &game->players[0];
    if (human->own_hits[row][col]) {
        if (human->own_board[row][col] != 0)
            return BS_OWN_HIT;
        else
            return BS_OWN_WATER;  /* opponent attacked water */
    }
    return (human->own_board[row][col] != 0) ? BS_OWN_SHIP : BS_OWN_WATER;
}

BsAttackCell bs_get_attack_cell(const BsGame* game, int row, int col) {
    if (!game || !in_bounds(row, col)) return BS_CELL_UNKNOWN;
    return game->players[0].attack_board[row][col];
}

/* ── Ship queries ───────────────────────────────────────────────── */

int bs_ship_sunk(const BsGame* game, int player, BsShipType ship) {
    if (!game || player < 0 || player > 1 ||
        ship < 0 || ship >= BS_NUM_SHIPS) return 0;
    const Ship* s = &game->players[player].ships[ship];
    return (s->placed && s->hits >= s->size) ? 1 : 0;
}

int bs_ships_remaining(const BsGame* game, int player) {
    if (!game || player < 0 || player > 1) return 0;
    int count = 0;
    for (int i = 0; i < BS_NUM_SHIPS; i++) {
        if (game->players[player].ships[i].placed &&
            game->players[player].ships[i].hits < game->players[player].ships[i].size) {
            count++;
        }
    }
    return count;
}

/* ── Stats ──────────────────────────────────────────────────────── */

int bs_turn_count(const BsGame* game) {
    if (!game) return 0;
    return game->turn_count;
}

int bs_hit_count(const BsGame* game, int player) {
    if (!game || player < 0 || player > 1) return 0;
    return game->players[player].hits;
}

int bs_miss_count(const BsGame* game, int player) {
    if (!game || player < 0 || player > 1) return 0;
    return game->players[player].misses;
}
