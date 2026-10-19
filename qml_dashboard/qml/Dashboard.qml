import QtQuick
import QtQuick.Controls
import WatchdogDashboard

Window {
    width: 740
    height: 640
    minimumWidth: 740
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

    Row {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 8
        spacing: 8

        Button {
            id: addButton
            text: "Add Process"
            onClicked: addDialog.open()
        }

        Label {
            text: "Sort by:"
            anchors.verticalCenter: parent.verticalCenter
        }

        ComboBox {
            id: sortCombo
            model: ["Memory", "CPU", "Name"]
            onActivated: watchdogModel.setSortMode(currentText)
        }
    }

    Row {
        id: headerRow
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.topMargin: 12
        anchors.leftMargin: 16
        spacing: 16

        Text { text: "Name";   font.bold: true; width: 200 }
        Text { text: "State";  font.bold: true; width: 70 }
        Text { text: "Memory"; font.bold: true; width: 90 }
        Text { text: "CPU";    font.bold: true; width: 60 }
    }

    ListView {
        anchors.top: headerRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        model: watchdogModel

        delegate: Rectangle {
            width: ListView.view.width
            height: 56
            color: stateColor(model.state)

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#bdbdbd"
            }

            Row {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 16

                Text {
                    text: model.name
                    font.bold: true
                    width: 200
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: model.state
                    width: 70
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: model.memory + " KB"
                    width: 90
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: model.cpu.toFixed(1) + "%"
                    width: 60
                    anchors.verticalCenter: parent.verticalCenter
                }

                Button {
                    text: "Kill"
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: watchdogModel.kill(model.name)
                }

                Button {
                    text: "Restart"
                    anchors.verticalCenter: parent.verticalCenter
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
