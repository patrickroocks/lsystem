#pragma once

#include <QDialog>

namespace Ui {
class SymbolsDialog;
}

class SymbolsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SymbolsDialog(QWidget * parent = nullptr);
	~SymbolsDialog();

	void setContent(const QString & content);
	void clearContent();

protected:
	void resizeEvent(QResizeEvent * event) override;

private slots:
	void onCmdCopyClicked();

private:
	Ui::SymbolsDialog * const ui;

	QString lastContent;
};
