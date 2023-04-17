import QtQuick
import QtQuick.Shapes
import QtQuick.Controls

// Selector for a linear value

Rectangle
{
    // the actual value. induces a signal "valueChanged"
    property alias value: range.value
    property alias minValue: range.minimumValue
    property alias maxValue: range.maximumValue
    // current step size (might be modified with key shift)
    property double rangeStepSize: 1
    property double rangeStepFactor: 1
    property double rangeStepEffective: rangeStepSize * rangeStepFactor

    // for keyboard input handling
    property bool textHasFocus: false

    // multiply max value by this when clicking extension button
    property real extensionFactor: 0

    // for e.g. granuarity 0.1
    property double fineStepSize: 0

    // the geometrical size of the control
    property int sizeWidth: 150
    property int sizeHeight: 50
    property int sizeBorder: 10
    property int sizeButton: 20
    property int sliderOuterHeight: 20
    property int sliderInnerHeight: 14
    property int widthToggleButton: 50

    // internal properties
    property bool mousePressed: false
    property bool mouseOverRect: false
    property bool isErr: false

    width: sizeWidth
    height: sizeHeight
    color: "#00FFFFFF" // transparent

    RangeModel
    {
        id: range
        minimumValue: 1
        maximumValue: 50
        stepSize: rangeStepEffective
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
    }

    function sliderStart()
    {
        return 2 * sizeBorder + sizeButton;
    }

    function sliderEnd()
    {
        return sizeWidth - 2 * sizeBorder - sizeButton;
    }

    function valueFromPoint(x, y)
    {
        var fct = (x - sliderStart()) / (sliderEnd() - sliderStart());
        fct = Math.max(Math.min(fct, 1), 0);
        return minValue + fct * (maxValue - minValue);
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

        // slider line
        Shape
        {
            layer.enabled: true
            layer.samples: 4
            width: sizeWidth
            height: sizeHeight

            ShapePath
            {
                strokeColor: "grey"
                strokeWidth: 1
                startX: sliderStart()
                startY: sizeHeight - sizeBorder - sliderOuterHeight / 2
                PathLine { x: sliderEnd(); y: sizeHeight - sizeBorder - sliderOuterHeight / 2 }
            }
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
                startX: sliderStart() + (1.0 * (value - minValue) / (maxValue - minValue)) * (sliderEnd() - sliderStart());
                startY: sizeHeight - sizeBorder - (sliderOuterHeight + sliderInnerHeight) / 2;
                PathLine { relativeX: 0; relativeY: sliderInnerHeight }
            }
        }

        SmallButton
        {
            id: extButton
            visible: extensionFactor > 0
            x: sizeWidth - sizeButton - sizeBorder
            y: sizeBorder
            text: "⨠"
            toolTipText: "Raise max value"
            onClicked:
            {
                range.maximumValue = Math.round(range.maximumValue * extensionFactor)
            }
        }

        ToggleButton
        {
            property bool internalCheckedChange: false

            id: toggleStepButton
            visible: fineStepSize > 0
            x: sizeBorder
            y: sizeBorder
            width: widthToggleButton
            toolTipText: {
                if (!checked)
                    "Set step size to ±" + fineStepSize
                else
                    "Set step size to ±1"
            }
            text: "±0.1"
            radius: 5
            onCheckedChanged: {
                rangeStepFactor = checked ? fineStepSize : 1;
                if (!internalCheckedChange) {
                    updateText();
                }
            }
        }

        SmallButton
        {
            id: decreaseButton
            x: sizeBorder
            y: sizeHeight - sizeButton - sizeBorder
            text: '-'
            onClicked:
            {
                setNewValue(value - rangeStepEffective, true)
            }
        }

        SmallButton
        {
            id: increaseButton
            x: sliderEnd() + sizeBorder
            y: sizeHeight - sizeButton - sizeBorder
            text: '+'
            onClicked:
            {
                setNewValue(value + rangeStepEffective, true)
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
                var parsed = parseFloat(text);
                if (fineStepSize > 0 && parsed !== Math.round(parsed)) {
                    toggleStepButton.internalCheckedChange = true
                    toggleStepButton.checked = true;
                    toggleStepButton.internalCheckedChange = false
                }
                isErr = !setStrValue(text, false);
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
        if (rangeStepEffective < 1) {
            parsed = parseFloat(strVal);
        } else {
            parsed = parseInt(strVal);
        }
        return !isNaN(parsed) && setNewValue(parsed, updateText);
    }

    function setFineChecked(isFineChecked)
    {
        toggleStepButton.internalCheckedChange = true;
        toggleStepButton.checked = isFineChecked
        toggleStepButton.internalCheckedChange = false;
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
        if (rangeStepEffective < 1) {
            if (rangeStepEffective == 0.1) {
                textEdit.text = value.toFixed(1);
            } else {
                textEdit.text = value.toFixed(2);
            }
        } else {
            textEdit.text = value;
        }
    }

    function updateRectColors()
    {
        if (isErr) {
            rectMain.color = "#fc5c00"; // light red
        } else if (mouseOverRect || mousePressed) {
            rectMain.color = "#aadcf7" // light blue
        } else {
            rectMain.color = "white";
        }
    }
}
