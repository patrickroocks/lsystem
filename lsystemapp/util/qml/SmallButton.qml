import QtQuick 2.11
import QtQuick.Shapes 1.15
import QtQuick.Controls 1.4
import QtQuick.Controls 2.15
import QtQuick.Controls.Private 1.0

Rectangle
{
    id: rectButton;

    property int sizeButton: 20

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
        anchors.centerIn: parent
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
        visible: mouseOver
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
