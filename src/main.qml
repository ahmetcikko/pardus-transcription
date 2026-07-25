import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true
    width: Screen.desktopAvailableWidth * 0.42
    height: Screen.desktopAvailableHeight * 0.52
    minimumWidth: Screen.desktopAvailableWidth * 0.24
    minimumHeight: Screen.desktopAvailableHeight * 0.3
    x: (Screen.desktopAvailableWidth - width) / 2
    y: (Screen.desktopAvailableHeight - height) / 2
    title: "Pardus Dikte"
    color: "#16171c"
    function statusText() {
        if (backend.state === "listening") return "Dinleniyor"
        if (backend.state === "transcribing") return "Yazıya dökülüyor"
        if (backend.state === "error") return "Mikrofon bulunamadı"
        return "Tamamlandı"
    }
    function statusColor() {
        if (backend.state === "listening") return "#2ec48d"
        if (backend.state === "transcribing") return "#2e7df6"
        if (backend.state === "error") return "#e05555"
        return "#7a818d"
    }
    component ActionButton: Button {
        id: control
        padding: window.height * 0.02
        background: Rectangle {
            radius: window.width * 0.012
            color: control.down ? "#21242c" : control.hovered ? "#2e323e" : "#262933"
            border.color: "#343846"
            border.width: 1
        }
        contentItem: Text {
            text: control.text
            color: control.enabled ? "#eceef3" : "#6e7481"
            font.pixelSize: window.height * 0.031
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: window.width * 0.028
        spacing: window.height * 0.024
        RowLayout {
            spacing: window.width * 0.016
            Rectangle {
                Layout.preferredWidth: window.height * 0.024
                Layout.preferredHeight: window.height * 0.024
                radius: width / 2
                color: window.statusColor()
                Behavior on color { ColorAnimation { duration: 220 } }
            }
            Text {
                text: window.statusText()
                color: "#eceef3"
                font.pixelSize: window.height * 0.036
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                Layout.preferredWidth: window.width * 0.2
                Layout.preferredHeight: window.height * 0.013
                radius: height / 2
                color: "#2c2f38"
                Rectangle {
                    width: parent.width * Math.min(1, backend.level * 9)
                    height: parent.height
                    radius: parent.radius
                    color: "#2ec48d"
                    Behavior on width { NumberAnimation { duration: 90 } }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: window.width * 0.016
            color: "#1e2027"
            border.color: "#2c2f38"
            border.width: 1
            ScrollView {
                anchors.fill: parent
                anchors.margins: window.width * 0.02
                TextArea {
                    id: editor
                    wrapMode: TextEdit.Wrap
                    color: "#eceef3"
                    font.pixelSize: window.height * 0.038
                    selectByMouse: true
                    placeholderText: "Konuşmaya başlayın, söyledikleriniz burada belirecek…"
                    background: Item {}
                }
            }
        }
        Text {
            text: "Metni düzenleyebilirsiniz. Bitirdiğinizde \"Dinlemeyi durdur\" düğmesine basın."
            color: "#7a818d"
            font.pixelSize: window.height * 0.026
        }
        RowLayout {
            spacing: window.width * 0.014
            ActionButton {
                text: "Dinlemeyi durdur"
                enabled: backend.state === "listening"
                onClicked: backend.stopListening()
            }
            ActionButton {
                text: "Kopyala"
                onClicked: backend.copyText(editor.text)
            }
            ActionButton {
                text: "Temizle"
                onClicked: editor.text = ""
            }
            Item { Layout.fillWidth: true }
            ActionButton {
                text: "Kapat"
                onClicked: backend.quitNow()
            }
        }
    }
    Connections {
        target: backend
        function onTranscriptChanged() {
            editor.text = backend.transcript
            editor.cursorPosition = editor.length
        }
    }
}
