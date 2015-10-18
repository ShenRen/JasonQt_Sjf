import QtQuick 2.4
import QtQuick.Controls 1.3

ApplicationWindow {
    title: qsTr("Hello World")
    width: 400
    height: 400
    visible: true

    Client {
        id: client
    }

    Button {
        id: button1
        anchors.centerIn: parent
        text: qsTr("Button")

        onClicked: {
            client.testAction({
                                  "key": "value"
                              });
        }
    }
}
