import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#1a1a1a"

    // ── Constants ──────────────────────────────────────────────────
    readonly property int gridSize: 10
    readonly property int cellSize: 32
    readonly property int gridCells: gridSize * gridSize

    // ── Game state ─────────────────────────────────────────────────
    property var ownBoard: new Array(gridCells).fill(0)   // BsOwnCell
    property var attackBoard: new Array(gridCells).fill(0) // BsAttackCell
    property int gameStatus: 0      // 0=placing,1=playing,2=won,3=lost
    property int currentPlayer: 0   // 0=human, 1=AI
    property int humanShips: 5
    property int aiShips: 5
    property int turns: 0
    property int humanHits: 0
    property int humanMisses: 0
    property int aiHits: 0
    property int aiMisses: 0

    // Last AI attack (for highlight)
    property int lastAiRow: -1
    property int lastAiCol: -1
    property int lastAiResult: -1

    // Ship sunk status (for the ship roster)
    property var humanShipsSunk: [false, false, false, false, false]
    property var aiShipsSunk: [false, false, false, false, false]

    readonly property var shipNames: ["Carrier", "Battleship", "Cruiser", "Submarine", "Destroyer"]
    readonly property var shipSizes: [5, 4, 3, 3, 2]

    // ── Event subscriptions ────────────────────────────────────────
    Component.onCompleted: {
        if (typeof logos !== "undefined" && logos.onModuleEvent) {
            logos.onModuleEvent("battleship", "attacked")
            logos.onModuleEvent("battleship", "aiAttacked")
            logos.onModuleEvent("battleship", "gameOver")
            logos.onModuleEvent("battleship", "newGame")
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

    // ── Layout ─────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Title
        Text {
            text: "BATTLESHIP"
            font.pixelSize: 22
            font.weight: Font.Bold
            font.letterSpacing: 4
            color: "#e0e0e0"
            Layout.alignment: Qt.AlignHCenter
        }

        // Status
        Text {
            id: statusLabel
            text: statusText()
            font.pixelSize: 14
            font.weight: Font.DemiBold
            color: root.gameStatus === 2 ? "#4aff4a"
                 : root.gameStatus === 3 ? "#ff4444"
                 : "#e0e0e0"
            Layout.alignment: Qt.AlignHCenter
        }

        // Two grids side by side
        RowLayout {
            spacing: 24
            Layout.alignment: Qt.AlignHCenter

            // ── Your Fleet ─────────────────────────────────────────
            ColumnLayout {
                spacing: 6

                Text {
                    text: "YOUR FLEET"
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    font.letterSpacing: 2
                    color: "#888"
                    Layout.alignment: Qt.AlignHCenter
                }

                // Column headers
                RowLayout {
                    spacing: 1
                    Item { width: 18; height: 1 } // spacer for row labels
                    Repeater {
                        model: root.gridSize
                        Text {
                            width: root.cellSize; height: 14
                            text: String.fromCharCode(65 + index)
                            font.pixelSize: 9
                            color: "#666"
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                // Grid with row labels
                Repeater {
                    model: root.gridSize
                    RowLayout {
                        spacing: 1
                        property int rowIdx: index
                        Text {
                            width: 18; height: root.cellSize
                            text: (rowIdx + 1).toString()
                            font.pixelSize: 9
                            color: "#666"
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                        }
                        Repeater {
                            model: root.gridSize
                            Rectangle {
                                width: root.cellSize; height: root.cellSize
                                property int cellIdx: rowIdx * root.gridSize + index
                                property int cellVal: root.ownBoard[cellIdx]
                                // Highlight last AI attack
                                property bool isLastAi: rowIdx === root.lastAiRow && index === root.lastAiCol

                                color: cellVal === 2 ? "#cc2222"    // HIT on our ship
                                     : cellVal === 1 ? "#4a4a5a"    // SHIP (unhit)
                                     : "#1a2a3a"                     // WATER
                                border.color: isLastAi ? "#ffcc00" : "#333"
                                border.width: isLastAi ? 2 : 1
                                radius: 2

                                // Show a dot for water cells that were attacked (miss on own board)
                                // We can detect this: own_hits shows attacks. Water + attacked = miss marker
                                Text {
                                    anchors.centerIn: parent
                                    text: parent.cellVal === 2 ? "X"
                                        : parent.cellVal === 1 ? "■"
                                        : ""
                                    font.pixelSize: parent.cellVal === 1 ? 10 : 14
                                    font.weight: Font.Bold
                                    color: parent.cellVal === 2 ? "#ff8888"
                                         : "#6a6a7a"
                                }
                            }
                        }
                    }
                }
            }

            // ── Enemy Waters ───────────────────────────────────────
            ColumnLayout {
                spacing: 6

                Text {
                    text: "ENEMY WATERS"
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    font.letterSpacing: 2
                    color: "#888"
                    Layout.alignment: Qt.AlignHCenter
                }

                // Column headers
                RowLayout {
                    spacing: 1
                    Item { width: 18; height: 1 }
                    Repeater {
                        model: root.gridSize
                        Text {
                            width: root.cellSize; height: 14
                            text: String.fromCharCode(65 + index)
                            font.pixelSize: 9
                            color: "#666"
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                Repeater {
                    model: root.gridSize
                    RowLayout {
                        spacing: 1
                        property int rowIdx: index
                        Text {
                            width: 18; height: root.cellSize
                            text: (rowIdx + 1).toString()
                            font.pixelSize: 9
                            color: "#666"
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                        }
                        Repeater {
                            model: root.gridSize
                            Rectangle {
                                width: root.cellSize; height: root.cellSize
                                property int cellIdx: rowIdx * root.gridSize + index
                                property int cellVal: root.attackBoard[cellIdx]
                                property bool canFire: cellVal === 0 && root.gameStatus === 1

                                color: cellVal === 3 ? "#aa1111"    // SUNK
                                     : cellVal === 2 ? "#cc4444"    // HIT
                                     : cellVal === 1 ? "#2a3a4a"    // MISS
                                     : (hoverArea.containsMouse && canFire) ? "#2a3a4a"
                                     : "#1a2a3a"                     // UNKNOWN
                                border.color: canFire && hoverArea.containsMouse ? "#4a9eff" : "#333"
                                border.width: 1
                                radius: 2

                                MouseArea {
                                    id: hoverArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: parent.canFire ? Qt.CrossCursor : Qt.ArrowCursor
                                    onClicked: {
                                        if (parent.canFire)
                                            callAttack(rowIdx, index)
                                    }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: parent.cellVal === 3 ? "X"
                                        : parent.cellVal === 2 ? "X"
                                        : parent.cellVal === 1 ? "•"
                                        : ""
                                    font.pixelSize: parent.cellVal === 1 ? 16 : 14
                                    font.weight: Font.Bold
                                    color: parent.cellVal === 3 ? "#ff4444"
                                         : parent.cellVal === 2 ? "#ff8888"
                                         : "#556"
                                }
                            }
                        }
                    }
                }
            }
        }

        // ── Ship Roster ────────────────────────────────────────────
        RowLayout {
            spacing: 32
            Layout.alignment: Qt.AlignHCenter

            // Your ships
            ColumnLayout {
                spacing: 2
                Text {
                    text: "Your Ships"
                    font.pixelSize: 10
                    color: "#888"
                }
                Repeater {
                    model: 5
                    Text {
                        property bool sunk: root.humanShipsSunk[index]
                        text: root.shipNames[index] + " (" + root.shipSizes[index] + ")"
                        font.pixelSize: 10
                        font.strikeout: sunk
                        color: sunk ? "#666" : "#4aff4a"
                    }
                }
            }

            // AI ships
            ColumnLayout {
                spacing: 2
                Text {
                    text: "Enemy Ships"
                    font.pixelSize: 10
                    color: "#888"
                }
                Repeater {
                    model: 5
                    Text {
                        property bool sunk: root.aiShipsSunk[index]
                        text: root.shipNames[index] + " (" + root.shipSizes[index] + ")"
                        font.pixelSize: 10
                        font.strikeout: sunk
                        color: sunk ? "#666" : "#ff6b6b"
                    }
                }
            }
        }

        // ── Under the Hood ─────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#222"
            radius: 4
            border.color: "#333"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 2

                Text {
                    text: "— Under the Hood —"
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    color: "#666"
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: "Turn: " + root.turns
                        + "  |  You: " + root.humanHits + " hits / " + root.humanMisses + " misses"
                        + "  |  AI: " + root.aiHits + " hits / " + root.aiMisses + " misses"
                        + "  |  Ships: " + root.humanShips + "/5 vs " + root.aiShips + "/5"
                    font.pixelSize: 10
                    font.family: "monospace"
                    color: "#aaa"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // ── New Game button ────────────────────────────────────────
        Button {
            id: newGameBtn
            text: "New Game"
            font.pixelSize: 12
            Layout.fillWidth: true

            onClicked: callNewGame()

            background: Rectangle {
                color: newGameBtn.hovered ? "#3a3a3a" : "#2a2a2a"
                border.color: "#555"
                border.width: 1
                radius: 4
            }

            contentItem: Text {
                text: newGameBtn.text
                font: newGameBtn.font
                color: "#e0e0e0"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Item { Layout.fillHeight: true }
    }

    // ── Logos bridge helpers ───────────────────────────────────────

    function callModule(method, args) {
        if (typeof logos === "undefined" || !logos.callModule)
            return -1
        return logos.callModule("battleship", method, args)
    }

    function callNewGame() {
        callModule("newGame", [])
        root.lastAiRow = -1
        root.lastAiCol = -1
        root.lastAiResult = -1
        refreshAll()
    }

    function callAttack(row, col) {
        var err = callModule("attack", [row, col])
        if (err === 0) {
            // Read AI's last attack info
            root.lastAiRow = Number(callModule("aiAttackRow", [])) || -1
            root.lastAiCol = Number(callModule("aiAttackCol", [])) || -1
            root.lastAiResult = Number(callModule("aiShotResult", [])) || 0
        }
        refreshAll()
    }

    function refreshAll() {
        refreshOwnBoard()
        refreshAttackBoard()
        refreshStats()
        refreshShipStatus()
    }

    function refreshOwnBoard() {
        var b = []
        for (var r = 0; r < root.gridSize; r++)
            for (var c = 0; c < root.gridSize; c++)
                b.push(Number(callModule("getOwnCell", [r, c])) || 0)
        root.ownBoard = b
    }

    function refreshAttackBoard() {
        var b = []
        for (var r = 0; r < root.gridSize; r++)
            for (var c = 0; c < root.gridSize; c++)
                b.push(Number(callModule("getAttackCell", [r, c])) || 0)
        root.attackBoard = b
    }

    function refreshStats() {
        root.gameStatus = Number(callModule("status", [])) || 0
        root.currentPlayer = Number(callModule("currentPlayer", [])) || 0
        root.turns = Number(callModule("turnCount", [])) || 0
        root.humanHits = Number(callModule("hitCount", [0])) || 0
        root.humanMisses = Number(callModule("missCount", [0])) || 0
        root.aiHits = Number(callModule("hitCount", [1])) || 0
        root.aiMisses = Number(callModule("missCount", [1])) || 0
        root.humanShips = Number(callModule("shipsRemaining", [0])) || 0
        root.aiShips = Number(callModule("shipsRemaining", [1])) || 0
    }

    function refreshShipStatus() {
        var hs = []
        var as = []
        for (var i = 0; i < 5; i++) {
            hs.push(Number(callModule("shipSunk", [0, i])) === 1)
            as.push(Number(callModule("shipSunk", [1, i])) === 1)
        }
        root.humanShipsSunk = hs
        root.aiShipsSunk = as
    }

    function statusText() {
        if (root.gameStatus === 0) return "Setting up..."
        if (root.gameStatus === 2) return "VICTORY! All enemy ships destroyed!"
        if (root.gameStatus === 3) return "DEFEAT. Your fleet has been sunk."
        return "Your turn — click an enemy cell to fire"
    }
}
