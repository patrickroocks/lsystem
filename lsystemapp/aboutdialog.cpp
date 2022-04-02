#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <version.h>
#include <QDate>

AboutDialog::AboutDialog(QWidget * parent)
	 : QDialog(parent)
	 , ui(new Ui::AboutDialog)
{
	ui->setupUi(this);

	// replace %1 and %2 in info string
	ui->lblAbout->setText(ui->lblAbout->text().arg(lsystem::common::Version, QDate::currentDate().toString("yyyy")));
}

AboutDialog::~AboutDialog()
{
	delete ui;
}

void AboutDialog::on_cmdClose_clicked()
{
	close();
}
