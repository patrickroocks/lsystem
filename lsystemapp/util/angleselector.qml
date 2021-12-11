import QtQuick 2.11
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Shapes 1.15
import QtQuick.Controls 1.4
import QtQuick.Controls.Private 1.0

// Selector for an angle

Rectangle {
    width: size
    height: size
    color: "#00FFFFFF" // transparent

    // the geometrical size of the control
    property int size: 100

    // the actual value. induces a signal "valueChanged"
    property alias value: range.value

    // used to filter for negative values (-1) or positive values (+1)
    property int valueFilter: 0

    // key handling
    property bool textHasFocus: false;

    // step size is modified externally with KeyShift
    property int rangeStepSize: 1

    // internal properties (coloring while hover / error handling)
    property string circleColor: "white"
    property bool mousePressed: false;
    property bool mouseOverCircle: false;
    property bool isErr: false;

    RangeModel {
        id: range
        minimumValue: -180
        maximumValue: 180
        stepSize: rangeStepSize
        value: 0
    }

    Shape {
        width: size
        height: size
        anchors.top: parent.top
        anchors.left: parent.left
        // antialiasing:
        layer.enabled: true
        layer.samples: 4

        ShapePath {
            fillColor: circleColor
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

        Item {
            // arrow
            Shape {
                layer.enabled: true
                layer.samples: 4
                width: 100
                height: 100
                ShapePath {
                    strokeColor: "grey"
                    strokeWidth: 3
                    startX: size/2;
                    startY: size/4 - 5;
                    PathLine { x: size/2; y: size/8 - 5 }
                }
                transform: Rotation { origin.x: size/2; origin.y: size/2; angle: value}
            }
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
                var parsed = parseInt(text);
                isErr = !setStrValue(text, true);
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
        var parsed = parseInt(strVal);
        return !isNaN(parsed) && setNewValue(parsed, updateText);
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
        textEdit.text = value + "°";
    }

    function updateCircleColor()
    {
        if (isErr) {
            circleColor = "#fc5c00"; // light red
        } else if (mouseOverCircle || mousePressed) {
            circleColor = "#aadcf7" // light blue
        } else {
            circleColor = "white";
        }
    }
}
