import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#1a1a1a"

    readonly property int gridSize: 10
    readonly property int cellSize: 30
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

    Component.onCompleted: {
        callNewGame()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Text {
            text: "BATTLESHIP"
            font.pixelSize: 20
            font.weight: Font.Bold
            color: "#e0e0e0"
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: statusText()
            font.pixelSize: 13
            font.weight: Font.DemiBold
            color: root.gameStatus === 2 ? "#4aff4a"
                 : root.gameStatus === 3 ? "#ff4444"
                 : "#e0e0e0"
            Layout.alignment: Qt.AlignHCenter
        }

        // Two grids side by side
        RowLayout {
            spacing: 20
            Layout.alignment: Qt.AlignHCenter

            // Your Fleet
            ColumnLayout {
                spacing: 4
                Text {
                    text: "YOUR FLEET"
                    font.pixelSize: 10
                    color: "#888"
                    Layout.alignment: Qt.AlignHCenter
                }
                GridLayout {
                    columns: root.gridSize
                    rowSpacing: 1
                    columnSpacing: 1
                    Repeater {
                        model: root.gridCells
                        Rectangle {
                            width: root.cellSize
                            height: root.cellSize
                            property int r: Math.floor(index / root.gridSize)
                            property int c: index % root.gridSize
                            property int v: (root.ownBoard.length > index) ? root.ownBoard[index] : 0
                            property bool isAiHit: r === root.lastAiRow && c === root.lastAiCol

                            color: v === 2 ? "#cc2222"
                                 : v === 1 ? "#4a4a5a"
                                 : "#1a2a3a"
                            border.color: isAiHit ? "#ffcc00" : "#333"
                            border.width: isAiHit ? 2 : 1
                            radius: 2

                            Text {
                                anchors.centerIn: parent
                                text: parent.v === 2 ? "X" : parent.v === 1 ? "o" : ""
                                font.pixelSize: parent.v === 1 ? 8 : 12
                                font.weight: Font.Bold
                                color: parent.v === 2 ? "#ff8888" : "#6a6a7a"
                            }
                        }
                    }
                }
            }

            // Enemy Waters
            ColumnLayout {
                spacing: 4
                Text {
                    text: "ENEMY WATERS"
                    font.pixelSize: 10
                    color: "#888"
                    Layout.alignment: Qt.AlignHCenter
                }
                GridLayout {
                    columns: root.gridSize
                    rowSpacing: 1
                    columnSpacing: 1
                    Repeater {
                        model: root.gridCells
                        Rectangle {
                            width: root.cellSize
                            height: root.cellSize
                            property int r: Math.floor(index / root.gridSize)
                            property int c: index % root.gridSize
                            property int v: (root.attackBoard.length > index) ? root.attackBoard[index] : 0
                            property bool canFire: v === 0 && root.gameStatus === 1

                            color: v === 3 ? "#aa1111"
                                 : v === 2 ? "#cc4444"
                                 : v === 1 ? "#2a3a4a"
                                 : "#1a2a3a"
                            border.color: "#333"
                            border.width: 1
                            radius: 2

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (parent.canFire)
                                        callAttack(parent.r, parent.c)
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                text: parent.v === 3 ? "X"
                                    : parent.v === 2 ? "X"
                                    : parent.v === 1 ? "."
                                    : ""
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: parent.v >= 2 ? "#ff8888" : "#556"
                            }
                        }
                    }
                }
            }
        }

        // Stats panel
        Rectangle {
            Layout.fillWidth: true
            height: 44
            color: "#222"
            radius: 4
            border.color: "#333"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 1
                Text {
                    text: "Turn: " + root.turns
                        + "  |  You: " + root.humanHits + "h/" + root.humanMisses + "m"
                        + "  |  AI: " + root.aiHits + "h/" + root.aiMisses + "m"
                    font.pixelSize: 10
                    color: "#aaa"
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: "Your ships: " + root.humanShips + "/5"
                        + "  |  Enemy ships: " + root.aiShips + "/5"
                    font.pixelSize: 10
                    color: "#aaa"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        Button {
            id: newGameBtn
            text: "New Game"
            font.pixelSize: 12
            Layout.fillWidth: true
            onClicked: callNewGame()

            background: Rectangle {
                color: newGameBtn.hovered ? "#3a3a3a" : "#2a2a2a"
                border.color: "#555"
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
        if (Number(err) === 0) {
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
    }

    function statusText() {
        if (root.gameStatus === 0) return "Setting up..."
        if (root.gameStatus === 2) return "VICTORY! All enemy ships destroyed!"
        if (root.gameStatus === 3) return "DEFEAT. Your fleet has been sunk."
        return "Your turn - click an enemy cell to fire"
    }
}
