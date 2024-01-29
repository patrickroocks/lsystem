import QtQuick
import QtQuick.Shapes
import QtQuick.Controls

// Selector for an angle

Rectangle
{
    // the geometrical size of the control
    property int size: 100

    property int distButtonY: 40
    property int distButtonX: 40
    property int sizeButton: 20
    property int widthToggleButton: 50
    property int sizeBorder: 10

    // for the C++ class for keyboard input handling (if up/down keys are relevant)
    property alias textHasFocus: textEdit.focus

    // the actual value. induces a signal "valueChanged"
    property alias value: range.value

    // used to filter for negative values (-1) or positive values (+1)
    property double valueFilter: 0

    // for e.g. granuarity 0.1
    property alias fineStepSize: range.fineStepSize

    // for usual granularity, e.g. 1
    property alias coarseStepSize: range.coarseStepSize

    // The actual used step size
    property alias currentStepSize: range.currentStepSize

    // If fine steps are activated
    property alias isFineStepSize: range.isFineStepSize

    // For huge steps (when shift is pressed)
    property alias overrideStepSize: range.overrideStepSize

    // internal properties (coloring while hover / error handling)
    property bool mousePressed: false
    property bool mouseOverCircle: false
    property bool isErr: false

    width: size
    height: size
    color: "#00FFFFFF" // transparent

    RangeModel
    {
        id: range
        minimumValue: -180
        maximumValue: 180
        fineStepSize: 0.1
        value: 0

        onValueChanged:
        {
            if (!textEdit.focus)
                updateText();
        }

        onValueErrorChanged:
        {
            updateCircleColor();
        }
    }

    Shape
    {
        width: size
        height: size
        anchors.top: parent.top
        anchors.left: parent.left
        // antialiasing:
        layer.enabled: true
        layer.samples: 4

        // circle
        ShapePath {
            id: shapeCircle
            fillColor: "white"
            strokeColor: "darkGrey"
            strokeWidth: 2
            capStyle: ShapePath.FlatCap

            PathAngleArc {
                centerX: size/2
                centerY: size/2
                radiusX: size/2 - 2
                radiusY: size/2 - 2
                startAngle: 0
                sweepAngle: 360
            }
        }

        // arrow
        Shape
        {
            layer.enabled: true
            layer.samples: 4
            width: 100
            height: 100
            ShapePath
            {
                strokeColor: "grey"
                strokeWidth: 3
                startX: size/2;
                startY: size/4 - 5;
                PathLine { x: size/2; y: size/8 - 5 }
            }
            transform: Rotation { origin.x: size/2; origin.y: size/2; angle: value}
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
                isErr = !setNewValue(valueFromPoint(mouseX, mouseY), true);
                updateCircleColor();
            }
        }

        onPressedChanged:
        {
            textEdit.focus = false;
            mousePressed = pressed;
            updateCircleColor();
        }

        onExited:
        {
            mouseOverCircle = false;
            updateCircleColor();
        }

        onEntered:
        {
            mouseOverCircle = true
            updateCircleColor();
        }

        function valueFromPoint(x, y)
        {
            var xC = x - size/2;
            var yC = y - size/2;
            var rv = (xC || yC) ? Math.atan2(yC, xC) * 180 / Math.PI : 0;
            return rv + 90;
        }
    }

    SmallButton
    {
        id: increaseButton
        x: size/2 + distButtonX
        y: size/2 - sizeButton / 2
        text: "↷"
        toolTipText: "Increase by " + currentStepSize.toFixed(1) + "°"
        onClicked:
        {
            setNewValue(value + currentStepSize, true)
        }
    }

    SmallButton
    {
        id: decreaseButton
        x: size/2 - distButtonX - width
        y: size/2 - sizeButton / 2
        text: "↶"
        toolTipText: "Decrease by " + currentStepSize.toFixed(1) + "°"
        onClicked:
        {
            setNewValue(value - currentStepSize, true)
        }
    }

    ToggleButton
    {
        id: toggleStepButton
        x: size/2 - widthToggleButton / 2
        y: size/2 + distButtonY - sizeButton
        width: widthToggleButton
        toolTipText: {
            if (!checked)
                "Set step size to ±0.1"
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

    // manual text input
    TextInput
    {
        id: textEdit
        text: ""
        color: "black"
        font.pixelSize: 20
        anchors.centerIn: parent
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

    function setExtValue(val, focusToText)
    {
        setNewValue(val, true);
        if (focusToText) {
            textEdit.focus = true;
            textEdit.cursorPosition = textEdit.text.length - 1;
        }
    }

    function setNewValue(newVal, updText)
    {
        while (newVal > 180) newVal -= 360;
        while (newVal < -180) newVal += 360;

        range.submitInputValue(newVal)

        if (updText) updateText();
        if (value * valueFilter < 0) return false;
        return true;
    }

    function updateText()
    {
        if (range.currentStepSize < 1) {
            textEdit.text = value.toFixed(1) + "°";
        } else {
            textEdit.text = value.toFixed(0) + "°";
        }
    }

    function updateCircleColor()
    {
        if (isErr || range.valueError) {
            shapeCircle.fillColor = "#fc5c00"; // light red
        } else if (mouseOverCircle || mousePressed) {
            shapeCircle.fillColor = "#aadcf7" // light blue
        } else {
            shapeCircle.fillColor = "white";
        }
    }
}
