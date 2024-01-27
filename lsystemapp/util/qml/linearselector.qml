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

    // for e.g. granuarity 0.1
    property alias fineStepSize: range.fineStepSize

    // for usual granularity, e.g. 1
    property alias coarseStepSize: range.coarseStepSize

    // The actual used step size
    property alias currentStepSize: range.currentStepSize

    // If fine steps are activated
    property alias isFineStepSize: range.isFineStepSize

    // multiply max value by this when clicking extension button
    property real extensionFactor: 0

    // for the C++ class for keyboard input handling (if up/down keys are relevant)
    property alias textHasFocus: textEdit.focus

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
    property alias isErr: range.valueError

    width: sizeWidth
    height: sizeHeight
    color: "#00FFFFFF" // transparent

    RangeModel
    {
        id: range
        minimumValue: 1
        maximumValue: 50
        value: 0
        fineStepSize: 0.1

        onValueChanged:
        {
            if (!textEdit.focus)
                updateText();
        }

        onValueErrorChanged:
        {
            updateRectColors();
        }
    }

    MouseArea
    {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent

        onPositionChanged:
        {
            if (pressed) {
                range.submitInputValue(valueFromPoint(mouseX, mouseY));
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
            text: ">"
            toolTipText: "Raise max value"
            onClicked:
            {
                range.maximumValue = Math.round(range.maximumValue * extensionFactor)
            }
        }

        ToggleButton
        {
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
            checked: isFineStepSize
            onCheckedChanged:
            {
                range.isFineStepSize = checked;
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
                range.submitInputValue(value - currentStepSize);
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
                range.submitInputValue(value + currentStepSize);
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
                range.submitInputString(text);
            }
        }

        onFocusChanged:
        {
            if (!focus) {
                updateText();
            }
        }
    }

    function lostFocus()
    {
        textEdit.focus = false;
    }

    // called from C++
    function setFineChecked(isFineChecked)
    {
        toggleStepButton.checked = isFineChecked
    }

    function setExtValue(val, focusToText)
    {
        range.submitInputValue(val, true);

        // If the text edit focus is on, we normally assume that the user is editing
        // and no text update is done after a change of the value.
        if (textEdit.focus)
            updateText();

        if (focusToText) {
            textEdit.focus = true;
            textEdit.cursorPosition = textEdit.text.length;
        }
    }

    function updateText()
    {
        if (range.currentStepSize < 1) {
            if (range.currentStepSize == 0.1) {
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
