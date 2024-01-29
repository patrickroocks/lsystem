#pragma once

#include <QJSEngine>

class AngleEvaluator final
{
public:
	struct Result {
		bool isOk = false;
		bool isFormula = false;
		double angle = 0;
		QString error;

		QString toString() const;
	};

	Result evaluate(const QString& leftAngle, const QString& rightAngleFormula);

private:
	QJSEngine qsEngine;
};
