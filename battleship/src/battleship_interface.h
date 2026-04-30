#ifndef BATTLESHIP_INTERFACE_H
#define BATTLESHIP_INTERFACE_H

#include <QObject>
#include <QString>
#include "interface.h"

/**
 * @brief Interface for the Battleship module.
 *
 * Players:      0 = local, 1 = opponent (AI or remote)
 * BsOwnCell:    0 = WATER, 1 = SHIP, 2 = HIT
 * BsAttackCell: 0 = UNKNOWN, 1 = MISS, 2 = HIT, 3 = SUNK
 * BsGameStatus: 0 = PLACING, 1 = PLAYING, 2 = WON, 3 = LOST
 * BsShotResult: 0 = MISS, 1 = HIT, 2 = SUNK
 * BsError:      0 = OK, 1..7 = various errors
 */
class BattleshipInterface : public PluginInterface
{
public:
    virtual ~BattleshipInterface() = default;

    // ── Game commands ──────────────────────────────────────────
    Q_INVOKABLE virtual void newGame() = 0;
    Q_INVOKABLE virtual int  attack(int row, int col) = 0;

    // ── Last shot info ─────────────────────────────────────────
    Q_INVOKABLE virtual int  lastShotResult() = 0;
    Q_INVOKABLE virtual int  aiAttackRow() = 0;
    Q_INVOKABLE virtual int  aiAttackCol() = 0;
    Q_INVOKABLE virtual int  aiShotResult() = 0;

    // ── Game state ─────────────────────────────────────────────
    Q_INVOKABLE virtual int  status() = 0;
    Q_INVOKABLE virtual int  currentPlayer() = 0;

    // ── Board queries ──────────────────────────────────────────
    Q_INVOKABLE virtual int  getOwnCell(int row, int col) = 0;
    Q_INVOKABLE virtual int  getAttackCell(int row, int col) = 0;

    // ── Ship queries ───────────────────────────────────────────
    Q_INVOKABLE virtual int  shipSunk(int player, int shipType) = 0;
    Q_INVOKABLE virtual int  shipsRemaining(int player) = 0;

    // ── Stats ──────────────────────────────────────────────────
    Q_INVOKABLE virtual int  turnCount() = 0;
    Q_INVOKABLE virtual int  hitCount(int player) = 0;
    Q_INVOKABLE virtual int  missCount(int player) = 0;

    // ── Multiplayer ────────────────────────────────────────────
    Q_INVOKABLE virtual void enableMultiplayer() = 0;
    Q_INVOKABLE virtual void disableMultiplayer() = 0;

    /** 0=off, 1=connecting, 2=waiting_for_opponent, 3=playing, 4=game_over, 5=error */
    Q_INVOKABLE virtual int  mpStatus() = 0;
    Q_INVOKABLE virtual int  mpConnected() = 0;
    Q_INVOKABLE virtual int  mpMessagesSent() = 0;
    Q_INVOKABLE virtual int  mpMessagesReceived() = 0;
    Q_INVOKABLE virtual QString mpError() = 0;

    /** 0=pending, 1=verified, 2=cheat_detected */
    Q_INVOKABLE virtual int  mpVerified() = 0;

    /** 1 if multiplayer is active, 0 if AI mode */
    Q_INVOKABLE virtual int  mpIsMultiplayer() = 0;
};

#define BattleshipInterface_iid "org.logos.BattleshipInterface"
Q_DECLARE_INTERFACE(BattleshipInterface, BattleshipInterface_iid)

#endif // BATTLESHIP_INTERFACE_H
