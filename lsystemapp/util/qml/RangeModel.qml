import QtQuick

Item
{
    property double minimumValue: 0
    property double maximumValue: 1
    property double currentStepSize: 1
    property double coarseStepSize: 1
    property double fineStepSize: 0   // 0 means: not available
    property double overrideStepSize: 0
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
            newVal = minimumValue;
        } else if (newVal > maximumValue) {
            newVal = maximumValue;
        } else {
            valueError = false;
        }
        return newVal;
    }

    function submitInputValue(val, modifyIsFine = false)
    {
        if (val === value)
            return;

        var discretizedValue = applyStep(val);
        if (modifyIsFine && overrideStepSize == 0 && fineStepSize > 0 && !isFineStepSize && discretizedValue !== val) {
            // We need the fine step size to represt the value more accurate.
            isFineStepSize = true;
            discretizedValue = applyStep(val);
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
        if (fineStepSize == 0) // fine step size not available!
            isFineStepSize = false;

        if (overrideStepSize > 0)
            currentStepSize = overrideStepSize;
        else
            currentStepSize = isFineStepSize ? fineStepSize : coarseStepSize;
    }

    onIsFineStepSizeChanged: updateStepSize();
    onFineStepSizeChanged: updateStepSize();
    onCoarseStepSizeChanged: updateStepSize();
    onOverrideStepSizeChanged: updateStepSize();
}
