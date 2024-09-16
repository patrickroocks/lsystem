#include "symbolsdialog.h"
#include "ui_symbolsdialog.h"

#include <QClipboard>
#include <QResizeEvent>

SymbolsDialog::SymbolsDialog(QWidget * parent)
	: QDialog(parent)
	, ui(new Ui::SymbolsDialog)
{
	ui->setupUi(this);

	connect(ui->cmdClose, &QPushButton::clicked, this, &SymbolsDialog::close);
}

SymbolsDialog::~SymbolsDialog() { delete ui; }

void SymbolsDialog::setContent(const QString & content)
{
	if (content.isEmpty()) return;

	lastContent = content;
	ui->cmdCopy->setEnabled(true);

	// inserting zero-width-spaces allows linebreak at every position
	const QString zeroWidthSpace = "\u200B";
	QString rv;
	for (QChar c : content) {
		rv += c + zeroWidthSpace;
	}
	ui->txtSymbols->setText(rv);
}

void SymbolsDialog::clearContent()
{
	ui->cmdCopy->setEnabled(false);
	ui->txtSymbols->clear();
}

void SymbolsDialog::resizeEvent(QResizeEvent * event)
{
	const int border = 10;
	const int wdt = event->size().width();
	const int hgt = event->size().height();

	ui->txtSymbols->resize(wdt - 2 * border, hgt - ui->cmdClose->height() - 3 * border);
	auto labelGeom = ui->txtSymbols->geometry();
	auto buttonsGeom = ui->wdgButtons->geometry();
	ui->wdgButtons->setGeometry(
		labelGeom.right() - buttonsGeom.width(), labelGeom.bottom() + border, buttonsGeom.width(), buttonsGeom.height());
}

void SymbolsDialog::onCmdCopyClicked()
{
	QClipboard * clipboard = QGuiApplication::clipboard();
	clipboard->setText(lastContent);
}
