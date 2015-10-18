import QtQuick 2.0
import JasonQtSjf 1.0

JsfForTcpClientManageExtended {
    id: client

    Component.onCompleted: {
        begin();
    }

    signal testAction(var data)

    function onTestActionSucceed(received) {
        print("onTestActionSucceed", received["return"]);
    }

    function onTestActionFail() {
        print("onTestActionFail");
    }

    function onReadySettings(tcpDeviceSettings, tcpClientSettings) {
        tcpClientSettings["serverPort"] = 23410;
        readySettingsDone(tcpDeviceSettings, tcpClientSettings);
    }
}
