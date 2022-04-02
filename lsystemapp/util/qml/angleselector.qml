import QtQuick 2.11
import QtQuick.Shapes 1.15
import QtQuick.Controls 1.4
import QtQuick.Controls.Private 1.0

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

    // the actual value. induces a signal "valueChanged"
    property alias value: range.value

    // used to filter for negative values (-1) or positive values (+1)
    property double valueFilter: 0

    // for keyboard input handling
    property bool textHasFocus: false

    // current step size is rangeStepSize (might be modified with key shift) * rangeStepFactor
    property double rangeStepSize: 1
    property double rangeStepFactor: 1
    property double rangeStepEffective: rangeStepSize * rangeStepFactor
    property alias rangeStepSmall: toggleStepButton.checked

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
        stepSize: rangeStepEffective
        value: 0
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
        toolTipText: "Increase by " + rangeStepEffective.toFixed(1) + "°"
        onClicked:
        {
            setNewValue(value + rangeStepEffective, true)
        }
    }

    SmallButton
    {
        id: decreaseButton
        x: size/2 - distButtonX - width
        y: size/2 - sizeButton / 2
        text: "↶"
        toolTipText: "Decrease by " + rangeStepEffective.toFixed(1) + "°"
        onClicked:
        {
            setNewValue(value - rangeStepEffective, true)
        }
    }

    ToggleButton
    {
        property bool internalCheckedChange: false

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
        onCheckedChanged: {
            rangeStepFactor = checked ? 0.1 : 1;
            if (!internalCheckedChange) {
                updateText();
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
        anchors.centerIn: parent
        width: 50
        horizontalAlignment: "AlignHCenter"
        selectByMouse: true

        onTextChanged:
        {
            if (focus) {
                var parsed = parseFloat(text);
                if (parsed !== Math.round(parsed)) {
                    toggleStepButton.internalCheckedChange = true
                    toggleStepButton.checked = true;
                    toggleStepButton.internalCheckedChange = false
                }
                isErr = !setStrValue(text, false);
                updateCircleColor();
            }
        }

        onFocusChanged:
        {
            textHasFocus = focus;
            if (!focus) {
                isErr = !setStrValue(text, true);
                if (isErr) updateText();
                isErr = false;
                updateCircleColor();
            }
        }
    }

    function lostFocus()
    {
        textEdit.focus = false;
    }

    function setStrValue(strVal, updateText)
    {
        var parsed = parseFloat(strVal);
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
            textEdit.cursorPosition = textEdit.text.length - 1;
        }
    }

    function setNewValue(newVal, updText)
    {
        while (newVal > 180) newVal -= 360;
        while (newVal < -180) newVal += 360;
        value = newVal;
        if (updText) updateText();
        if (value * valueFilter < 0) return false;
        return true;
    }

    function updateText()
    {
        if (toggleStepButton.checked) {
            textEdit.text = value.toFixed(1) + "°";
        } else {
            textEdit.text = value.toFixed(0) + "°";
        }
    }

    function updateCircleColor()
    {
        if (isErr) {
            shapeCircle.fillColor = "#fc5c00"; // light red
        } else if (mouseOverCircle || mousePressed) {
            shapeCircle.fillColor = "#aadcf7" // light blue
        } else {
            shapeCircle.fillColor = "white";
        }
    }
}
