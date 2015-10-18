import QtQuick 2.4
import QtQuick.Controls 1.3

ApplicationWindow {
    title: qsTr("Hello World")
    width: 640
    height: 480
    visible: true

    Client {
        id: client
    }

    Button {
        id: button1
        x: 282
        y: 227
        text: qsTr("Button")

        onClicked: {
//            client.testAction("123123");
            client.testAction({
                                  "key": "value"
                              });
        }
    }
}
