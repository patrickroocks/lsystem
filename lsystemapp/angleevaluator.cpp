#include "angleevaluator.h"


AngleEvaluator::Result AngleEvaluator::evaluate(const QString & leftAngle, const QString & rightAngleFormula)
{
	Result rv;

	if (rightAngleFormula.isEmpty()) {
		rv.error = "no value given";
		return rv;
	}

	bool ok;
	const double parseRes = rightAngleFormula.toDouble(&ok);
	if (ok) {
		// no formula?
		rv.isOk = true;
		rv.angle = parseRes;
		return rv;
	}

	leftAngle.toDouble(&ok);

	if (!ok) {
		rv.error = "invalid left angle";
		return rv;
	}

	const QString stmt = QString("l = %1; %2").arg(leftAngle, rightAngleFormula);
	const QJSValue evalRes = qsEngine.evaluate(stmt);

	if (evalRes.isError()) {
		rv.error = evalRes.toString();
		return rv;
	} else {
		rv.isOk = true;
		rv.isFormula = true;
		rv.angle = evalRes.toNumber();
		return rv;
	}
}

QString AngleEvaluator::Result::toString() const
{
	if (isOk) {
		return QString::number(angle);
	} else {
		return error;
	}
}
