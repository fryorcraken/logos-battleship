#include "battleship_plugin.h"
#include "battleship.pb.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include <QByteArray>
#include <QDebug>
#include <QUuid>
#include <QVariantList>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BS_TRACE(...) do { \
    fprintf(stderr, "[battleship] " __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    fflush(stderr); \
} while (0)

// Pick a free OS-assigned port. Same workaround as tictactoe for
// logos-delivery-module#24: createNode fails silently when port is 0.
static int pickFreePort(int type)
{
    int fd = ::socket(AF_INET, type, 0);
    if (fd < 0) return 0;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return 0;
    }
    socklen_t len = sizeof(addr);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
        ::close(fd);
        return 0;
    }
    int port = ntohs(addr.sin_port);
    ::close(fd);
    return port;
}

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

    if (m_isMultiplayer) {
        // In MP: place our ships, compute hash, broadcast join, wait
        m_playerId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        bs_get_board_hash(m_game, 0, m_myBoardHash);
        m_opponentJoined = false;
        m_waitingForResult = false;
        m_mpVerified = 0;
        m_revealSent = false;
        memset(m_opponentBoardHash, 0, 32);
        m_opponentId.clear();
        broadcastJoin();
        emit eventResponse("mpStatusChanged", QVariantList() << 2); // waiting
    } else {
        // AI mode: place AI ships too, start immediately
        bs_place_ships_random(m_game, 1);
        bs_start(m_game);
    }

    m_lastShotResult = 0;
    m_aiRow = -1;
    m_aiCol = -1;
    m_aiShotResult = 0;

    emit eventResponse("newGame", {});
}

int BattleshipPlugin::attack(int row, int col)
{
    if (m_isMultiplayer) {
        // MP: broadcast our attack, wait for result via event
        if (m_waitingForResult) return -1;  // still waiting
        broadcastAttack(row, col);
        m_waitingForResult = true;
        emit eventResponse("attacked", { row, col, -1 }); // -1 = pending
        return BS_OK;
    }

    // AI mode: same as before
    BsShotResult result;
    BsError err = bs_attack(m_game, row, col, &result);
    if (err != BS_OK) return (int)err;

    m_lastShotResult = (int)result;
    emit eventResponse("attacked", { row, col, (int)result });

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

    BsGameStatus st = bs_status(m_game);
    if (st == BS_STATUS_WON || st == BS_STATUS_LOST) {
        emit eventResponse("gameOver", { (int)st });
    }

    return BS_OK;
}

/* ── Getters ────────────────────────────────────────────────────── */

int BattleshipPlugin::lastShotResult() { return m_lastShotResult; }
int BattleshipPlugin::aiAttackRow()    { return m_aiRow; }
int BattleshipPlugin::aiAttackCol()    { return m_aiCol; }
int BattleshipPlugin::aiShotResult()   { return m_aiShotResult; }
int BattleshipPlugin::status()         { return (int)bs_status(m_game); }
int BattleshipPlugin::currentPlayer()  { return bs_current_player(m_game); }

int BattleshipPlugin::getOwnCell(int row, int col)
{
    return (int)bs_get_own_cell(m_game, row, col);
}

int BattleshipPlugin::getAttackCell(int row, int col)
{
    return (int)bs_get_attack_cell(m_game, row, col);
}

int BattleshipPlugin::shipSunk(int player, int shipType)
{
    return bs_ship_sunk(m_game, player, (BsShipType)shipType);
}

int BattleshipPlugin::shipsRemaining(int player)
{
    return bs_ships_remaining(m_game, player);
}

int BattleshipPlugin::turnCount()           { return bs_turn_count(m_game); }
int BattleshipPlugin::hitCount(int player)  { return bs_hit_count(m_game, player); }
int BattleshipPlugin::missCount(int player) { return bs_miss_count(m_game, player); }

/* ── Multiplayer ────────────────────────────────────────────────── */

void BattleshipPlugin::enableMultiplayer()
{
    BS_TRACE("enableMultiplayer() entered");
    emit eventResponse("mpStatusChanged", QVariantList() << 1); // connecting
    if (m_mpEnabled) { BS_TRACE("already enabled"); return; }
    if (!logos) {
        setMpError("Logos SDK not initialized");
        return;
    }

    // Register event handlers once (same pattern as tictactoe)
    if (!m_handlersRegistered) {
        logos->delivery_module.on("messageReceived",
            [this](const QString&, const QVariantList& data) {
                if (!m_mpEnabled) return;
                if (data.size() < 3) return;

                QByteArray deliveryPayload = QByteArray::fromBase64(data[2].toString().toUtf8());
                QByteArray protoBytes = QByteArray::fromBase64(deliveryPayload);

                battleship::GameMessage msg;
                if (!msg.ParseFromArray(protoBytes.data(), protoBytes.size())) {
                    qWarning() << "battleship: failed to parse protobuf";
                    return;
                }

                if (msg.has_join()) {
                    const auto& join = msg.join();
                    QString senderId = QString::fromStdString(join.player_id());
                    if (senderId == m_playerId) return; // self-echo
                    BS_TRACE("received JoinGame from %s", senderId.toUtf8().constData());

                    m_opponentId = senderId;
                    if (join.board_hash().size() == 32)
                        memcpy(m_opponentBoardHash, join.board_hash().data(), 32);
                    m_opponentJoined = true;

                    // Determine turn order: lower player_id goes first
                    m_iAmPlayer1 = (m_playerId < m_opponentId);
                    bs_start(m_game);
                    // In MP, player 0 is always "us", player 1 is opponent.
                    // m_iAmPlayer1 means we fire first.
                    m_mpMsgReceived++;
                    emit eventResponse("mpStatusChanged", QVariantList() << 3); // playing
                    emit eventResponse("opponentJoined", {});

                } else if (msg.has_attack()) {
                    const auto& atk = msg.attack();
                    if (QString::fromStdString(atk.player_id()) == m_playerId) return;
                    BS_TRACE("received Attack at (%d,%d)", atk.row(), atk.col());

                    // Opponent is attacking our board
                    BsShotResult result;
                    int sunkShip = -1;
                    int sunkCells[10];
                    int nSunkCells = 0;
                    BsError err = bs_apply_remote_attack(m_game, atk.row(), atk.col(),
                                                         &result, &sunkShip, sunkCells, &nSunkCells);
                    if (err != BS_OK) {
                        qWarning() << "battleship: remote attack rejected, err=" << err;
                        return;
                    }
                    m_mpMsgReceived++;

                    // Report result back
                    broadcastResult(atk.row(), atk.col(), result, sunkShip, sunkCells, nSunkCells);

                    m_aiRow = atk.row();
                    m_aiCol = atk.col();
                    m_aiShotResult = (int)result;
                    emit eventResponse("aiAttacked", { (int)atk.row(), (int)atk.col(), (int)result });

                    // Check if we lost
                    if (bs_ships_remaining(m_game, 0) == 0) {
                        bs_set_lost(m_game);
                        emit eventResponse("gameOver", { (int)BS_STATUS_LOST });
                        broadcastReveal();
                    }

                } else if (msg.has_attack_result()) {
                    const auto& res = msg.attack_result();
                    if (QString::fromStdString(res.player_id()) == m_playerId) return;
                    BS_TRACE("received AttackResult at (%d,%d) result=%d", res.row(), res.col(), res.result());

                    // Apply result to our attack board
                    int sunkCells[10];
                    int nSunkCells = res.sunk_cells_size() / 2;
                    for (int i = 0; i < nSunkCells && i < 5; i++) {
                        sunkCells[i * 2]     = res.sunk_cells(i * 2);
                        sunkCells[i * 2 + 1] = res.sunk_cells(i * 2 + 1);
                    }
                    bs_apply_remote_result(m_game, res.row(), res.col(),
                                           (BsShotResult)res.result(),
                                           sunkCells, nSunkCells);

                    m_lastShotResult = res.result();
                    m_waitingForResult = false;
                    m_mpMsgReceived++;

                    emit eventResponse("attackResult", { (int)res.row(), (int)res.col(), (int)res.result() });

                    // Check if we won
                    if (bs_ships_remaining(m_game, 1) == 0) {
                        bs_set_won(m_game);
                        emit eventResponse("gameOver", { (int)BS_STATUS_WON });
                        broadcastReveal();
                    }

                } else if (msg.has_reveal()) {
                    const auto& rev = msg.reveal();
                    if (QString::fromStdString(rev.player_id()) == m_playerId) return;
                    BS_TRACE("received RevealBoard");

                    m_mpMsgReceived++;

                    // Verify: SHA-256(board_data) == board_hash in reveal
                    if (rev.board_data().size() == BS_BOARD_DATA_SIZE &&
                        rev.board_hash().size() == BS_HASH_SIZE) {
                        const uint8_t* data = reinterpret_cast<const uint8_t*>(rev.board_data().data());
                        const uint8_t* hash = reinterpret_cast<const uint8_t*>(rev.board_hash().data());

                        int dataMatch = bs_verify_board(data, hash);
                        // Cross-check: board_hash == stored join hash
                        int joinMatch = (memcmp(hash, m_opponentBoardHash, 32) == 0) ? 1 : 0;

                        if (dataMatch && joinMatch) {
                            m_mpVerified = 1;
                            BS_TRACE("board verification PASSED");
                        } else {
                            m_mpVerified = 2;
                            BS_TRACE("board verification FAILED data=%d join=%d", dataMatch, joinMatch);
                        }
                    } else {
                        m_mpVerified = 2;
                        BS_TRACE("board verification FAILED: bad sizes");
                    }
                    emit eventResponse("verificationResult", QVariantList() << m_mpVerified);
                }
            });

        logos->delivery_module.on("connectionStateChanged",
            [this](const QString&, const QVariantList& data) {
                if (!m_mpEnabled) return;
                m_mpConnected = data.size() > 0 && !data[0].toString().isEmpty();
                BS_TRACE("connection state changed, connected: %d", m_mpConnected);
                emit eventResponse("mpStatusChanged", QVariantList() << (m_mpConnected ? 2 : 1));
            });

        logos->delivery_module.on("messageError",
            [this](const QString&, const QVariantList& data) {
                if (!m_mpEnabled) return;
                m_mpError = data.size() >= 3 ? data[2].toString() : "send failed";
                qWarning() << "battleship: message error:" << m_mpError;
                emit eventResponse("mpStatusChanged", QVariantList() << 5);
            });

        m_handlersRegistered = true;
    }

    // Bypass typed SDK wrapper (same as tictactoe: LogosResult mismatch)
    LogosAPIClient* client = logosAPI->getClient("delivery_module");
    if (!client) {
        setMpError("delivery_module client unavailable");
        return;
    }

    auto callBool = [&](const char* what, const QVariant& v) {
        BS_TRACE("RPC %s: type=%d val=%s", what, (int)v.userType(), v.toString().toUtf8().constData());
        if (!v.isValid()) { setMpError(QStringLiteral("%1: no response").arg(what)); return false; }
        if (!v.toBool())  { setMpError(QStringLiteral("%1 returned false").arg(what)); return false; }
        return true;
    };

    int tcpPort = pickFreePort(SOCK_STREAM);
    int udpPort = pickFreePort(SOCK_DGRAM);
    if (tcpPort == 0 || udpPort == 0) {
        setMpError("failed to pick free ports");
        return;
    }
    BS_TRACE("picked tcpPort=%d discv5UdpPort=%d", tcpPort, udpPort);

    QString config = QStringLiteral(
        R"({"logLevel":"INFO","mode":"Core","preset":"logos.dev","tcpPort":%1,"discv5UdpPort":%2})"
    ).arg(tcpPort).arg(udpPort);

    const Timeout kTimeout(90000);

    BS_TRACE("calling createNode");
    if (!callBool("createNode",
                  client->invokeRemoteMethod("delivery_module", "createNode", config, kTimeout)))
        return;

    BS_TRACE("calling start");
    if (!callBool("start",
                  client->invokeRemoteMethod("delivery_module", "start", QVariantList(), kTimeout)))
        return;

    BS_TRACE("calling subscribe");
    if (!callBool("subscribe",
                  client->invokeRemoteMethod("delivery_module", "subscribe", m_contentTopic, kTimeout))) {
        client->invokeRemoteMethod("delivery_module", "stop", QVariantList(), kTimeout);
        return;
    }

    m_mpEnabled = true;
    m_isMultiplayer = true;
    BS_TRACE("multiplayer enabled");
    emit eventResponse("mpStatusChanged", QVariantList() << 1);

    // Auto-start a new game (places ships, computes hash, broadcasts join)
    newGame();
}

void BattleshipPlugin::disableMultiplayer()
{
    if (!m_mpEnabled) return;

    if (logosAPI) {
        if (LogosAPIClient* client = logosAPI->getClient("delivery_module")) {
            const Timeout kTimeout(90000);
            client->invokeRemoteMethod("delivery_module", "unsubscribe", m_contentTopic, kTimeout);
            client->invokeRemoteMethod("delivery_module", "stop", QVariantList(), kTimeout);
        }
    }
    m_mpConnected = false;
    m_mpEnabled = false;
    m_isMultiplayer = false;
    m_mpMsgSent = 0;
    m_mpMsgReceived = 0;
    m_mpError.clear();
    m_opponentJoined = false;
    m_waitingForResult = false;
    m_mpVerified = 0;
    m_revealSent = false;
    emit eventResponse("mpStatusChanged", QVariantList() << 0);
    BS_TRACE("multiplayer disabled");
}

int BattleshipPlugin::mpStatus()
{
    if (!m_mpEnabled) return 0;
    if (!m_mpError.isEmpty()) return 5;
    if (!m_mpConnected) return 1;
    if (!m_opponentJoined) return 2;
    BsGameStatus gs = bs_status(m_game);
    if (gs == BS_STATUS_WON || gs == BS_STATUS_LOST) return 4;
    return 3; // playing
}

int     BattleshipPlugin::mpConnected()         { return m_mpConnected ? 1 : 0; }
int     BattleshipPlugin::mpMessagesSent()      { return m_mpMsgSent; }
int     BattleshipPlugin::mpMessagesReceived()   { return m_mpMsgReceived; }
QString BattleshipPlugin::mpError()             { return m_mpError; }
int     BattleshipPlugin::mpVerified()          { return m_mpVerified; }
int     BattleshipPlugin::mpIsMultiplayer()     { return m_isMultiplayer ? 1 : 0; }

/* ── Private helpers ────────────────────────────────────────────── */

void BattleshipPlugin::sendGameMessage(const battleship::GameMessage& msg)
{
    m_mpError.clear();
    std::string serialized;
    if (!msg.SerializeToString(&serialized)) {
        setMpError("protobuf serialization failed");
        return;
    }
    QString payload = QString::fromLatin1(
        QByteArray(serialized.data(), serialized.size()).toBase64());

    LogosAPIClient* client = logosAPI ? logosAPI->getClient("delivery_module") : nullptr;
    if (!client) {
        setMpError("delivery_module client unavailable");
        return;
    }
    QVariant v = client->invokeRemoteMethod("delivery_module", "send",
                                            m_contentTopic, payload, Timeout(90000));
    if (!v.isValid()) {
        setMpError("send: no response");
        return;
    }
    m_mpMsgSent++;
}

void BattleshipPlugin::broadcastJoin()
{
    if (!m_mpEnabled) return;
    battleship::GameMessage msg;
    auto* join = msg.mutable_join();
    join->set_player_id(m_playerId.toStdString());
    join->set_board_hash(m_myBoardHash, 32);
    sendGameMessage(msg);
    BS_TRACE("broadcast JoinGame, id=%s", m_playerId.toUtf8().constData());
}

void BattleshipPlugin::broadcastAttack(int row, int col)
{
    if (!m_mpEnabled) return;
    battleship::GameMessage msg;
    auto* atk = msg.mutable_attack();
    atk->set_player_id(m_playerId.toStdString());
    atk->set_row(row);
    atk->set_col(col);
    sendGameMessage(msg);
}

void BattleshipPlugin::broadcastResult(int row, int col, BsShotResult result,
                                        int sunkShip,
                                        const int* sunkCells, int nSunkCells)
{
    if (!m_mpEnabled) return;
    battleship::GameMessage msg;
    auto* res = msg.mutable_attack_result();
    res->set_player_id(m_playerId.toStdString());
    res->set_row(row);
    res->set_col(col);
    res->set_result((uint32_t)result);
    if (result == BS_SHOT_SUNK && sunkShip >= 0) {
        res->set_sunk_ship_type((uint32_t)sunkShip);
        for (int i = 0; i < nSunkCells; i++) {
            res->add_sunk_cells(sunkCells[i * 2]);
            res->add_sunk_cells(sunkCells[i * 2 + 1]);
        }
    }
    sendGameMessage(msg);
}

void BattleshipPlugin::broadcastReveal()
{
    if (!m_mpEnabled || m_revealSent) return;
    battleship::GameMessage msg;
    auto* rev = msg.mutable_reveal();
    rev->set_player_id(m_playerId.toStdString());
    rev->set_board_hash(m_myBoardHash, 32);

    uint8_t boardData[BS_BOARD_DATA_SIZE];
    bs_get_board_data(m_game, 0, boardData);
    rev->set_board_data(boardData, BS_BOARD_DATA_SIZE);

    sendGameMessage(msg);
    m_revealSent = true;
    BS_TRACE("broadcast RevealBoard");
}

void BattleshipPlugin::setMpError(const QString& err)
{
    m_mpError = err;
    qWarning() << "battleship:" << err;
    emit eventResponse("mpStatusChanged", QVariantList() << 5);
}
