#ifndef BATTLESHIP_PLUGIN_H
#define BATTLESHIP_PLUGIN_H

#include <QObject>
#include <QString>
#include "battleship_interface.h"
#include "logos_api.h"
#include "logos_sdk.h"
#include "lib/libbattleship.h"

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

signals:
    void eventResponse(const QString& eventName, const QVariantList& args);

private:
    BsGame*       m_game = nullptr;
    LogosModules* logos  = nullptr;

    int m_lastShotResult = 0;
    int m_aiRow = -1;
    int m_aiCol = -1;
    int m_aiShotResult = 0;
};

#endif // BATTLESHIP_PLUGIN_H
