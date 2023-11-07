#ifndef ANGLEFORMULADIALOG_H
#define ANGLEFORMULADIALOG_H

#include <QDialog>

#include "angleevaluator.h"

namespace Ui {
class AngleFormulaDialog;
}

class AngleFormulaDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AngleFormulaDialog(QWidget * parent, const QString & initialLeftAngle, QString & rightAngle);
	~AngleFormulaDialog();

protected:
	void accept() override;

private slots:
	void on_cmbFormula_currentTextChanged(const QString & arg1);
	void on_txtExampleLeftAngle_textChanged(const QString & arg1);

private:
	void recalculateExample();

private:
	Ui::AngleFormulaDialog *ui;
	QString& strOut;
	AngleEvaluator angleEvaluator;
};

#endif // ANGLEFORMULADIALOG_H
