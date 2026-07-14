import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 980
    height: 800
    minimumWidth: 400
    minimumHeight: 520
    visible: true
    title: qsTr("JJY Simulator")

    property int selectedSecond: jjyController.activeSecond
    property bool selectionPinned: false
    property string selectedField: ""
    property var selectedDefinition: jjyController.bitDefinitions[selectedSecond]
    property real selectedPulse: jjyController.frame[selectedSecond] || 0

    function selectSecond(second) {
        selectedSecond = second
        selectionPinned = true
        selectedField = ""
    }

    function selectField(field, second) {
        selectedField = field
        selectedSecond = second
        selectionPinned = true
    }

    function categoryColor(category) {
        switch (category) {
        case "marker": return "#a83a3a"
        case "minute": return "#276b9a"
        case "hour": return "#7257a5"
        case "day": return "#247a65"
        case "year": return "#8b6228"
        case "weekday": return "#8b3f6b"
        case "parity": return "#4d7890"
        case "control": return "#7e6a31"
        default: return "#4b4b4b"
        }
    }

    function pulseText(pulse) {
        if (pulse < 0.3) return qsTr("マーカー（0.2秒）")
        if (pulse < 0.7) return qsTr("ビット 1（0.5秒）")
        return qsTr("ビット 0（0.8秒）")
    }

    function cellOpacity(definition, pulse) {
        if (definition.category === "unused")
            return 0.32
        if (definition.category === "marker")
            return 1.0
        return pulse < 0.7 ? 1.0 : 0.52
    }

    Connections {
        target: jjyController
        function onActiveSecondChanged() {
            if (!window.selectionPinned)
                window.selectedSecond = jjyController.activeSecond
        }
    }

    header: ToolBar {
        Label {
            anchors.centerIn: parent
            text: qsTr("JJY 標準電波シミュレータ")
            font.bold: true
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true

        ColumnLayout {
            // ScrollView の contentItem 内では anchors.centerIn が横スクロール
            // 領域をはみ出すことがあるため、明示的な左右余白を使う。
            x: 20
            width: Math.max(360, scrollView.availableWidth - 40)
            spacing: 14

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 16
                text: jjyController.currentTime
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 18
            }

            Label {
                Layout.fillWidth: true
                text: jjyController.status
                horizontalAlignment: Text.AlignHCenter
                color: jjyController.running ? "#087f23" : "#777777"
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Switch {
                    text: qsTr("夏時間を有効にする")
                    checked: jjyController.summerTime
                    enabled: !jjyController.running && !jjyController.pending
                    onToggled: jjyController.summerTime = checked
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: parent.enabled ? "#f0f0f0" : "#777777"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: parent.indicator.width + parent.spacing
                    }
                }
                Button {
                    text: (jjyController.running || jjyController.pending) ? qsTr("停止") : qsTr("開始")
                    highlighted: !jjyController.running && !jjyController.pending
                    onClicked: (jjyController.running || jjyController.pending) ? jjyController.stop() : jjyController.start()
                }
            }

            GroupBox {
                title: qsTr("1. 送信波形（クリック／タップでビット詳細を表示）")
                Layout.fillWidth: true
                // 2段 × (バー、項目名、秒番号) と行間を収める。
                Layout.preferredHeight: 290

                Grid {
                    anchors.fill: parent
                    anchors.margins: 12
                    columns: 30
                    columnSpacing: 3
                    rowSpacing: 12

                    Repeater {
                        model: jjyController.frame
                        delegate: Column {
                            required property int index
                            required property var modelData
                            width: (parent.width - (parent.columns - 1) * parent.columnSpacing) / parent.columns
                            spacing: 2
                            property bool active: jjyController.activeSecond === index
                            property bool selected: window.selectedSecond === index
                            property var definition: jjyController.bitDefinitions[index]

                            Item {
                                width: parent.width
                                height: 74
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width
                                    height: parent.height * modelData
                                    radius: 2
                                    color: modelData < 0.3 ? "#bf2424" : modelData < 0.7 ? "#c68e00" : "#27833c"
                                    opacity: active ? 1.0 : 0.70
                                }
                                Rectangle {
                                    anchors.fill: parent
                                    color: "transparent"
                                    radius: 3
                                    border.width: selected ? 2 : (active ? 2 : 0)
                                    border.color: selected ? "#43a9ff" : "#ffffff"
                                }
                                TapHandler {
                                    onTapped: {
                                        window.selectSecond(index)
                                        detailPopup.open()
                                    }
                                }
                            }
                            Label {
                                width: parent.width
                                horizontalAlignment: Text.AlignHCenter
                                text: definition.name
                                font.pixelSize: 9
                                elide: Text.ElideRight
                                color: selected ? "#43a9ff" : "#bdbdbd"
                            }
                            Label {
                                width: parent.width
                                horizontalAlignment: Text.AlignHCenter
                                text: index
                                font.pixelSize: 9
                                color: active ? "white" : "#8f8f8f"
                            }
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("2. JJYタイムコードのフォーマット")
                Layout.fillWidth: true
                Layout.preferredHeight: 410

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Flow {
                        Layout.fillWidth: true
                        spacing: 6
                        Repeater {
                            model: jjyController.decodedSummary
                            delegate: Button {
                                required property var modelData
                                text: modelData.label + "：" + modelData.value
                                font.pixelSize: 12
                                onClicked: window.selectField(modelData.field, modelData.second)
                                background: Rectangle {
                                    radius: 4
                                    color: window.categoryColor(modelData.category)
                                    border.width: window.selectedField === modelData.field ? 2 : 0
                                    border.color: "#43a9ff"
                                }
                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: "white"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 10
                                    rightPadding: 10
                                }
                            }
                        }
                    }

                    Grid {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        columns: 10
                        columnSpacing: 5
                        rowSpacing: 5

                        Repeater {
                            model: jjyController.bitDefinitions
                            delegate: Rectangle {
                                required property int index
                                required property var modelData
                                width: (parent.width - (parent.columns - 1) * parent.columnSpacing) / parent.columns
                                height: (parent.height - (6 - 1) * parent.rowSpacing) / 6
                                radius: 4
                                color: window.categoryColor(modelData.category)
                                opacity: window.cellOpacity(modelData, jjyController.frame[index] || 0)
                                property bool fieldSelected: window.selectedField !== "" &&
                                                             window.selectedField === modelData.field
                                border.width: fieldSelected || window.selectedSecond === index ? 3 :
                                              jjyController.activeSecond === index ? 2 : 0
                                border.color: fieldSelected || window.selectedSecond === index ? "#43a9ff" : "white"

                                Column {
                                    anchors.centerIn: parent
                                    width: parent.width - 4
                                    spacing: 1
                                    Label {
                                        width: parent.width
                                        text: modelData.name
                                        horizontalAlignment: Text.AlignHCenter
                                        font.bold: true
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                        color: "white"
                                    }
                                    Label {
                                        width: parent.width
                                        visible: modelData.weight !== ""
                                        text: modelData.weight
                                        horizontalAlignment: Text.AlignHCenter
                                        font.pixelSize: 11
                                        color: "#e6e6e6"
                                    }
                                }
                                HoverHandler { id: hover }
                                ToolTip.visible: hover.hovered
                                ToolTip.delay: 300
                                ToolTip.text: qsTr("%1秒  %2%3\n%4\n現在の信号: %5")
                                    .arg(String(index).padStart(2, "0"))
                                    .arg(modelData.name)
                                    .arg(modelData.weight !== "" ? " / " + modelData.weight : "")
                                    .arg(modelData.description)
                                    .arg(window.pulseText(jjyController.frame[index] || 0))
                                TapHandler {
                                    onTapped: {
                                        window.selectSecond(index)
                                        detailPopup.open()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 16
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "#777777"
                    text: qsTr("セルの明度：明=ビット1、暗=ビット0、濃灰=未使用。白枠=現在秒、青枠=選択中の項目。")
                }
                Button {
                    text: qsTr("現在秒を表示")
                    onClicked: {
                        window.selectionPinned = false
                        window.selectedField = ""
                        window.selectedSecond = jjyController.activeSecond
                    }
                }
            }
        }
    }

    Popup {
        id: detailPopup
        parent: Overlay.overlay
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: Math.min(360, parent.width - 30)
        modal: false
        focus: true
        padding: 16
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        ColumnLayout {
            width: parent.width - 32
            spacing: 8
            Label {
                text: qsTr("%1秒：%2%3")
                    .arg(String(window.selectedSecond).padStart(2, "0"))
                    .arg(window.selectedDefinition ? window.selectedDefinition.name : "")
                    .arg(window.selectedDefinition && window.selectedDefinition.weight !== "" ?
                         " / " + window.selectedDefinition.weight : "")
                font.bold: true
                font.pixelSize: 18
            }
            Label {
                Layout.fillWidth: true
                text: window.selectedDefinition ? window.selectedDefinition.description : ""
                wrapMode: Text.WordWrap
            }
            Label {
                text: qsTr("現在の信号：%1").arg(window.pulseText(window.selectedPulse))
                font.bold: true
            }
            Button {
                text: qsTr("閉じる")
                Layout.alignment: Qt.AlignRight
                onClicked: detailPopup.close()
            }
        }
    }
}
