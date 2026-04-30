#ifndef BATTLESHIP_INTERFACE_H
#define BATTLESHIP_INTERFACE_H

#include <QObject>
#include <QString>
#include "interface.h"

/**
 * @brief Interface for the Battleship module.
 *
 * Exposes a single game instance managed by the plugin.
 * All enum values from libbattleship are passed as plain int so that
 * callers (QML / other modules) do not need to include the C header.
 *
 * Players:      0 = human, 1 = AI
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

    /** Reset, randomly place ships for both sides, and start the game. */
    Q_INVOKABLE virtual void newGame() = 0;

    /**
     * Human fires at (row, col) on the AI board.
     * If the shot succeeds and the game is still ongoing, the AI
     * automatically takes its turn.
     * @return BsError as int (0 = success).
     */
    Q_INVOKABLE virtual int attack(int row, int col) = 0;

    /** BsShotResult of the last human attack. */
    Q_INVOKABLE virtual int lastShotResult() = 0;

    /** Row of the last AI attack (-1 if none yet). */
    Q_INVOKABLE virtual int aiAttackRow() = 0;

    /** Column of the last AI attack (-1 if none yet). */
    Q_INVOKABLE virtual int aiAttackCol() = 0;

    /** BsShotResult of the last AI attack. */
    Q_INVOKABLE virtual int aiShotResult() = 0;

    /** BsGameStatus as int. */
    Q_INVOKABLE virtual int status() = 0;

    /** Whose turn: 0 = human, 1 = AI. */
    Q_INVOKABLE virtual int currentPlayer() = 0;

    /** BsOwnCell for the human's board at (row, col). */
    Q_INVOKABLE virtual int getOwnCell(int row, int col) = 0;

    /** BsAttackCell for the human's attack board at (row, col). */
    Q_INVOKABLE virtual int getAttackCell(int row, int col) = 0;

    /** Whether the given ship is sunk (0/1). player: 0=human, 1=AI. */
    Q_INVOKABLE virtual int shipSunk(int player, int shipType) = 0;

    /** Count of ships still alive. player: 0=human, 1=AI. */
    Q_INVOKABLE virtual int shipsRemaining(int player) = 0;

    Q_INVOKABLE virtual int turnCount() = 0;
    Q_INVOKABLE virtual int hitCount(int player) = 0;
    Q_INVOKABLE virtual int missCount(int player) = 0;
};

#define BattleshipInterface_iid "org.logos.BattleshipInterface"
Q_DECLARE_INTERFACE(BattleshipInterface, BattleshipInterface_iid)

#endif // BATTLESHIP_INTERFACE_H
