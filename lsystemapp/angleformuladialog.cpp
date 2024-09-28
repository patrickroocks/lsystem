#include "angleformuladialog.h"
#include "ui_angleformuladialog.h"

AngleFormulaDialog::AngleFormulaDialog(QWidget * parent, const QString & initialLeftAngle, QString & rightAngleResult)
	: QDialog(parent)
	, ui(new Ui::AngleFormulaDialog)
	, strOut(rightAngleResult)
{
	ui->setupUi(this);
	setFixedSize(size());

	ui->txtExampleLeftAngle->setText(initialLeftAngle);

	if (angleEvaluator.evaluate(initialLeftAngle, rightAngleResult).isFormula) {
		ui->cmbFormula->setCurrentText(rightAngleResult);
	}

	bool found = false;
	for (int i = 0; i < ui->cmbFormula->count(); ++i) {
		if (ui->cmbFormula->itemText(i) == rightAngleResult) {
			found = true;
			break;
		}
	}
	if (!found) {
		ui->cmbFormula->addItem(rightAngleResult);
	}

	rightAngleResult = QString();

	recalculateExample();

	setupConnections();
}

AngleFormulaDialog::~AngleFormulaDialog() { delete ui; }

void AngleFormulaDialog::setupConnections()
{
	connect(ui->cmbFormula, &QComboBox::currentTextChanged, this, &AngleFormulaDialog::recalculateExample);
	connect(ui->txtExampleLeftAngle, &QLineEdit::textChanged, this, &AngleFormulaDialog::recalculateExample);
	connect(ui->cmdDeleteFormula, &QPushButton::clicked, this, &AngleFormulaDialog::onCmdDeleteFormulaClicked);
}

void AngleFormulaDialog::accept()
{
	strOut = ui->cmbFormula->currentText();
	close();
}

void AngleFormulaDialog::recalculateExample()
{
	const AngleEvaluator::Result res = angleEvaluator.evaluate(ui->txtExampleLeftAngle->text(), ui->cmbFormula->currentText());
	ui->lblExampleRightAngle->setText(res.toString());
}

void AngleFormulaDialog::onCmdDeleteFormulaClicked() { ui->cmbFormula->setCurrentText(ui->lblExampleRightAngle->text()); }
