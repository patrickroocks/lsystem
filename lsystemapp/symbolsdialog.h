#ifndef SYMBOLSDIALOG_H
#define SYMBOLSDIALOG_H

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
	void on_cmdClose_clicked();
	void on_cmdCopy_clicked();

private:
	Ui::SymbolsDialog * ui;

	QString lastContent;
};

#endif // SYMBOLSDIALOG_H
