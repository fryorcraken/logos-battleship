#ifndef BATTLESHIP_PLUGIN_H
#define BATTLESHIP_PLUGIN_H

#include <QObject>
#include <QString>
#include "battleship_interface.h"
#include "logos_api.h"
#include "logos_sdk.h"
#include "lib/libbattleship.h"

namespace battleship { class GameMessage; }

class BattleshipPlugin : public QObject, public BattleshipInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID BattleshipInterface_iid FILE "metadata.json")
    Q_INTERFACES(BattleshipInterface PluginInterface)

public:
    explicit BattleshipPlugin(QObject* parent = nullptr);
    ~BattleshipPlugin() override;

    QString name()    const override { return "battleship"; }
    QString version() const override { return "1.0.0"; }

    Q_INVOKABLE void initLogos(LogosAPI* logosAPIInstance);

    // Game commands
    Q_INVOKABLE void newGame()                    override;
    Q_INVOKABLE int  attack(int row, int col)     override;

    // Last shot info
    Q_INVOKABLE int  lastShotResult()             override;
    Q_INVOKABLE int  aiAttackRow()                override;
    Q_INVOKABLE int  aiAttackCol()                override;
    Q_INVOKABLE int  aiShotResult()               override;

    // Game state
    Q_INVOKABLE int  status()                     override;
    Q_INVOKABLE int  currentPlayer()              override;

    // Board queries
    Q_INVOKABLE int  getOwnCell(int row, int col) override;
    Q_INVOKABLE int  getAttackCell(int row, int col) override;

    // Ship queries
    Q_INVOKABLE int  shipSunk(int player, int shipType)  override;
    Q_INVOKABLE int  shipsRemaining(int player)          override;

    // Stats
    Q_INVOKABLE int  turnCount()              override;
    Q_INVOKABLE int  hitCount(int player)     override;
    Q_INVOKABLE int  missCount(int player)    override;

    // Multiplayer
    Q_INVOKABLE void enableMultiplayer()      override;
    Q_INVOKABLE void disableMultiplayer()     override;
    Q_INVOKABLE int  mpStatus()               override;
    Q_INVOKABLE int  mpConnected()            override;
    Q_INVOKABLE int  mpMessagesSent()         override;
    Q_INVOKABLE int  mpMessagesReceived()     override;
    Q_INVOKABLE QString mpError()             override;
    Q_INVOKABLE int  mpVerified()             override;
    Q_INVOKABLE int  mpIsMultiplayer()        override;

signals:
    void eventResponse(const QString& eventName, const QVariantList& args);

private:
    void sendGameMessage(const battleship::GameMessage& msg);
    void broadcastJoin();
    void broadcastAttack(int row, int col);
    void broadcastResult(int row, int col, BsShotResult result, int sunkShip,
                         const int* sunkCells, int nSunkCells);
    void broadcastReveal();
    void setMpError(const QString& err);

    BsGame*       m_game = nullptr;
    LogosModules* logos  = nullptr;

    // AI mode state
    int m_lastShotResult = 0;
    int m_aiRow = -1;
    int m_aiCol = -1;
    int m_aiShotResult = 0;

    // Multiplayer state
    bool    m_mpEnabled           = false;
    bool    m_mpConnected         = false;
    bool    m_handlersRegistered  = false;
    bool    m_isMultiplayer       = false;
    int     m_mpMsgSent           = 0;
    int     m_mpMsgReceived       = 0;
    QString m_mpError;
    QString m_contentTopic        = "/battleship/1/game/proto";

    // Session state
    QString m_playerId;
    QString m_opponentId;
    uint8_t m_myBoardHash[32]       = {};
    uint8_t m_opponentBoardHash[32] = {};
    bool    m_opponentJoined        = false;
    bool    m_iAmPlayer1            = false;  // lower player_id goes first
    bool    m_waitingForResult      = false;
    int     m_mpVerified            = 0;  // 0=pending, 1=verified, 2=cheat
    bool    m_revealSent            = false;
};

#endif // BATTLESHIP_PLUGIN_H
