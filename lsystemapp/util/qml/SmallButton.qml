import QtQuick 2.11
import QtQuick.Shapes 1.15
import QtQuick.Controls

Rectangle
{
    id: rectButton;

    property int sizeButton: 20
    property int yOffsetLabel: 0

    property alias text: lblText.text
    property alias toolTipText: toolTip.text

    property bool mouseOver: false
    property bool mousePressed: false

    signal clicked() // -> subscribe to "onClicked"

    width: sizeButton
    height: sizeButton
    radius: 5
    antialiasing: true
    color: "white"

    Label
    {
        id: lblText
        font.pointSize: 0.7*sizeButton
        y: yOffsetLabel
        x: 0
        height: sizeButton
        width: sizeButton
        text: "a"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    border
    {
        color: "grey"
        width: 2
    }

    ToolTip
    {
        id: toolTip
        delay: 1000
        timeout: 2000
        visible: text && mouseOver
    }

    MouseArea
    {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent

        onPressedChanged:
        {
            mousePressed = pressed;
            updateColors();
        }

        onExited:
        {
            mouseOver = false;
            updateColors();
        }

        onEntered:
        {
            mouseOver = true;
            updateColors();
        }

        function updateColors()
        {
            if (mouseOver || mousePressed) {
                rectButton.color = "#aadcf7";
            } else {
                rectButton.color = "white";
            }
        }

        onClicked:
        {
            rectButton.clicked();
        }
    }
}
