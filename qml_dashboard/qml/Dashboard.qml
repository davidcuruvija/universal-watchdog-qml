import QtQuick

Window {
    width: 480
    height: 640
    visible: true
    title: "Watchdog Dashboard"

    ListView {
        anchors.fill: parent
        model: watchdogModel

        delegate: Rectangle {
            width: ListView.view.width
            height: 48

            Row {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 16

                Text { text: model.name; font.bold: true }
                Text { text: model.state }
                Text { text: model.memory + " KB" }
                Text { text: model.cpu.toFixed(1) + "%" }
            }
        }
    }
}
