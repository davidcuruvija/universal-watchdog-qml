import QtQuick
import QtQuick.Controls

Window {
    width: 640
    height: 640
    minimumWidth: 640
    visible: true
    title: "Watchdog Dashboard"

    function stateColor(state) {
        switch (state) {
            case "ALIVE":   return "#c8e6c9";
            case "STOPPED": return "#fff9c4";
            case "STUCK":   return "#ffcc80";
            case "ZOMBIE":  return "#d1c4e9";
            case "DEAD":    return "#ef9a9a";
            default:        return "#eeeeee";
        }
    }

    ListView {
        anchors.fill: parent
        model: watchdogModel

        delegate: Rectangle {
            width: ListView.view.width
            height: 48
            color: stateColor(model.state)

            Row {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 16

                Text { text: model.name; font.bold: true }
                Text { text: model.state }
                Text { text: model.memory + " KB" }
                Text { text: model.cpu.toFixed(1) + "%" }

                Button {
                    text: "Kill"
                    onClicked: watchdogModel.kill(model.name)
                }

                Button {
                    text: "Restart"
                    onClicked: watchdogModel.restart(model.name)
                }
            }
        }
    }
}
