#ifndef ANGLEEVALUATOR_H
#define ANGLEEVALUATOR_H

#include <QJSEngine>


class AngleEvaluator
{
public:
	struct Result {
		bool isOk = false;
		bool isFormula = false;
		double angle;
		QString error;

		QString toString() const;
	};

	Result evaluate(const QString& leftAngle, const QString& rightAngleFormula);

private:
	QJSEngine qsEngine;
};

#endif // ANGLEEVALUATOR_H
