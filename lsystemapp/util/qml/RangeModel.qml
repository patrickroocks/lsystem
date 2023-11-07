import QtQuick

Item
{
    property double minimumValue: 0
    property double maximumValue: 1
    property double stepSize: 1
    property double value: 0

    onValueChanged:
    {
        value = Math.max(minimumValue,
                         Math.min(maximumValue,
                                  minimumValue + Math.round((value - minimumValue) / stepSize) * stepSize));
    }
}
