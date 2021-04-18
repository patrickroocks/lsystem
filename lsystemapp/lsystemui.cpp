#include "lsystemui.h"
#include "ui_lsystemui.h"

#include <util/print.h>

#include <QResizeEvent>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QClipboard>
#include <QGuiApplication>

using namespace lsystem;
using namespace lsystem::common;
using namespace util;

namespace  {

	const char * StatusDefaultStyle = "background-color: rgb(211, 215, 207); border: 1px solid black; margin: 2px 10px 2px 0px;";
	const char * StatusErrorStyle = "background-color: red; color: white; border: 1px solid black; margin: 2px 10px 2px 0px;";
	const char * StatusWarningStyle = "background-color: orange; color: white; border: 1px solid black; margin: 2px 10px 2px 0px;";

	const char * StatusDefaultText = "Left click into the drawing area to draw the figure, right click for options";

	const int StatusIntervalMs = 8000;
}

LSystemUi::LSystemUi(QWidget *parent)
	: QMainWindow(parent),
	  ui(new Ui::LSystemUi),
	  defModel(this),
	  configStore(this)
{
	ui->setupUi(this);

	resetStatus();
	errorDecayTimer.setInterval(StatusIntervalMs);
	connect(&errorDecayTimer, &QTimer::timeout, this, &LSystemUi::resetStatus);

	if (!configStore.loadConfig()) {
		showMessage("config file could not be parsed", MsgType::Error);
	}

	ui->tblDefinitions->setModel(&defModel);
	ui->tblDefinitions->setColumnWidth(0, 20);
	ui->tblDefinitions->setColumnWidth(1, 200);
	ui->tblDefinitions->setColumnWidth(2, 20);
	ui->tblDefinitions->setColumnWidth(3, 15);

	connect(ui->tblDefinitions->selectionModel(), &QItemSelectionModel::selectionChanged,
			&defModel, &DefinitionModel::selectionChanged);
	connect(&defModel, &DefinitionModel::deselect,
			[&]() { ui->tblDefinitions->setCurrentIndex(QModelIndex()); });
	connect(&defModel, &DefinitionModel::newStartSymbol, ui->lblStartSymbol, &QLabel::setText);
	connect(&defModel, &DefinitionModel::showError, [&](const QString & errMsg) { showMessage(errMsg, MsgType::Error); });
	connect(&defModel, &DefinitionModel::edited, this, &LSystemUi::configLiveEdit);

	drawArea.reset(new DrawArea(ui->wdgOut));
	connect(drawArea.data(), &DrawArea::mouseClick, this, &LSystemUi::drawAreaClick);
	connect(drawArea.data(), &DrawArea::enableUndoRedu, this, &LSystemUi::enableUndoRedo);

	drawAreaMenu.reset(new DrawAreaMenu(this)); // needs drawArea
	statusMenu.reset(new StatusMenu(this));

	ui->lstConfigs->setModel(&configStore);
}

LSystemUi::~LSystemUi()
{
	delete ui;
}

void LSystemUi::resizeEvent(QResizeEvent * event)
{
	const int border = 5;
	const int wdt = event->size().width() - border;
	const int hgt = event->size().height() - border;

	ui->topMainLayoutWidget->resize(wdt - ui->topMainLayoutWidget->x(), ui->topMainLayoutWidget->height());
	ui->layPaintFrameWidget->resize(wdt - ui->layPaintFrameWidget->x(), hgt - ui->layPaintFrameWidget->y());

	// size of wdgOut is not available at start
	drawArea->resize(ui->layPaintFrameWidget->size().width() - 20, ui->layPaintFrameWidget->size().height() - 30);
}

void LSystemUi::on_cmdAdd_clicked()
{
	defModel.add();
}

void LSystemUi::on_cmdRemove_clicked()
{
	defModel.remove();
}

void LSystemUi::startPaint(int x, int y)
{
	const ConfigSet configSet = getConfigSet();
	if (!configSet.valid) return;

	processResult(simulator.execAndExpand(configSet), x, y,
			drawAreaMenu->autoClearToggle->isChecked() || drawAreaMenu->autoPaintToggle->isChecked());
}

void LSystemUi::setBgColorClear()
{
	bool ok;
	drawArea->backColor = QColorDialog::getRgba(drawArea->backColor, &ok, this);
	if (ok) drawArea->clear();
}

void LSystemUi::showMessage(const QString & errorStr, MsgType msgType)
{
	ui->lblStatus->setText(errorStr);
	ui->lblStatus->setStyleSheet([&msgType]() {
			switch (msgType) {
			case MsgType::Info:    return StatusDefaultStyle;
			case MsgType::Warning: return StatusWarningStyle;
			case MsgType::Error:   return StatusErrorStyle;
			}
			return ""; // prevent compiler warning
		}());

	errorDecayTimer.start();
}

void LSystemUi::resetStatus()
{
	ui->lblStatus->setStyleSheet(StatusDefaultStyle);
	ui->lblStatus->setText(StatusDefaultText);
}

ConfigSet LSystemUi::getConfigSet()
{
	ConfigSet configSet;

	auto showVarError = [&](const QString & errorVar, const QString & extraInfo = QString()) {
		showMessage(QString("Invalid value for variable '%1'%2").arg(errorVar)
				.arg(extraInfo.isEmpty() ? "" : (" (" + extraInfo + ")")), MsgType::Error);
	};

	configSet.definitions = defModel.getDefinitions();
	bool ok;
	configSet.scaling = ui->txtScaleDown->text().toDouble(&ok);
	if (!ok) { showVarError("scaling"); return configSet; }
	configSet.turn.left = ui->txtLeft->text().toDouble(&ok);
	if (!ok || configSet.turn.left < 0) { showVarError("turn left", "must be a positive number"); return configSet; }
	configSet.turn.right = ui->txtRight->text().toDouble(&ok);
	if (!ok || configSet.turn.right > 0) { showVarError("turn right", "must be a negative number"); return configSet; }
	configSet.startAngle = ui->txtStartAngle->text().toDouble(&ok);
	if (!ok) { showVarError("start angle"); return configSet; }

	configSet.numIter = ui->txtIter->text().toUInt(&ok);
	if (!ok) { showVarError("iterations"); return configSet; }
	configSet.stepSize = ui->txtStep->text().toDouble(&ok);
	if (!ok) { showVarError("step size"); return configSet; }

	configSet.valid = true;
	return configSet;
}

void LSystemUi::setConfigSet(const ConfigSet & configSet)
{
	defModel.setDefinitions(configSet.definitions);
	ui->txtScaleDown ->setText(QString::number(configSet.scaling));
	ui->txtLeft      ->setText(QString::number(configSet.turn.left));
	ui->txtRight     ->setText(QString::number(configSet.turn.right));
	ui->txtStartAngle->setText(QString::number(configSet.startAngle));
	ui->txtIter      ->setText(QString::number(configSet.numIter));
	ui->txtStep      ->setText(QString::number(configSet.stepSize));
}

void LSystemUi::on_cmdStore_clicked()
{
	const ConfigSet c = getConfigSet();
	if (!c.valid) return;

	ConfigNameKind currentConfig = configStore.getConfigNameKindByIndex(ui->lstConfigs->currentIndex());
	bool ok;
	QString configName = QInputDialog::getText(this, "Config name",
			"Enter a name for the config:", QLineEdit::Normal,
			currentConfig.configName, &ok);

	if (!ok) return;

	configStore.storeConfig(configName, c);
}

void LSystemUi::on_cmdLoad_clicked()
{
	loadConfigByLstIndex(ui->lstConfigs->currentIndex());
}

void LSystemUi::on_lstConfigs_doubleClicked(const QModelIndex & index)
{
	loadConfigByLstIndex(index);
}

void LSystemUi::drawAreaClick(int x, int y, Qt::MouseButton button)
{
	ui->tblDefinitions->setFocus(); // removes focus from text fields
	ui->tblDefinitions->setCurrentIndex(QModelIndex()); // finishes editing

	if (button == Qt::MouseButton::LeftButton) {
		startPaint(x, y);
	} else if (button == Qt::MouseButton::RightButton) {
		drawAreaMenu->menu.exec(drawArea->mapToGlobal(QPoint(x, y)));
	};
}

void LSystemUi::loadConfigByLstIndex(const QModelIndex & index)
{
	ConfigSet config = configStore.getConfigByIndex(index);
	if (config.valid) {
		setConfigSet(config);
	} else {
		showMessage("Config could not be loaded", MsgType::Error);
	}
}

void LSystemUi::on_cmdDelete_clicked()
{
	ConfigNameKind currentConfig = configStore.getConfigNameKindByIndex(ui->lstConfigs->currentIndex());

	if (currentConfig.fromUser) {
		configStore.deleteConfig(currentConfig.configName);
	} else {
		showMessage("predefined config can't be deleted", MsgType::Error);
	}
}

void LSystemUi::enableUndoRedo(bool undoOrRedo)
{
	drawAreaMenu->undoAction->setEnabled( undoOrRedo);
	drawAreaMenu->redoAction->setEnabled(!undoOrRedo);
}

void LSystemUi::processResult(Simulator::ExecResult execResult, int x, int y, bool clear)
{
	lastX = x;
	lastY = y;

	if (execResult == Simulator::ExecResult::Invalid) {
		showMessage(simulator.getLastError(), MsgType::Error);
		return;
	}

	LineSegs segs = simulator.getSegments();
	drawArea->draw(segs, x, y, clear);

	const QString msgPainted = printStr("Painted %1 segments, size is %2 px, <a href=\"%3\">Show symbols</a>",
			segs.size(), drawArea->getLastSize(), Links::ShowSymbols);

	if (execResult == Simulator::ExecResult::Ok) {
		showMessage(msgPainted, MsgType::Info);
	} else if (execResult == Simulator::ExecResult::ExceedStackSize) {
		showMessage(simulator.getLastError() + " | " + msgPainted, MsgType::Warning);
	}
}

void LSystemUi::configLiveEdit()
{
	if (!drawAreaMenu->autoPaintToggle->isChecked()) return;

	if (lastX == -1 && lastY == -1) {
		showMessage("No start position given. Click on the drawing area first.", MsgType::Error);
		return;
	}

	ConfigSet configSet = getConfigSet();
	if (!configSet.valid) return;
	processResult(simulator.execAndExpand(configSet), lastX, lastY, true);
}

void LSystemUi::copyStatus()
{
	QClipboard * clipboard = QGuiApplication::clipboard();
	clipboard->setText(ui->lblStatus->text());
}

void LSystemUi::on_lblStatus_linkActivated(const QString & link)
{
	if (link == Links::NextIterations) {
		processResult(simulator.execWithDoubleStackSize(), lastX, lastY, true);
	} else if (link == Links::ShowSymbols) {
		showMessage(simulator.getActionStr(), MsgType::Info);
	} else {
		QMessageBox::critical(this, "Unknown link action", QString("Link action '%1' was not found").arg(link));
	}
}

void LSystemUi::on_lblStatus_mousePressed(QMouseEvent * event)
{
	if (event->button() == Qt::RightButton) {
		statusMenu->menu.exec(event->globalPos());
	}
}

void LSystemUi::on_txtStartAngle_textChanged(const QString & arg1)
{
	Q_UNUSED(arg1);
	configLiveEdit();
}

void LSystemUi::on_txtScaleDown_textChanged(const QString & arg1)
{
	Q_UNUSED(arg1);
	configLiveEdit();
}

void LSystemUi::on_txtRight_textChanged(const QString & arg1)
{
	Q_UNUSED(arg1);
	configLiveEdit();
}

void LSystemUi::on_txtIter_textChanged(const QString & arg1)
{
	Q_UNUSED(arg1);
	configLiveEdit();
}

void LSystemUi::on_txtLeft_textChanged(const QString & arg1)
{
	Q_UNUSED(arg1);
	configLiveEdit();
}

void LSystemUi::on_txtStep_textChanged(const QString & arg1)
{
	Q_UNUSED(arg1);
	configLiveEdit();
}

void LSystemUi::on_cmdAbout_clicked()
{
	AboutDialog dia(this);
	dia.setModal(true);
	dia.exec();
}

// ------------------------------------------------------

LSystemUi::DrawAreaMenu::DrawAreaMenu(LSystemUi * parent)
	: menu(parent),
	  clipboardMenu("Copy to clipboard", parent)
{
	undoAction = menu.addAction("Undo", &*parent->drawArea, &DrawArea::restoreLastImage, Qt::CTRL + Qt::Key_Z);
	undoAction->setEnabled(false);
	redoAction = menu.addAction("Redo", &*parent->drawArea, &DrawArea::restoreLastImage, Qt::CTRL + Qt::Key_Y);
	redoAction->setEnabled(false);

	autoClearToggle = menu.addAction("Auto clear");
	autoClearToggle->setCheckable(true);
	autoClearToggle->setChecked(true);

	autoPaintToggle = menu.addAction("Auto paint");
	autoPaintToggle->setCheckable(true);

	menu.addAction("Clear", &*parent->drawArea, &DrawArea::clear, Qt::CTRL + Qt::Key_C);
	menu.addAction("Set Bg-Color && Clear", parent, &LSystemUi::setBgColorClear, Qt::CTRL + Qt::Key_B);

	menu.addMenu(&clipboardMenu);
	clipboardMenu.addAction("Copy full area", &*parent->drawArea, &DrawArea::copyToClipboardFull);
	clipboardMenu.addAction("Copy last drawing", &*parent->drawArea, &DrawArea::copyToClipboardLastDrawing);

	// to get the shortcurts working
	parent->ui->menubar->addMenu(&menu);
	parent->ui->menubar->setNativeMenuBar(true);
}

LSystemUi::StatusMenu::StatusMenu(LSystemUi * parent)
	: menu(parent)
{
	menu.addAction("Copy to clipboard", &*parent, &LSystemUi::copyStatus);
}
