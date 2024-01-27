import QtQuick

Item
{
    property double minimumValue: 0
    property double maximumValue: 1
    property double currentStepSize: 1
    property double coarseStepSize: 1
    property double fineStepSize: 0   // 0 means: not available
    property double value: 0
    property bool valueError: false
    property bool isFineStepSize: false

    function applyStep(val)
    {
        return minimumValue + Math.round((val - minimumValue) / currentStepSize) * currentStepSize;
    }

    function applyValueLimits(val)
    {
        var newVal = val;
        if (newVal < minimumValue) {
            valueError = true;
            newVal = minimumValue;
        } else if (newVal > maximumValue) {
            valueError = true;
            newVal = maximumValue;
        } else {
            valueError = false;
        }
        return newVal;
    }

    function submitInputValue(val, modifyIsFine = false)
    {
        var discretizedValue = applyStep(val);
        if (modifyIsFine && !isFineStepSize && discretizedValue !== val) {
            isFineStepSize = true;
            return;
        }

        value = applyValueLimits(discretizedValue);
    }

    function submitInputString(val)
    {
        var parsedValue = parseFloat(val);
        if (isNaN(parsedValue)) {
            valueError = true;
            return;
        }

        submitInputValue(parsedValue, true);
    }

    onCurrentStepSizeChanged:
    {
        submitInputValue(value, false);
    }

    function updateStepSize()
    {
        currentStepSize = isFineStepSize ? fineStepSize : coarseStepSize;
    }

    onIsFineStepSizeChanged: updateStepSize();
    onFineStepSizeChanged: updateStepSize();
    onCoarseStepSizeChanged: updateStepSize();
}
