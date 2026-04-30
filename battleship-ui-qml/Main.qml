import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#1a1a1a"

    readonly property int gridSize: 10
    readonly property int cellSize: 56
    readonly property int gridCells: 100

    // Game state
    property var ownBoard: []
    property var attackBoard: []
    property int gameStatus: 0
    property int humanShips: 5
    property int aiShips: 5
    property int turns: 0
    property int humanHits: 0
    property int humanMisses: 0
    property int aiHits: 0
    property int aiMisses: 0
    property int lastAiRow: -1
    property int lastAiCol: -1

    // Multiplayer state
    property int mpSt: 0         // 0=off,1=connecting,2=waiting,3=playing,4=game_over,5=error
    property int mpIsMultiplayer: 0
    property int mpMsgSent: 0
    property int mpMsgRecv: 0
    property string mpErr: ""
    property int mpVerified: 0   // 0=pending,1=verified,2=cheat

    Component.onCompleted: {
        if (typeof logos !== "undefined" && logos.onModuleEvent) {
            logos.onModuleEvent("battleship", "attacked")
            logos.onModuleEvent("battleship", "aiAttacked")
            logos.onModuleEvent("battleship", "attackResult")
            logos.onModuleEvent("battleship", "gameOver")
            logos.onModuleEvent("battleship", "newGame")
            logos.onModuleEvent("battleship", "mpStatusChanged")
            logos.onModuleEvent("battleship", "opponentJoined")
            logos.onModuleEvent("battleship", "verificationResult")
        }
        callNewGame()
    }

    Connections {
        target: typeof logos !== "undefined" ? logos : null
        function onModuleEventReceived(moduleName, eventName, data) {
            if (moduleName !== "battleship") return
            refreshAll()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Text {
            text: "BATTLESHIP"
            font.pixelSize: 40
            font.weight: Font.Bold
            font.letterSpacing: 6
            color: "#e0e0e0"
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: statusText()
            font.pixelSize: 26
            font.weight: Font.DemiBold
            color: root.gameStatus === 2 ? "#4aff4a"
                 : root.gameStatus === 3 ? "#ff4444"
                 : "#e0e0e0"
            Layout.alignment: Qt.AlignHCenter
        }

        // Two grids side by side
        RowLayout {
            spacing: 40
            Layout.alignment: Qt.AlignHCenter

            // Your Fleet
            ColumnLayout {
                spacing: 8
                Text {
                    text: "YOUR FLEET"
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    font.letterSpacing: 3
                    color: "#888"
                    Layout.alignment: Qt.AlignHCenter
                }
                GridLayout {
                    columns: root.gridSize
                    rowSpacing: 2
                    columnSpacing: 2
                    Repeater {
                        model: root.gridCells
                        Rectangle {
                            width: root.cellSize
                            height: root.cellSize
                            property int r: Math.floor(index / root.gridSize)
                            property int c: index % root.gridSize
                            property int v: (root.ownBoard.length > index) ? root.ownBoard[index] : 0
                            property bool isAiHit: r === root.lastAiRow && c === root.lastAiCol

                            color: v === 2 ? "#881111"
                                 : v === 1 ? "#4a4a5a"
                                 : "#1a2a3a"
                            border.color: isAiHit ? "#ffcc00" : "#333"
                            border.width: isAiHit ? 3 : 1
                            radius: 4

                            Text {
                                anchors.centerIn: parent
                                text: parent.v === 2 ? "\u2716" : parent.v === 1 ? "\u25A0" : ""
                                font.pixelSize: parent.v === 1 ? 20 : 28
                                font.weight: Font.Bold
                                color: parent.v === 2 ? "#ff4444" : "#7a7a8a"
                            }
                        }
                    }
                }
            }

            // Enemy Waters
            ColumnLayout {
                spacing: 8
                Text {
                    text: "ENEMY WATERS"
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    font.letterSpacing: 3
                    color: "#888"
                    Layout.alignment: Qt.AlignHCenter
                }
                GridLayout {
                    columns: root.gridSize
                    rowSpacing: 2
                    columnSpacing: 2
                    Repeater {
                        model: root.gridCells
                        Rectangle {
                            width: root.cellSize
                            height: root.cellSize
                            property int r: Math.floor(index / root.gridSize)
                            property int c: index % root.gridSize
                            property int v: (root.attackBoard.length > index) ? root.attackBoard[index] : 0
                            property bool canFire: v === 0 && root.gameStatus === 1

                            color: v === 3 ? "#660000"
                                 : v === 2 ? "#884400"
                                 : v === 1 ? "#1a2a3a"
                                 : "#1a2a3a"
                            border.color: v === 3 ? "#ff2222"
                                        : v === 2 ? "#ff8800"
                                        : "#333"
                            border.width: v >= 2 ? 2 : 1
                            radius: 4

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (parent.canFire)
                                        callAttack(parent.r, parent.c)
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                text: parent.v === 3 ? "\u2620"
                                    : parent.v === 2 ? "\u2716"
                                    : parent.v === 1 ? "\u25CB"
                                    : ""
                                font.pixelSize: parent.v === 3 ? 30 : 24
                                font.weight: Font.Bold
                                color: parent.v === 3 ? "#ff2222"
                                     : parent.v === 2 ? "#ff8800"
                                     : "#445566"
                            }
                        }
                    }
                }
            }
        }

        // Legend
        RowLayout {
            spacing: 32
            Layout.alignment: Qt.AlignHCenter

            RowLayout {
                spacing: 16
                Text { text: "Your Fleet:"; font.pixelSize: 16; color: "#666" }
                RowLayout {
                    spacing: 4
                    Rectangle { width: 20; height: 20; color: "#4a4a5a"; radius: 3 }
                    Text { text: "Ship"; font.pixelSize: 16; color: "#aaa" }
                }
                RowLayout {
                    spacing: 4
                    Rectangle { width: 20; height: 20; color: "#881111"; radius: 3 }
                    Text { text: "Hit"; font.pixelSize: 16; color: "#aaa" }
                }
                RowLayout {
                    spacing: 4
                    Rectangle { width: 20; height: 20; color: "#1a2a3a"; radius: 3 }
                    Text { text: "Water"; font.pixelSize: 16; color: "#aaa" }
                }
            }

            Rectangle { width: 2; height: 20; color: "#444" }

            RowLayout {
                spacing: 16
                Text { text: "Enemy:"; font.pixelSize: 16; color: "#666" }
                RowLayout {
                    spacing: 4
                    Rectangle { width: 20; height: 20; color: "#1a2a3a"; radius: 3; border.color: "#333"; border.width: 1
                        Text { anchors.centerIn: parent; text: "\u25CB"; font.pixelSize: 14; color: "#445566" }
                    }
                    Text { text: "Miss"; font.pixelSize: 16; color: "#aaa" }
                }
                RowLayout {
                    spacing: 4
                    Rectangle { width: 20; height: 20; color: "#884400"; radius: 3; border.color: "#ff8800"; border.width: 2
                        Text { anchors.centerIn: parent; text: "\u2716"; font.pixelSize: 14; color: "#ff8800" }
                    }
                    Text { text: "Hit"; font.pixelSize: 16; color: "#aaa" }
                }
                RowLayout {
                    spacing: 4
                    Rectangle { width: 20; height: 20; color: "#660000"; radius: 3; border.color: "#ff2222"; border.width: 2
                        Text { anchors.centerIn: parent; text: "\u2620"; font.pixelSize: 16; color: "#ff2222" }
                    }
                    Text { text: "Sunk"; font.pixelSize: 16; color: "#aaa" }
                }
            }
        }

        // Stats panel
        Rectangle {
            Layout.fillWidth: true
            height: 80
            color: "#222"
            radius: 8
            border.color: "#333"
            border.width: 2

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4
                Text {
                    text: "Turn: " + root.turns
                        + "  |  You: " + root.humanHits + "h/" + root.humanMisses + "m"
                        + "  |  Opp: " + root.aiHits + "h/" + root.aiMisses + "m"
                    font.pixelSize: 20
                    font.family: "monospace"
                    color: "#aaa"
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: "Your ships: " + root.humanShips + "/5"
                        + "  |  Enemy ships: " + root.aiShips + "/5"
                        + (root.mpIsMultiplayer ? ("  |  Msgs: " + root.mpMsgSent + "/" + root.mpMsgRecv) : "")
                    font.pixelSize: 20
                    font.family: "monospace"
                    color: "#aaa"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // Multiplayer section
        RowLayout {
            spacing: 16
            Layout.fillWidth: true

            Button {
                id: mpToggle
                text: root.mpIsMultiplayer ? "Disable Multiplayer" : "Enable Multiplayer"
                font.pixelSize: 20
                Layout.fillWidth: true
                implicitHeight: 44

                onClicked: {
                    if (root.mpIsMultiplayer)
                        callModule("disableMultiplayer", [])
                    else
                        callModule("enableMultiplayer", [])
                    refreshAll()
                }

                background: Rectangle {
                    color: mpToggle.hovered ? "#3a3a3a" : "#2a2a2a"
                    border.color: root.mpIsMultiplayer ? "#ff6b6b" : "#4aff4a"
                    border.width: 2
                    radius: 8
                }
                contentItem: Text {
                    text: mpToggle.text
                    font: mpToggle.font
                    color: "#e0e0e0"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                id: newGameBtn
                text: "New Game"
                font.pixelSize: 20
                Layout.fillWidth: true
                implicitHeight: 44
                onClicked: callNewGame()

                background: Rectangle {
                    color: newGameBtn.hovered ? "#3a3a3a" : "#2a2a2a"
                    border.color: "#555"
                    border.width: 2
                    radius: 8
                }
                contentItem: Text {
                    text: newGameBtn.text
                    font: newGameBtn.font
                    color: "#e0e0e0"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // Delivery status + verification
        Text {
            text: deliveryStatusText()
            font.pixelSize: 16
            color: root.mpErr.length > 0 ? "#ff6b6b"
                 : root.mpSt === 3 ? "#4aff4a"
                 : root.mpSt >= 1 ? "#ffcc00"
                 : "#666"
            Layout.alignment: Qt.AlignHCenter
            visible: root.mpIsMultiplayer
        }

        // Verification result
        Text {
            text: root.mpVerified === 1 ? "Board Verified"
                : root.mpVerified === 2 ? "CHEAT DETECTED"
                : ""
            font.pixelSize: 22
            font.weight: Font.Bold
            color: root.mpVerified === 1 ? "#4aff4a" : "#ff2222"
            Layout.alignment: Qt.AlignHCenter
            visible: root.mpVerified > 0
        }

        Item { Layout.fillHeight: true }
    }

    // Fallback poll timer for multiplayer (if onModuleEvent unavailable)
    Timer {
        interval: 500
        repeat: true
        running: root.mpIsMultiplayer && (typeof logos === "undefined" || !logos.onModuleEvent)
        onTriggered: refreshAll()
    }

    // Bridge helpers

    function callModule(method, args) {
        if (typeof logos === "undefined" || !logos.callModule)
            return -1
        return logos.callModule("battleship", method, args)
    }

    function callNewGame() {
        callModule("newGame", [])
        root.lastAiRow = -1
        root.lastAiCol = -1
        refreshAll()
    }

    function callAttack(row, col) {
        var err = callModule("attack", [row, col])
        if (Number(err) === 0 && !root.mpIsMultiplayer) {
            var ar = callModule("aiAttackRow", [])
            var ac = callModule("aiAttackCol", [])
            root.lastAiRow = (ar !== undefined && ar !== null) ? Number(ar) : -1
            root.lastAiCol = (ac !== undefined && ac !== null) ? Number(ac) : -1
        }
        refreshAll()
    }

    function refreshAll() {
        var ob = []
        var ab = []
        for (var r = 0; r < root.gridSize; r++) {
            for (var c = 0; c < root.gridSize; c++) {
                var ov = callModule("getOwnCell", [r, c])
                ob.push((ov !== undefined && ov !== null) ? Number(ov) : 0)
                var av = callModule("getAttackCell", [r, c])
                ab.push((av !== undefined && av !== null) ? Number(av) : 0)
            }
        }
        root.ownBoard = ob
        root.attackBoard = ab

        var st = callModule("status", [])
        root.gameStatus = (st !== undefined && st !== null) ? Number(st) : 0
        var tc = callModule("turnCount", [])
        root.turns = (tc !== undefined && tc !== null) ? Number(tc) : 0

        var hh = callModule("hitCount", [0])
        root.humanHits = (hh !== undefined && hh !== null) ? Number(hh) : 0
        var hm = callModule("missCount", [0])
        root.humanMisses = (hm !== undefined && hm !== null) ? Number(hm) : 0
        var ah = callModule("hitCount", [1])
        root.aiHits = (ah !== undefined && ah !== null) ? Number(ah) : 0
        var am = callModule("missCount", [1])
        root.aiMisses = (am !== undefined && am !== null) ? Number(am) : 0

        var hs = callModule("shipsRemaining", [0])
        root.humanShips = (hs !== undefined && hs !== null) ? Number(hs) : 0
        var as2 = callModule("shipsRemaining", [1])
        root.aiShips = (as2 !== undefined && as2 !== null) ? Number(as2) : 0

        // Multiplayer state
        var ms = callModule("mpStatus", [])
        root.mpSt = (ms !== undefined && ms !== null) ? Number(ms) : 0
        var mi = callModule("mpIsMultiplayer", [])
        root.mpIsMultiplayer = (mi !== undefined && mi !== null) ? Number(mi) : 0
        var mms = callModule("mpMessagesSent", [])
        root.mpMsgSent = (mms !== undefined && mms !== null) ? Number(mms) : 0
        var mmr = callModule("mpMessagesReceived", [])
        root.mpMsgRecv = (mmr !== undefined && mmr !== null) ? Number(mmr) : 0
        var me = callModule("mpError", [])
        root.mpErr = (typeof me === "string") ? me : ""
        var mv = callModule("mpVerified", [])
        root.mpVerified = (mv !== undefined && mv !== null) ? Number(mv) : 0

        // In MP, read opponent's last attack
        if (root.mpIsMultiplayer) {
            var oar = callModule("aiAttackRow", [])
            var oac = callModule("aiAttackCol", [])
            root.lastAiRow = (oar !== undefined && oar !== null) ? Number(oar) : -1
            root.lastAiCol = (oac !== undefined && oac !== null) ? Number(oac) : -1
        }
    }

    function statusText() {
        if (root.mpIsMultiplayer) {
            if (root.mpSt === 1) return "Connecting to network..."
            if (root.mpSt === 2) return "Waiting for opponent..."
            if (root.mpSt === 5) return "Error: " + root.mpErr
            if (root.gameStatus === 2) return "VICTORY! All enemy ships destroyed!"
            if (root.gameStatus === 3) return "DEFEAT. Your fleet has been sunk."
            if (root.mpSt === 4) return "Game over"
            return "Multiplayer - Your turn"
        }
        if (root.gameStatus === 0) return "Setting up..."
        if (root.gameStatus === 2) return "VICTORY! All enemy ships destroyed!"
        if (root.gameStatus === 3) return "DEFEAT. Your fleet has been sunk."
        return "Your turn - click an enemy cell to fire"
    }

    function deliveryStatusText() {
        if (root.mpSt === 0) return "Multiplayer: off"
        if (root.mpSt === 1) return "Delivery: connecting..."
        if (root.mpSt === 2) return "Delivery: connected - waiting for opponent"
        if (root.mpSt === 5) return "Error: " + root.mpErr
        return "Delivery: connected | sent: " + root.mpMsgSent + " recv: " + root.mpMsgRecv
    }
}
