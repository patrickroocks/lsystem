#pragma once

#include "configfilestore.h"

#include <QDialog>

namespace Ui {
class settingsdialog;
}

namespace lsystem {
class ConfigFileStore;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget * parent, lsystem::ConfigFileStore * cfgStore);
	~SettingsDialog();

protected:
	void accept() override;

private:
	Ui::settingsdialog * const ui;
	lsystem::ConfigFileStore * const cfgStore;
	lsystem::common::AppSettings settings;

};
