import QtQuick
import QtQuick.Controls
import WatchdogDashboard

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

    Button {
        id: addButton
        text: "Add Process"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 8
        onClicked: addDialog.open()
    }

    ListView {
        anchors.top: addButton.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
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

    Dialog {
        id: addDialog
        title: "Add Process"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        TextField {
            id: pidField
            placeholderText: "PID"
            validator: IntValidator { bottom: 1 }
        }

        onAccepted: {
            if (pidField.text.length > 0) {
                const result = watchdogModel.addProcess(parseInt(pidField.text));
                if (result === WatchdogModel.NotFound) {
                    notFoundDialog.open();
                } else if (result === WatchdogModel.AlreadyMonitored) {
                    alreadyMonitoredDialog.open();
                }
            }
            pidField.text = "";
        }

        onRejected: pidField.text = ""
    }

    Dialog {
        id: notFoundDialog
        title: "Process Not Found"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok

        Text {
            text: "No process with that PID exists."
        }
    }

    Dialog {
        id: alreadyMonitoredDialog
        title: "Already Monitored"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok

        Text {
            text: "That process is already being monitored."
        }
    }
}
