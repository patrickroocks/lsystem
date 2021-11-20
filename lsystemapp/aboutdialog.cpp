#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <version.h>

AboutDialog::AboutDialog(QWidget * parent)
	 : QDialog(parent)
	 , ui(new Ui::AboutDialog)
{
	ui->setupUi(this);

	ui->lblAbout->setText(ui->lblAbout->text().arg(lsystem::common::Version));
}

AboutDialog::~AboutDialog()
{
	delete ui;
}

void AboutDialog::on_cmdClose_clicked()
{
	close();
}
