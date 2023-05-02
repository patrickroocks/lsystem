import QtQuick
import QtQuick.Shapes
import QtQuick.Controls
import QtQuick.Particles

Rectangle {
    id: rectangle1

    property int sizeWidth: 200
    property int sizeHeight: 50
    property int sizeBorder: 10
    property int sizePlayButton: 30

    property int maxValue: range.maximumValue
    property int minValue: range.minimumValue
    property alias value: range.value

    // induces "playingChanged"
    property bool playing: false

    width: sizeWidth
    height: sizeHeight
    color: "#00FFFFFF" // transparent

    RangeModel
    {
        id: range
        minimumValue: 1
        maximumValue: 10
        stepSize: 1
        value: 1
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

        Shape
        {
            id: shapeSlider
            x: sizeBorder
            y: sizeBorder
            width: sizeWidth - 3 * sizeBorder - sizePlayButton
            height: 30

            ShapePath
            {
                strokeColor: "black"
                strokeWidth: 2
                startX: 0
                startY: shapeSlider.height / 2
                PathLine { relativeX: shapeSlider.width; relativeY: 0 }
            }

            ShapePath
            {
                strokeColor: "black"
                strokeWidth: 2
                startX: 0;
                startY: 0;
                PathLine { relativeX: 0; relativeY: shapeSlider.height }
            }

            ShapePath
            {
                strokeColor: "black"
                strokeWidth: 2
                startX: shapeSlider.width;
                startY: 0;
                PathLine { relativeX: 0; relativeY: shapeSlider.height }
            }

            // circle
            ShapePath {
                id: shapeCircle
                fillColor: "red"
                strokeColor: "red"
                strokeWidth: 1
                capStyle: ShapePath.FlatCap

                PathAngleArc {
                    centerX: shapeSlider.width * (value - minValue) / (maxValue - minValue)
                    centerY: shapeSlider.height / 2
                    radiusX: 5
                    radiusY: 5
                    startAngle: 0
                    sweepAngle: 360
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onPositionChanged:
                {
                    if (pressedButtons & Qt.LeftButton) {
                        setValueByMouseX(mouseX)
                    }
                }

                onClicked:
                {
                    // first stop the replayer, then jump to the value
                    playing = false
                    setValueByMouseX(mouseX)
                }

                function setValueByMouseX(mouseX)
                {
                    value = minValue + (maxValue - minValue) * (mouseX / shapeSlider.width)
                }
            }
        }

        SmallButton
        {
            id: playPause
            yOffsetLabel: 5 // finetuning, UTF play/stop symbols seem not to be centered
            sizeButton: sizePlayButton
            x: shapeSlider.width + 2 * sizeBorder
            y: sizeBorder
            text: playing ? "⏸" : "⏵"

            onClicked:
            {
                playing = !playing
            }
        }
    }

}

