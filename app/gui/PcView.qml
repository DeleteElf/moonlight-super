import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import ComputerModel 1.0

import ComputerManager 1.0
import StreamingPreferences 1.0
import SystemProperties 1.0
import SdlGamepadKeyNavigation 1.0

CenteredGridView {
    property ComputerModel computerModel : createModel()

    id: pcGrid
    focus: true
    activeFocusOnTab: true
    topMargin: 20
    bottomMargin: 5
    cellWidth: 310; cellHeight: 330;
    objectName: qsTr("Computers")

    Component.onCompleted: {
        // Don't show any highlighted item until interacting with them.
        // We do this here instead of onActivated to avoid losing the user's
        // selection when backing out of a different page of the app.
        currentIndex = -1
    }

    // Note: Any initialization done here that is critical for streaming must
    // also be done in CliStartStreamSegue.qml, since this code does not run
    // for command-line initiated streams.
    StackView.onActivated: {
        // Setup signals on CM
        ComputerManager.computerAddCompleted.connect(addComplete)

        // Highlight the first item if a gamepad is connected
        if (currentIndex === -1 && SdlGamepadKeyNavigation.getConnectedGamepads() > 0) {
            currentIndex = 0
        }
    }

    StackView.onDeactivating: {
        ComputerManager.computerAddCompleted.disconnect(addComplete)
    }

    function pairingComplete(error)
    {
        // Close the PIN dialog
        pairDialog.close()

        // Display a failed dialog if we got an error
        if (error !== undefined) {
            errorDialog.text = error
            errorDialog.helpText = ""
            errorDialog.open()
        }
    }

    function addComplete(success, detectedPortBlocking)
    {
        if (!success) {
            errorDialog.text = qsTr("Unable to connect to the specified PC.")

            if (detectedPortBlocking) {
                errorDialog.text += "\n\n" + qsTr("This PC's Internet connection is blocking Moonlight. Streaming over the Internet may not work while connected to this network.")
            }
            else {
                errorDialog.helpText = qsTr("Click the Help button for possible solutions.")
            }

            errorDialog.open()
        }
    }

    function createModel()
    {
        var model = Qt.createQmlObject('import ComputerModel 1.0; ComputerModel {}', parent, '')
        model.initialize(ComputerManager)
        model.pairingCompleted.connect(pairingComplete)
        model.connectionTestCompleted.connect(testConnectionDialog.connectionTestComplete)
        return model
    }

    Row {
        anchors.centerIn: parent
        spacing: 5
        visible: pcGrid.count === 0

        BusyIndicator {
            id: searchSpinner
            visible: StreamingPreferences.enableMdns
        }

        Label {
            height: searchSpinner.height
            elide: Label.ElideRight
            text: StreamingPreferences.enableMdns ? qsTr("Searching for compatible hosts on your local network...")
                                                  : qsTr("Automatic PC discovery is disabled. Add your PC manually.")
            font.pointSize: 20
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }
    }

    model: computerModel

    delegate: NavigableItemDelegate {
        width: 300; height: 160;//每个电脑实例的宽高
        grid: pcGrid

        property alias pcContextMenu : pcContextMenuLoader.item
        //每个电脑的样式
        Image {
            id: pcIcon
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/res/desktop_windows-48px.svg"
            sourceSize {
                width: 100
                height: 100
            }
        }
        //每个电脑解锁前的样式
        Image {
            // TODO: Tooltip
            id: stateIcon
            anchors.horizontalCenter: pcIcon.horizontalCenter
            anchors.verticalCenter: pcIcon.verticalCenter
            anchors.verticalCenterOffset: !model.online ? -9 : -8
            visible: !model.statusUnknown && (!model.online || !model.paired)
            source: !model.online ? "qrc:/res/warning_FILL1_wght300_GRAD200_opsz24.svg" : "qrc:/res/baseline-lock-24px.svg"
            sourceSize {
                width: !model.online ? 37 : 35
                height: !model.online ? 37 : 35
            }
        }

        BusyIndicator {
            id: statusUnknownSpinner
            anchors.horizontalCenter: pcIcon.horizontalCenter
            anchors.verticalCenter: pcIcon.verticalCenter
            anchors.verticalCenterOffset: -15
            width: 75
            height: 75
            visible: model.statusUnknown
        }

        Label {
            id: pcNameText
            text: model.name

            width: parent.width
            anchors.top: pcIcon.bottom
            anchors.bottom: parent.bottom
            font.pointSize: 18
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            elide: Text.ElideRight
        }


        Dialog {
            id: loginDialog
            title:"登录主机"
            x: parent.width / 2 - width / 2
            y: parent.height / 2 - height / 2
            width: 300
            height: 310
            modal: true  // 如果需要模态对话框，设置为true
            //focus: true  // 使Popup获得焦点
            //closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside // 设置关闭策略
            // standardButtons: Dialog.Ok | Dialog.Cancel
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                TextField {
                   id: usernameField
                   placeholderText: qsTr("用户名")
                   Layout.fillWidth: true
                }
                TextField {
                   id: passwordField
                   placeholderText: qsTr("密码")
                   echoMode: TextInput.Password
                   Layout.fillWidth: true
                   onAccepted: {
                      login()
                   }
                }
                TextField {
                   id: nameField
                   placeholderText: qsTr("注册的计算机名,留空使用本机计算机名")
                   Layout.fillWidth: true
                   onAccepted: {
                      login()
                   }
                }
                RowLayout{
                    Button {
                        id: loginButton
                        text: "登录"
                        Layout.fillWidth: true  // 按钮宽度填满容器
                        Layout.preferredHeight: 50  // 明确按钮高度

                        // 自定义按钮内容（文字样式）
                        contentItem: Text {
                            text: loginButton.text
                            color: "white"                // 文字颜色
                            horizontalAlignment: Text.AlignHCenter // 水平居中
                            verticalAlignment: Text.AlignVCenter   // 垂直居中
                        }
                        // 自定义按钮背景
                        background: Rectangle {
                            radius: 5  // 圆角半径
                            // 按钮颜色（按下时变深色）
                            color: loginButton.down ? "#2980b9" : "#3498db"
                        }
                        // 点击事件处理
                        onClicked: {
                            login()
                        }
                    }
                    Button {
                        id: loginCancelButton
                        text: "取消"
                        //hovered: true
                        Layout.fillWidth: true  // 按钮宽度填满容器
                        //Layout.topMargin: 20
                        Layout.preferredHeight: 50  // 明确按钮高度
                        // 自定义按钮内容（文字样式）
                        contentItem: Text {
                            text: loginCancelButton.text
                            color: "white"                // 文字颜色
                            horizontalAlignment: Text.AlignHCenter // 水平居中
                            verticalAlignment: Text.AlignVCenter   // 垂直居中
                        }
                        // 自定义按钮背景
                        background: Rectangle {
                            radius: 5  // 圆角半径
                            // 按钮颜色（按下时变深色）
                            color: loginButton.down ? "#939393" : "#939393"
                        }
                        // 点击事件处理
                        onClicked: {
                            loginDialog.close()
                        }
                    }
                    ToolTip {
                       id: loginCancelButtonTT
                       text: "按[esc]或点击取消"
                       delay: 500
                       timeout: 5000
                       visible: loginCancelButton.hovered
                       x: loginCancelButton.x
                       y: loginCancelButton.y -  height // 垂直居中显示
                    }
                }
                // 错误提示文本
                Text {
                   id: errorMessage
                   //visible: false         // 默认隐藏
                   color: "#e74c3c"      // 红色错误提示
                   Layout.alignment: Qt.AlignHCenter // 居中显示
                }
            }
        }
        property bool logined:true //声明一个bool
        function login(){
            // 非空验证
            if(usernameField.text === "" || passwordField.text === ""){
                errorMessage.text = "用户名和密码不能为空！"
            } else {
                // 此处应添加实际登录验证逻辑 我们使用 /api/config 这个api来校验账号密码是否正确，通过验证 status 是否为true
                logined= computerModel.checkServerConfig(index,usernameField.text,passwordField.text)

                if(logined){
                    errorMessage.text =""
                    var pin = computerModel.generatePinString() //生成pin码
                    computerModel.pairComputer(index, pin) //向远程主机形成pin通知
                    //todo: 使用远程后台直接完成pin匹配   这个过程需要在指定时间内完成
                   if(!computerModel.registePinToServer(index,usernameField.text,passwordField.text,pin,nameField.text)){
                       errorMessage.text ="pin注册失败！"
                   }else{
                       console.log("登录注册成功！")
                       errorMessage.text =""
                       loginDialog.accept()
                   }
                }else{
                    errorMessage.text ="登录失败，账号或密码错误！"
                }
            }
        }

        Loader {
            id: pcContextMenuLoader
            asynchronous: true
            sourceComponent: NavigableMenu {
                id: pcContextMenu
                MenuItem {
                    text: qsTr("PC Status: %1").arg(model.online ? qsTr("Online") : qsTr("Offline"))
                    font.bold: true
                    enabled: false
                }
                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("View All Apps")
                    onTriggered: {
                        var component = Qt.createComponent("AppView.qml")
                        var appView = component.createObject(stackView, {"computerIndex": index, "objectName": model.name, "showHiddenGames": true})
                        stackView.push(appView)
                    }
                    visible: model.online && model.paired
                }
                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("Wake PC")
                    onTriggered: computerModel.wakeComputer(index)
                    visible: !model.online && model.wakeable
                }
                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("Test Network")
                    onTriggered: {
                        computerModel.testConnectionForComputer(index)
                        testConnectionDialog.open()
                    }
                }

                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("Rename PC")
                    onTriggered: {
                        renamePcDialog.pcIndex = index
                        renamePcDialog.originalName = model.name
                        renamePcDialog.open()
                    }
                }
                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("Delete PC")
                    onTriggered: {
                        deletePcDialog.pcIndex = index
                        deletePcDialog.pcName = model.name
                        deletePcDialog.open()
                    }
                }
                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("View Details")
                    onTriggered: {
                        showPcDetailsDialog.pcDetails = model.details
                        showPcDetailsDialog.open()
                    }
                }
            }
        }

        onClicked: {
            if (model.online) { //设备如果在线
                if (!model.serverSupported) { //服务器不支持 moonlight客户端
                    errorDialog.text = qsTr("The version of GeForce Experience on %1 is not supported by this build of Moonlight. You must update Moonlight to stream from %1.").arg(model.name)
                    errorDialog.helpText = ""
                    errorDialog.open()
                }else if (model.paired) { //已经配对完成
                    // go to game view
                    var component = Qt.createComponent("AppView.qml")
                    var appView = component.createObject(stackView, {"computerIndex": index, "objectName": model.name})
                    stackView.push(appView)
                }else { //没有配对  //todo: 我们这边需要修改成先要求输入主机账号密码
                    if(StreamingPreferences.loginMode) {
                        loginDialog.title = "登录主机[" + model.name + "]"
                        loginDialog.open()
                    }else{
                        var pin = computerModel.generatePinString()
                        // Kick off pairing in the background
                        computerModel.pairComputer(index, pin)
                        // Display the pairing dialog
                        pairDialog.pin = pin
                        pairDialog.open()
                    }
                }
            } else if (!model.online) { //设备如果不在线，则直接弹出右键菜单
                // Using open() here because it may be activated by keyboard
                pcContextMenu.open()
            }
        }

        onPressAndHold: {
            // popup() ensures the menu appears under the mouse cursor
            if (pcContextMenu.popup) {
                pcContextMenu.popup()
            }
            else {
                // Qt 5.9 doesn't have popup()
                pcContextMenu.open()
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton;
            onClicked: {
                parent.pressAndHold()
            }
        }

        Keys.onMenuPressed: {
            // We must use open() here so the menu is positioned on
            // the ItemDelegate and not where the mouse cursor is
            pcContextMenu.open()
        }

        Keys.onDeletePressed: {
            deletePcDialog.pcIndex = index
            deletePcDialog.pcName = model.name
            deletePcDialog.open()
        }
    }

    ErrorMessageDialog {
        id: errorDialog

        // Using Setup-Guide here instead of Troubleshooting because it's likely that users
        // will arrive here by forgetting to enable GameStream or not forwarding ports.
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Setup-Guide"
    }

    NavigableMessageDialog {
        id: pairDialog

        // Pairing dialog must be modal to prevent double-clicks from triggering
        // pairing twice
        modal: true
        closePolicy: Popup.CloseOnEscape

        // don't allow edits to the rest of the window while open
        property string pin : "0000"
        text:qsTr("Please enter %1 on your host PC. This dialog will close when pairing is completed.").arg(pin)+"\n\n"+
             qsTr("If your host PC is running Sunshine, navigate to the Sunshine web UI to enter the PIN.")
        standardButtons: Dialog.Cancel
        onRejected: {
            // FIXME: We should interrupt pairing here
        }
    }

    NavigableMessageDialog {
        id: deletePcDialog
        // don't allow edits to the rest of the window while open
        property int pcIndex : -1
        property string pcName : ""
        text: qsTr("Are you sure you want to remove '%1'?").arg(pcName)
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: {
            computerModel.deleteComputer(pcIndex)
        }
    }

    NavigableMessageDialog {
        id: testConnectionDialog
        closePolicy: Popup.CloseOnEscape
        standardButtons: Dialog.Ok

        onAboutToShow: {
            testConnectionDialog.text = qsTr("Moonlight is testing your network connection to determine if any required ports are blocked.") + "\n\n" + qsTr("This may take a few seconds…")
            showSpinner = true
        }

        function connectionTestComplete(result, blockedPorts)
        {
            if (result === -1) {
                text = qsTr("The network test could not be performed because none of Moonlight's connection testing servers were reachable from this PC. Check your Internet connection or try again later.")
                imageSrc = "qrc:/res/baseline-warning-24px.svg"
            }
            else if (result === 0) {
                text = qsTr("This network does not appear to be blocking Moonlight. If you still have trouble connecting, check your PC's firewall settings.") + "\n\n" + qsTr("If you are trying to stream over the Internet, install the Moonlight Internet Hosting Tool on your gaming PC and run the included Internet Streaming Tester to check your gaming PC's Internet connection.")
                imageSrc = "qrc:/res/baseline-check_circle_outline-24px.svg"
            }
            else {
                text = qsTr("Your PC's current network connection seems to be blocking Moonlight. Streaming over the Internet may not work while connected to this network.") + "\n\n" + qsTr("The following network ports were blocked:") + "\n"
                text += blockedPorts
                imageSrc = "qrc:/res/baseline-error_outline-24px.svg"
            }

            // Stop showing the spinner and show the image instead
            showSpinner = false
        }
    }

    NavigableDialog {
        id: renamePcDialog
        property string label: qsTr("Enter the new name for this PC:")
        property string originalName
        property int pcIndex : -1;

        standardButtons: Dialog.Ok | Dialog.Cancel

        onOpened: {
            // Force keyboard focus on the textbox so keyboard navigation works
            editText.forceActiveFocus()
        }

        onClosed: {
            editText.clear()
        }

        onAccepted: {
            if (editText.text) {
                computerModel.renameComputer(pcIndex, editText.text)
            }
        }

        ColumnLayout {
            Label {
                text: renamePcDialog.label
                font.bold: true
            }

            TextField {
                id: editText
                placeholderText: renamePcDialog.originalName
                Layout.fillWidth: true
                focus: true

                Keys.onReturnPressed: {
                    renamePcDialog.accept()
                }

                Keys.onEnterPressed: {
                    renamePcDialog.accept()
                }
            }
        }
    }

    NavigableMessageDialog {
        id: showPcDetailsDialog
        property string pcDetails : "";
        text: showPcDetailsDialog.pcDetails
        imageSrc: "qrc:/res/baseline-help_outline-24px.svg"
        standardButtons: Dialog.Ok
    }

    ScrollBar.vertical: ScrollBar {}
}
