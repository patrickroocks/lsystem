#pragma once

#include <QDialog>

namespace Ui {
class AboutDialog;
}

class AboutDialog final : public QDialog
{
	Q_OBJECT

public:
	explicit AboutDialog(QWidget * parent = nullptr);
	~AboutDialog();

private slots:
	void on_cmdClose_clicked();

private:
	Ui::AboutDialog * const ui;
};
