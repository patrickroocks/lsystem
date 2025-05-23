#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <configfilestore.h>

SettingsDialog::SettingsDialog(QWidget * parent, lsystem::ConfigFileStore * cfgStore)
	: QDialog(parent)
	, ui(new Ui::settingsdialog)
	, cfgStore(cfgStore)
	, settings(cfgStore->getSettings())
{
	ui->setupUi(this);
	//setFixedSize(size());

	ui->txtStackSize->setText(QString::number(cfgStore->getSettings().maxStackSize));
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

void SettingsDialog::accept()
{
	bool ok;
	const quint32 newStackSize = ui->txtStackSize->text().toUInt(&ok);
	if (ok) {
		settings.maxStackSize = newStackSize;
		cfgStore->saveSettings(settings);
	}
	close();
}

