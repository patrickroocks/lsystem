import QtQuick
import QtQuick.Shapes
import QtQuick.Controls
import QtQuick.Particles

Rectangle {
    id: rectangle1

    property int sizeWidth: 250
    property int sizeHeight: 50
    property int sizeBorder: 10
    property int sizePlayButton: 30

    property alias maxValue: range.maximumValue
    property alias minValue: range.minimumValue
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

            Label
            {
                y: 15
                x: 5
                text: minValue
            }

            Label
            {
                x: shapeSlider.width - 35
                y: 15
                width: 30
                text: maxValue
                horizontalAlignment: Text.AlignRight
            }

            Label
            {
                x: shapeSlider.width / 2 - 15
                y: -5
                width: 30
                color: "#25b300"
                text: value
                horizontalAlignment: Text.AlignHCenter
            }

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
        }

        MouseArea {
            height: sizeHeight
            width: shapeSlider.width + 2 * sizeBorder
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
                range.submitInputValue(minValue + (maxValue - minValue) * ((mouseX - shapeSlider.x) / shapeSlider.width))
            }
        }

        SmallButton
        {
            id: playPause
            sizeButton: sizePlayButton
            x: shapeSlider.width + 2 * sizeBorder
            y: sizeBorder
            text: ""

            // play/pause icon
            Item {
                x: 8
                y: 5

                // play icon
                Shape {
                    ShapePath {
                        strokeWidth: 1
                        strokeColor: "black"
                        fillColor: "black"
                        startX: 0; startY: 0
                        PathLine { x: 0; y: 0 }
                        PathLine { x: 0; y: 20 }
                        PathLine { x: 15; y: 10 }
                    }
                    visible: !playing;
                }

                // pause icon
                Shape {
                    Rectangle {
                        x: 0
                        y: 0
                        width: 6
                        height: 20
                        color: "black"
                    }
                    Rectangle {
                        x: 9
                        y: 0
                        width: 6
                        height: 20
                        color: "black"
                    }
                    visible: playing;
                }
            }

            onClicked:
            {
                playing = !playing
            }
        }
    }

}

