import QtQuick 2.11
import QtQuick.Shapes 1.15
import QtQuick.Controls 1.4
import QtQuick.Controls 2.15
import QtQuick.Controls.Private 1.0

// Selector for a linear value

Rectangle
{
    // the actual value. induces a signal "valueChanged"
    property alias value: range.value
    property alias minValue: range.minimumValue
    property alias maxValue: range.maximumValue
    // current step size (might be modified with key shift)
    property alias rangeStepSize: range.stepSize

    // for keyboard input handling
    property bool textHasFocus: false

    // multiply max value by this when clicking extension button
    property real extensionFactor: 0

    // the geometrical size of the control
    property int sizeWidth: 100
    property int sizeHeight: 50
    property int sizeBorder: 10

    // internal properties
    property bool mousePressed: false
    property bool mouseOverRect: false
    property bool mouseOverExtButton: false
    property bool isErr: false

    width: sizeWidth
    height: sizeHeight
    color: "#00FFFFFF" // transparent

    RangeModel
    {
        id: range
        minimumValue: 1
        maximumValue: 50
        stepSize: 1
        value: 0
    }

    MouseArea
    {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent

        onPositionChanged:
        {
            if (pressed) {
                isErr = !setNewValue(valueFromPoint(mouseX, mouseY), true);
                updateRectColors();
            }
        }

        onPressedChanged:
        {
            textEdit.focus = false;
            mousePressed = pressed;
            updateRectColors();
        }

        onExited:
        {
            mouseOverRect = false;
            updateRectColors();
        }

        onEntered:
        {
            mouseOverRect = true
            updateRectColors();
        }

        function valueFromPoint(x, y)
        {
            var fct = (x - sizeBorder) / (sizeWidth - 2 * sizeBorder);
            fct = Math.max(Math.min(fct, 1), 0);
            return minValue + fct * (maxValue - minValue);
        }
    }

    Rectangle
    {
        id: rectMain
        width: sizeWidth
        height: sizeHeight
        anchors.top: parent.top
        anchors.left: parent.left
        radius: 15
        antialiasing: true
        color: "white"
        layer // antialiasing
        {
            enabled: true
            samples: 4
        }

        border
        {
            color: "grey"
            width: 3
        }

        // slider
        Shape
        {
            visible: !isErr
            layer.enabled: true
            layer.samples: 4
            width: sizeWidth
            height: sizeHeight
            ShapePath
            {
                strokeColor: "grey"
                strokeWidth: 3
                startX: sizeBorder + (1.0 * (value - minValue) / (maxValue - minValue)) * (sizeWidth - 2 * sizeBorder);
                startY: sizeHeight - 20;
                PathLine { relativeX: 0; relativeY: 10 }
            }
        }

        // extension button
        Rectangle
        {
            id: rectExtButton
            visible: extensionFactor > 0
            width: 20
            height: 20
            radius: 5
            antialiasing: true
            x: sizeWidth - 20 - sizeBorder
            y: sizeBorder

            border
            {
                color: "grey"
                width: 2
            }

            Label
            {
                anchors.centerIn: parent
                text: '+'
            }

            MouseArea
            {
                id: mouseAreaExtButton
                hoverEnabled: true
                anchors.fill: parent

                onExited:
                {
                    mouseOverExtButton = false;
                    updateRectColors();
                }

                onEntered:
                {
                    mouseOverExtButton = true
                    updateRectColors();
                }

                onClicked:
                {
                    range.maximumValue = Math.round(range.maximumValue * extensionFactor)
                }
            }

            ToolTip
            {
                delay: 1000
                timeout: 2000
                visible: mouseOverExtButton
                text: "Raise max value"
            }
        }
    }

    // manual text input
    TextInput
    {
        id: textEdit
        text: ""
        color: "black"
        font.pixelSize: 20
        x: sizeWidth / 2 - 25
        y: 10
        width: 50
        horizontalAlignment: "AlignHCenter"
        selectByMouse: true

        onTextChanged:
        {
            if (focus) {
                var parsed = parseInt(text);
                isErr = !setStrValue(text, true);
                updateRectColors();
            }
        }

        onFocusChanged:
        {
            textHasFocus = focus;
            if (!focus) {
                isErr = !setStrValue(text, true);
                if (isErr) updateText();
                isErr = false;
                updateRectColors();
            }
        }
    }

    function lostFocus()
    {
        textEdit.focus = false;
    }

    function setStrValue(strVal, updateText)
    {
        var parsed;
        if (rangeStepSize < 1) {
            parsed = parseFloat(strVal);
        } else {
            parsed = parseInt(strVal);
        }
        return !isNaN(parsed) && setNewValue(parsed, updateText);
    }

    function setExtValue(val, focusToText)
    {
        setNewValue(val, true);
        if (focusToText) {
            textEdit.focus = true;
            textEdit.cursorPosition = textEdit.text.length;
        }
    }

    function setNewValue(newVal, updText)
    {
        if (newVal > maxValue || newVal < minValue) return false;
        value = newVal;
        if (updText) updateText();
        return true;
    }

    function updateText()
    {
        if (rangeStepSize < 1) {
            textEdit.text = value.toFixed(2);
        } else {
            textEdit.text = value;
        }
    }

    function updateRectColors()
    {
        if (isErr) {
            rectMain.color = "#fc5c00"; // light red
        } else if ((mouseOverRect && !mouseOverExtButton) || mousePressed) {
            rectMain.color = "#aadcf7" // light blue
        } else {
            rectMain.color = "white";
        }

        if (mouseOverExtButton && !mousePressed) {
            rectExtButton.color = "#aadcf7"
        } else {
            rectExtButton.color = "white";
        }
    }
}
