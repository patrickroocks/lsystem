#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#include "configfilestore.h"

namespace Ui {
class settingsdialog;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent, lsystem::ConfigFileStore & cfgStore);
	~SettingsDialog();

private slots:
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();

private:
	Ui::settingsdialog * ui;
	lsystem::ConfigFileStore & cfgStore;
	lsystem::common::AppSettings settings;

};

#endif // SETTINGSDIALOG_H
