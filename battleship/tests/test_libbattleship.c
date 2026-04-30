#include <stdio.h>
#include <assert.h>
#include "../lib/libbattleship.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("  %-50s ", #name); \
        name(); \
        tests_passed++; \
        printf("PASS\n"); \
    } while (0)

/* ── Test helpers ───────────────────────────────────────────────── */

/** Create a game with both players randomly placed and started. */
static BsGame* make_started_game(void) {
    BsGame* g = bs_new();
    bs_place_ships_random(g, 0);
    bs_place_ships_random(g, 1);
    bs_start(g);
    return g;
}

/* ── Tests ──────────────────────────────────────────────────────── */

static void test_new_free(void) {
    BsGame* g = bs_new();
    assert(g != NULL);
    assert(bs_status(g) == BS_STATUS_PLACING);
    assert(bs_current_player(g) == 0);
    assert(bs_turn_count(g) == 0);
    bs_free(g);
}

static void test_free_null(void) {
    bs_free(NULL);  /* should not crash */
}

static void test_ship_sizes(void) {
    assert(bs_ship_size(BS_SHIP_CARRIER)    == 5);
    assert(bs_ship_size(BS_SHIP_BATTLESHIP) == 4);
    assert(bs_ship_size(BS_SHIP_CRUISER)    == 3);
    assert(bs_ship_size(BS_SHIP_SUBMARINE)  == 3);
    assert(bs_ship_size(BS_SHIP_DESTROYER)  == 2);
}

static void test_place_ship_valid(void) {
    BsGame* g = bs_new();
    BsError err = bs_place_ship(g, 0, BS_SHIP_CARRIER, 0, 0, BS_DIR_HORIZONTAL);
    assert(err == BS_OK);
    /* Carrier should occupy (0,0)-(0,4) */
    assert(bs_get_own_cell(g, 0, 0) == BS_OWN_SHIP);
    assert(bs_get_own_cell(g, 0, 4) == BS_OWN_SHIP);
    assert(bs_get_own_cell(g, 0, 5) == BS_OWN_WATER);
    bs_free(g);
}

static void test_place_ship_vertical(void) {
    BsGame* g = bs_new();
    BsError err = bs_place_ship(g, 0, BS_SHIP_DESTROYER, 7, 3, BS_DIR_VERTICAL);
    assert(err == BS_OK);
    assert(bs_get_own_cell(g, 7, 3) == BS_OWN_SHIP);
    assert(bs_get_own_cell(g, 8, 3) == BS_OWN_SHIP);
    assert(bs_get_own_cell(g, 9, 3) == BS_OWN_WATER);
    bs_free(g);
}

static void test_place_ship_oob(void) {
    BsGame* g = bs_new();
    /* Carrier (size 5) at col 8 horizontal: 8+4=12 > 9 */
    assert(bs_place_ship(g, 0, BS_SHIP_CARRIER, 0, 8, BS_DIR_HORIZONTAL) == BS_ERR_PLACEMENT_OOB);
    /* Carrier at row 8 vertical: 8+4=12 > 9 */
    assert(bs_place_ship(g, 0, BS_SHIP_CARRIER, 8, 0, BS_DIR_VERTICAL) == BS_ERR_PLACEMENT_OOB);
    bs_free(g);
}

static void test_place_ship_overlap(void) {
    BsGame* g = bs_new();
    assert(bs_place_ship(g, 0, BS_SHIP_CARRIER, 0, 0, BS_DIR_HORIZONTAL) == BS_OK);
    /* Battleship at (0,0) overlaps carrier */
    assert(bs_place_ship(g, 0, BS_SHIP_BATTLESHIP, 0, 0, BS_DIR_VERTICAL) == BS_ERR_PLACEMENT_OVERLAP);
    bs_free(g);
}

static void test_random_placement_valid(void) {
    BsGame* g = bs_new();
    bs_place_ships_random(g, 0);
    bs_place_ships_random(g, 1);
    assert(bs_all_ships_placed(g) == 1);

    /* Count ship cells on human's board -- should be 5+4+3+3+2 = 17 */
    int ship_cells = 0;
    for (int r = 0; r < BS_GRID_SIZE; r++)
        for (int c = 0; c < BS_GRID_SIZE; c++)
            if (bs_get_own_cell(g, r, c) == BS_OWN_SHIP)
                ship_cells++;
    assert(ship_cells == 17);
    bs_free(g);
}

static void test_attack_miss(void) {
    BsGame* g = bs_new();
    /* Place carrier at (0,0) for human; nothing at AI (1,0) */
    bs_place_ship(g, 0, BS_SHIP_CARRIER, 0, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_BATTLESHIP, 1, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_CRUISER, 2, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_SUBMARINE, 3, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_DESTROYER, 4, 0, BS_DIR_HORIZONTAL);

    /* Place all AI ships in row 9 to avoid our test attacks */
    bs_place_ship(g, 1, BS_SHIP_CARRIER, 9, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_BATTLESHIP, 9, 5, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_CRUISER, 8, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_SUBMARINE, 8, 3, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_DESTROYER, 8, 6, BS_DIR_HORIZONTAL);

    bs_start(g);
    assert(bs_status(g) == BS_STATUS_PLAYING);

    /* Attack (0,0) on AI board -- AI has no ship there (row 0 is empty for AI) */
    BsShotResult result;
    BsError err = bs_attack(g, 0, 0, &result);
    assert(err == BS_OK);
    assert(result == BS_SHOT_MISS);
    assert(bs_get_attack_cell(g, 0, 0) == BS_CELL_MISS);
    assert(bs_miss_count(g, 0) == 1);
    bs_free(g);
}

static void test_attack_hit(void) {
    BsGame* g = bs_new();
    /* Human ships in rows 0-4 */
    bs_place_ship(g, 0, BS_SHIP_CARRIER, 0, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_BATTLESHIP, 1, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_CRUISER, 2, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_SUBMARINE, 3, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_DESTROYER, 4, 0, BS_DIR_HORIZONTAL);

    /* AI carrier at (0,0) */
    bs_place_ship(g, 1, BS_SHIP_CARRIER, 0, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_BATTLESHIP, 1, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_CRUISER, 2, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_SUBMARINE, 3, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_DESTROYER, 4, 0, BS_DIR_HORIZONTAL);

    bs_start(g);

    BsShotResult result;
    BsError err = bs_attack(g, 0, 0, &result);
    assert(err == BS_OK);
    assert(result == BS_SHOT_HIT);
    assert(bs_get_attack_cell(g, 0, 0) == BS_CELL_HIT);
    assert(bs_hit_count(g, 0) == 1);
    bs_free(g);
}

static void test_attack_already_attacked(void) {
    BsGame* g = make_started_game();

    BsShotResult result;
    bs_attack(g, 0, 0, &result);
    /* AI turn */
    int ar, ac;
    BsShotResult ares;
    bs_ai_attack(g, &ar, &ac, &ares);
    /* Human attacks same cell again */
    assert(bs_attack(g, 0, 0, &result) == BS_ERR_ALREADY_ATTACKED);
    bs_free(g);
}

static void test_turn_enforcement(void) {
    BsGame* g = make_started_game();
    BsShotResult result;

    /* Human attacks */
    assert(bs_attack(g, 0, 0, &result) == BS_OK);
    /* It's now AI's turn -- human can't attack again */
    assert(bs_current_player(g) == 1);
    assert(bs_attack(g, 1, 0, &result) == BS_ERR_NOT_YOUR_TURN);
    bs_free(g);
}

static void test_sink_ship(void) {
    BsGame* g = bs_new();
    /* Place all human ships */
    bs_place_ship(g, 0, BS_SHIP_CARRIER, 0, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_BATTLESHIP, 1, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_CRUISER, 2, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_SUBMARINE, 3, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_DESTROYER, 4, 0, BS_DIR_HORIZONTAL);

    /* Place AI's destroyer at (0,0)-(0,1) and rest far away */
    bs_place_ship(g, 1, BS_SHIP_DESTROYER, 0, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_CARRIER, 5, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_BATTLESHIP, 6, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_CRUISER, 7, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 1, BS_SHIP_SUBMARINE, 8, 0, BS_DIR_HORIZONTAL);

    bs_start(g);
    BsShotResult result;
    int ar, ac;
    BsShotResult ares;

    /* Hit destroyer cell 1 */
    assert(bs_attack(g, 0, 0, &result) == BS_OK);
    assert(result == BS_SHOT_HIT);
    assert(bs_ship_sunk(g, 1, BS_SHIP_DESTROYER) == 0);

    /* AI's turn */
    bs_ai_attack(g, &ar, &ac, &ares);

    /* Sink destroyer cell 2 */
    assert(bs_attack(g, 0, 1, &result) == BS_OK);
    assert(result == BS_SHOT_SUNK);
    assert(bs_ship_sunk(g, 1, BS_SHIP_DESTROYER) == 1);
    assert(bs_ships_remaining(g, 1) == 4);

    /* Sunk cells should show as SUNK */
    assert(bs_get_attack_cell(g, 0, 0) == BS_CELL_SUNK);
    assert(bs_get_attack_cell(g, 0, 1) == BS_CELL_SUNK);

    bs_free(g);
}

static void test_ai_never_attacks_same_cell(void) {
    BsGame* g = make_started_game();

    /* Play 50 rounds; AI should never error on attacking */
    for (int i = 0; i < 50 && bs_status(g) == BS_STATUS_PLAYING; i++) {
        /* Human attacks a unique cell */
        for (int r = 0; r < BS_GRID_SIZE; r++) {
            for (int c = 0; c < BS_GRID_SIZE; c++) {
                if (bs_get_attack_cell(g, r, c) == BS_CELL_UNKNOWN) {
                    BsShotResult res;
                    BsError err = bs_attack(g, r, c, &res);
                    assert(err == BS_OK);
                    goto human_done;
                }
            }
        }
        human_done:
        if (bs_status(g) != BS_STATUS_PLAYING) break;

        int ar, ac;
        BsShotResult ares;
        bs_ai_attack(g, &ar, &ac, &ares);
        /* AI should always pick an unknown cell */
    }
    bs_free(g);
}

static void test_win_detection(void) {
    BsGame* g = bs_new();
    /* Place all human ships */
    bs_place_ship(g, 0, BS_SHIP_CARRIER, 0, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_BATTLESHIP, 1, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_CRUISER, 2, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_SUBMARINE, 3, 0, BS_DIR_HORIZONTAL);
    bs_place_ship(g, 0, BS_SHIP_DESTROYER, 4, 0, BS_DIR_HORIZONTAL);

    /* Place all AI ships tightly: destroyer at (0,0)-(0,1), etc. */
    bs_place_ship(g, 1, BS_SHIP_DESTROYER, 0, 0, BS_DIR_HORIZONTAL);   /* 2 cells */
    bs_place_ship(g, 1, BS_SHIP_SUBMARINE, 1, 0, BS_DIR_HORIZONTAL);   /* 3 cells */
    bs_place_ship(g, 1, BS_SHIP_CRUISER, 2, 0, BS_DIR_HORIZONTAL);     /* 3 cells */
    bs_place_ship(g, 1, BS_SHIP_BATTLESHIP, 3, 0, BS_DIR_HORIZONTAL);  /* 4 cells */
    bs_place_ship(g, 1, BS_SHIP_CARRIER, 4, 0, BS_DIR_HORIZONTAL);     /* 5 cells */
    /* Total: 17 cells to sink */

    bs_start(g);

    /* Known AI ship positions: rows 0-4, cols 0-4(max) */
    int targets[][2] = {
        {0,0},{0,1},           /* destroyer */
        {1,0},{1,1},{1,2},     /* submarine */
        {2,0},{2,1},{2,2},     /* cruiser */
        {3,0},{3,1},{3,2},{3,3}, /* battleship */
        {4,0},{4,1},{4,2},{4,3},{4,4}  /* carrier */
    };
    int n = sizeof(targets) / sizeof(targets[0]);

    for (int i = 0; i < n; i++) {
        assert(bs_status(g) == BS_STATUS_PLAYING);
        BsShotResult result;
        BsError err = bs_attack(g, targets[i][0], targets[i][1], &result);
        assert(err == BS_OK);
        assert(result == BS_SHOT_HIT || result == BS_SHOT_SUNK);

        if (i < n - 1 && bs_status(g) == BS_STATUS_PLAYING) {
            /* AI turn */
            int ar, ac;
            BsShotResult ares;
            bs_ai_attack(g, &ar, &ac, &ares);
        }
    }

    assert(bs_status(g) == BS_STATUS_WON);
    assert(bs_ships_remaining(g, 1) == 0);
}

static void test_reset(void) {
    BsGame* g = make_started_game();
    BsShotResult result;
    bs_attack(g, 0, 0, &result);
    assert(bs_turn_count(g) == 1);

    bs_reset(g);
    assert(bs_status(g) == BS_STATUS_PLACING);
    assert(bs_turn_count(g) == 0);
    assert(bs_current_player(g) == 0);
    bs_free(g);
}

static void test_out_of_bounds_queries(void) {
    BsGame* g = bs_new();
    assert(bs_get_own_cell(g, -1, 0) == BS_OWN_WATER);
    assert(bs_get_own_cell(g, 0, 10) == BS_OWN_WATER);
    assert(bs_get_attack_cell(g, 10, 10) == BS_CELL_UNKNOWN);
    bs_free(g);
}

/* ── Main ───────────────────────────────────────────────────────── */

int main(void) {
    printf("libbattleship tests:\n");

    TEST(test_new_free);
    TEST(test_free_null);
    TEST(test_ship_sizes);
    TEST(test_place_ship_valid);
    TEST(test_place_ship_vertical);
    TEST(test_place_ship_oob);
    TEST(test_place_ship_overlap);
    TEST(test_random_placement_valid);
    TEST(test_attack_miss);
    TEST(test_attack_hit);
    TEST(test_attack_already_attacked);
    TEST(test_turn_enforcement);
    TEST(test_sink_ship);
    TEST(test_ai_never_attacks_same_cell);
    TEST(test_win_detection);
    TEST(test_reset);
    TEST(test_out_of_bounds_queries);

    printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
