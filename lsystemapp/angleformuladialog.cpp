#include "angleformuladialog.h"
#include "ui_angleformuladialog.h"

AngleFormulaDialog::AngleFormulaDialog(QWidget * parent, const QString & initialLeftAngle, QString & rightAngle) :
	QDialog(parent),
	ui(new Ui::AngleFormulaDialog),
	strOut(rightAngle)
{
	ui->setupUi(this);
	ui->txtExampleLeftAngle->setText(initialLeftAngle);

	if (angleEvaluator.evaluate(initialLeftAngle, rightAngle).isFormula) {
		ui->cmbFormula->setCurrentText(rightAngle);
	}

	bool found = false;
	for (int i = 0; i < ui->cmbFormula->count(); ++i) {
		if (ui->cmbFormula->itemText(i) == rightAngle) {
			found = true;
			break;
		}
	}
	if (!found) {
		ui->cmbFormula->addItem(rightAngle);
	}

	rightAngle = QString();

	recalculateExample();
}

AngleFormulaDialog::~AngleFormulaDialog()
{
	delete ui;
}

void AngleFormulaDialog::accept()
{
	strOut = ui->cmbFormula->currentText();
	close();
}

void AngleFormulaDialog::on_cmbFormula_currentTextChanged(const QString & arg1)
{
	Q_UNUSED(arg1);
	recalculateExample();
}

void AngleFormulaDialog::on_txtExampleLeftAngle_textChanged(const QString & arg1)
{
	Q_UNUSED(arg1);
	recalculateExample();
}

void AngleFormulaDialog::recalculateExample()
{
	const AngleEvaluator::Result res = angleEvaluator.evaluate(ui->txtExampleLeftAngle->text(), ui->cmbFormula->currentText());
	ui->lblExampleRightAngle->setText(res.toString());
}
