#pragma once

#include <angleevaluator.h>

#include <QDialog>

namespace Ui {
class AngleFormulaDialog;
}

class AngleFormulaDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AngleFormulaDialog(QWidget * parent, const QString & initialLeftAngle, QString & rightAngleResult);
	~AngleFormulaDialog();

protected:
	void accept() override;

private slots:
	void onCmdDeleteFormulaClicked();

private:
	void setupConnections();
	void recalculateExample();

private:
	Ui::AngleFormulaDialog *ui;
	QString& strOut;
	AngleEvaluator angleEvaluator;
};
