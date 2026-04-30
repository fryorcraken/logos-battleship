#include "battleship_plugin.h"
#include <QVariantList>

BattleshipPlugin::BattleshipPlugin(QObject* parent)
    : QObject(parent)
{
    m_game = bs_new();
}

BattleshipPlugin::~BattleshipPlugin()
{
    bs_free(m_game);
    delete logos;
}

void BattleshipPlugin::initLogos(LogosAPI* logosAPIInstance)
{
    if (logos) {
        delete logos;
        logos = nullptr;
    }
    logosAPI = logosAPIInstance;
    if (logosAPI) {
        logos = new LogosModules(logosAPI);
    }
}

/* ── Game commands ──────────────────────────────────────────────── */

void BattleshipPlugin::newGame()
{
    bs_reset(m_game);
    bs_place_ships_random(m_game, 0);
    bs_place_ships_random(m_game, 1);
    bs_start(m_game);

    m_lastShotResult = 0;
    m_aiRow = -1;
    m_aiCol = -1;
    m_aiShotResult = 0;

    emit eventResponse("newGame", {});
}

int BattleshipPlugin::attack(int row, int col)
{
    BsShotResult result;
    BsError err = bs_attack(m_game, row, col, &result);
    if (err != BS_OK) return (int)err;

    m_lastShotResult = (int)result;
    emit eventResponse("attacked", { row, col, (int)result });

    /* If game is still going, AI attacks immediately */
    if (bs_status(m_game) == BS_STATUS_PLAYING &&
        bs_current_player(m_game) == 1) {

        int aiRow = -1, aiCol = -1;
        BsShotResult aiResult;
        bs_ai_attack(m_game, &aiRow, &aiCol, &aiResult);

        m_aiRow = aiRow;
        m_aiCol = aiCol;
        m_aiShotResult = (int)aiResult;

        emit eventResponse("aiAttacked", { aiRow, aiCol, (int)aiResult });
    }

    /* Check if game ended */
    BsGameStatus st = bs_status(m_game);
    if (st == BS_STATUS_WON || st == BS_STATUS_LOST) {
        emit eventResponse("gameOver", { (int)st });
    }

    return BS_OK;
}

/* ── Last shot info ─────────────────────────────────────────────── */

int BattleshipPlugin::lastShotResult() { return m_lastShotResult; }
int BattleshipPlugin::aiAttackRow()    { return m_aiRow; }
int BattleshipPlugin::aiAttackCol()    { return m_aiCol; }
int BattleshipPlugin::aiShotResult()   { return m_aiShotResult; }

/* ── Game state ─────────────────────────────────────────────────── */

int BattleshipPlugin::status()        { return (int)bs_status(m_game); }
int BattleshipPlugin::currentPlayer() { return bs_current_player(m_game); }

/* ── Board queries ──────────────────────────────────────────────── */

int BattleshipPlugin::getOwnCell(int row, int col)
{
    return (int)bs_get_own_cell(m_game, row, col);
}

int BattleshipPlugin::getAttackCell(int row, int col)
{
    return (int)bs_get_attack_cell(m_game, row, col);
}

/* ── Ship queries ───────────────────────────────────────────────── */

int BattleshipPlugin::shipSunk(int player, int shipType)
{
    return bs_ship_sunk(m_game, player, (BsShipType)shipType);
}

int BattleshipPlugin::shipsRemaining(int player)
{
    return bs_ships_remaining(m_game, player);
}

/* ── Stats ──────────────────────────────────────────────────────── */

int BattleshipPlugin::turnCount()          { return bs_turn_count(m_game); }
int BattleshipPlugin::hitCount(int player) { return bs_hit_count(m_game, player); }
int BattleshipPlugin::missCount(int player){ return bs_miss_count(m_game, player); }
