#include "lsystemui.h"
#include "ui_lsystemui.h"
#include "util/tableitemdelegate.h"
#include "version.h"

#include "aboutdialog.h"
#include "angleformuladialog.h"
#include "settingsdialog.h"

#include <util/containerutils.h>
#include <util/print.h>

#include <QClipboard>
#include <QColorDialog>
#include <QDebug>
#include <QGuiApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QResizeEvent>

using namespace lsystem;
using namespace lsystem::common;
using namespace lsystem::ui;
using namespace util;

namespace {

const char * StatusDefaultStyle = "background-color: rgb(211, 215, 207); border: 1px solid black;";
const char * StatusErrorStyle = "background-color: red; color: white; border: 1px solid black;";
const char * StatusWarningStyle = "background-color: orange; color: white; border: 1px solid black;";

const char * StatusDefaultText = "Left click into the drawing area to draw the figure, right click for options";
const char * SelAngleHintText = "Use up&down keys to modify the angle or drag by mouse. Shift restricts to multiple of 5.";
const char * SelLinearHintText = "Use up&down keys to modify the value or drag by mouse. Shift restricts to step size %1.";

// After this interval status messages are replaced by the default message.
const int StatusIntervalMs = 8000;

// After this interval we execute pending operations, if no new input from the user came in.
const int ExecPendingIntervalMs = 100;

} // namespace

LSystemUi::LSystemUi(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::LSystemUi)
	, defModel(this)
	, configList(this)
{
	ui->setupUi(this);

	setWindowTitle(util::printStr("lsystem %1 - An interactive simulator for Lindenmayer systems", Version));

	resetStatus();
	messageDecayTimer.setInterval(StatusIntervalMs);
	connect(&messageDecayTimer, &QTimer::timeout, this, &LSystemUi::resetStatus);

	exec.pendingTimer.setInterval(ExecPendingIntervalMs);
	connect(&exec.pendingTimer, &QTimer::timeout, this, &LSystemUi::invokeExecPending);

	connect(&configList, &ConfigList::configMapUpdated, &configFileStore, &ConfigFileStore::newConfigMap);
	connect(&configFileStore, &ConfigFileStore::loadedPreAndUserConfigs, &configList, &ConfigList::newPreAndUserConfigs);
	connect(&configFileStore, &ConfigFileStore::showError, this, &LSystemUi::showErrorInUi);
	connect(&configFileStore, &ConfigFileStore::newStackSize, &simulator, &Simulator::setMaxStackSize);

	simulator.moveToThread(&simulatorThread);
	connect(this, &LSystemUi::simulatorExecActionStr, &simulator, &Simulator::execActionStr);
	connect(this, &LSystemUi::simulatorExec, &simulator, &Simulator::execAndExpand);
	connect(&simulator, &Simulator::resultReceived, this, &LSystemUi::processSimulatorResult);
	connect(&simulator, &Simulator::actionStrReceived, this, &LSystemUi::processActionStr);
	connect(&simulator, &Simulator::errorReceived, this, &LSystemUi::showErrorInUi);
	simulatorThread.start();

	ui->lstConfigs->setModel(&configList);
	configFileStore.loadConfig();
	loadConfigByLstIndex(configList.index(0, 0));

	ui->tblDefinitions->setModel(&defModel);
	ui->tblDefinitions->setColumnWidth(0, 20);
	ui->tblDefinitions->setColumnWidth(1, 200);
	ui->tblDefinitions->setColumnWidth(2, 20);
	ui->tblDefinitions->setColumnWidth(3, 15);
	ui->tblDefinitions->setColumnWidth(4, 15);
	ui->tblDefinitions->setAcceptDrops(true);
	ui->tblDefinitions->setStyleSheet("QHeaderView::section { background-color: #CCCCCC }");
	tableItemDelegate.reset(new TableItemDelegateAutoUpdate);
	ui->tblDefinitions->setItemDelegate(tableItemDelegate.data());

	connect(ui->tblDefinitions->selectionModel(), &QItemSelectionModel::selectionChanged,
			&defModel, &DefinitionModel::selectionChanged);
	connect(&defModel, &DefinitionModel::deselect, [this]() { ui->tblDefinitions->setCurrentIndex(QModelIndex()); });
	connect(&defModel, &DefinitionModel::getSelection, [this]() { return ui->tblDefinitions->currentIndex(); });
	connect(&defModel, &DefinitionModel::newStartSymbol, ui->lblStartSymbol, &QLabel::setText);
	connect(&defModel, &DefinitionModel::showError, this, &LSystemUi::showErrorInUi);
	connect(&defModel, &DefinitionModel::edited, this, &LSystemUi::configLiveEdit);

	// DrawArea is a widget and can't be moved to a new thread
	drawArea.reset(new DrawArea(ui->wdgOut));
	drawArea->setMouseTracking(true); // for mouse move event
	connect(drawArea.data(), &DrawArea::mouseClick, this, &LSystemUi::drawAreaClick);
	connect(drawArea.data(), &DrawArea::enableUndoRedo, this, &LSystemUi::enableUndoRedo);

	lblDrawActions.reset(new ClickableLabel(drawArea.data()));
	lblDrawActions->setVisible(false);
	lblDrawActions->setMouseTracking(true);
	lblDrawActions->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
	lblDrawActions->setStyleSheet("QLabel { background-color: white; border: 2px solid white;}");
	connect(lblDrawActions.data(), &ClickableLabel::linkActivated, this, &LSystemUi::processDrawAction);

	drawAreaMenu.reset(new DrawAreaMenu(this)); // needs drawArea
	statusMenu.reset(new StatusMenu(this));

	connect(drawArea.data(), &DrawArea::markingChanged, this, &LSystemUi::markDrawing);
	connect(drawArea.data(), &DrawArea::highlightChanged, this, &LSystemUi::highlightDrawing);

	quickAngle.reset(new QuickAngle(ui->centralwidget));
	quickAngle->setVisible(false);
	connect(quickAngle.data(), &QuickAngle::focusOut, this, &LSystemUi::unfocusAngleEdit);

	quickLinear.reset(new QuickLinear(ui->centralwidget));
	quickLinear->setVisible(false);
	connect(quickLinear.data(), &QuickLinear::focusOut, this, &LSystemUi::unfocusLinearEdit);

	ui->txtStartAngle->setValueRestriction(ValueRestriction::Numbers);
	ui->txtLeft->setValueRestriction(ValueRestriction::PositiveNumbers);
	ui->txtRight->setValueRestriction(ValueRestriction::NegativeNumbers);

	// * connections for live edits
	const std::vector<FocusableLineEdit*> configEditsLinear = {
			ui->txtIter, ui->txtStep, ui->txtScaleDown,
			ui->txtLastIterOpacity, ui->txtThickness, ui->txtOpacity};

	const std::vector<FocusableLineEdit*> allLinearEdits =  util::concatenateVectors(configEditsLinear, {ui->txtLatency});

	const std::vector<FocusableLineEdit*> configEditsAngle = {ui->txtStartAngle, ui->txtLeft, ui->txtRight};

	const std::vector<FocusableLineEdit*> allEdits = util::concatenateVectors(allLinearEdits, configEditsAngle);
	const std::vector<FocusableLineEdit*> allConfigEdits = util::concatenateVectors(configEditsLinear, configEditsAngle);

	for (FocusableLineEdit * lineEdit : allConfigEdits) {
		connect(lineEdit, &FocusableLineEdit::textChanged, this, &LSystemUi::configLiveEdit);
	}

	// connection for interactive sliders
	for (FocusableLineEdit * lineEdit : configEditsAngle) {
		connect(lineEdit, &FocusableLineEdit::gotFocus, this, &LSystemUi::focusAngleEdit);
	}

	for (FocusableLineEdit * lineEdit : allLinearEdits) {
		connect(lineEdit, &FocusableLineEdit::gotFocus, this, &LSystemUi::focusLinearEdit);
	}

	connect(ui->txtLatency, &FocusableLineEdit::textChanged, this, &LSystemUi::latencyChanged);

	connect(ui->chkAutoPaint, &QCheckBox::stateChanged, this, &LSystemUi::checkAutoPaintChanged);

	segDrawer.moveToThread(&segDrawerThread);
	connect(this, &LSystemUi::startDraw, &segDrawer, &SegmentDrawer::startDraw);
	connect(&segDrawer, &SegmentDrawer::drawDone, this, &LSystemUi::drawDone);
	segDrawerThread.start();

	// Segment animator (own thread does not really make sense, drawings have to be in the UI thread)
	segAnimator.reset(new SegmentAnimator());
	// clang-format off
	connect(this, &LSystemUi::startAnimateCurrentDrawing, segAnimator.get(), &SegmentAnimator::startAnimateCurrentDrawing);
	connect(this, &LSystemUi::setAnimateLatency,          segAnimator.get(), &SegmentAnimator::setAnimateLatency);
	connect(this, &LSystemUi::stopAnimate,                segAnimator.get(), &SegmentAnimator::stopAnimate);
	connect(this, &LSystemUi::goToAnimationStep,          segAnimator.get(), &SegmentAnimator::goToAnimationStep);
	// clang-format on
	connect(segAnimator.get(), &SegmentAnimator::newAnimationStep, this, &LSystemUi::newAnimationStep);

	// Player control (UI control for segment animator)
	connect(ui->playerControl, &PlayerControl::playPauseChanged, this, &LSystemUi::playPauseChanged);
	connect(ui->playerControl, &PlayerControl::playerValueChanged, this, &LSystemUi::playerValueChanged);

	ui->frmAdditionalOptions->setVisible(false);
	ui->frmPlayer->setVisible(false);
}

LSystemUi::~LSystemUi()
{
	quitAndWait({&simulatorThread, &segDrawerThread});
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
	drawArea->resize(ui->layPaintFrameWidget->size().width() - 20, ui->layPaintFrameWidget->size().height() - 50);

	// repaint during resize will fail!
	removeAllSliders();
}

void LSystemUi::on_cmdAdd_clicked()
{
	defModel.add();
}

void LSystemUi::on_cmdRemove_clicked()
{
	defModel.remove();
}

void LSystemUi::invokeExec(const QSharedPointer<DrawMetaData> & execMeta)
{
	exec.scheduledPending = false;

	// prevent that a queue of executions blocks everything.
	if (exec.active) {
		exec.pendingMeta = execMeta;
		return;
	} else {
		exec.pendingMeta = nullptr;
		exec.active = true;
		emit simulatorExec(execMeta);
	}
}

void LSystemUi::endInvokeExec()
{
	exec.active = false;
	if (exec.pendingMeta) {
		exec.scheduledPending = true;
		// This restarts the timer, even it was started before.
		// Don't use singleShot, it would start multiple timers.
		exec.pendingTimer.start();
	}
}

void LSystemUi::invokeExecPending()
{
	exec.pendingTimer.stop();
	if (exec.scheduledPending) {
		auto copiedPendingMeta = exec.pendingMeta;
		invokeExec(copiedPendingMeta);
	}
}

void LSystemUi::startPaint(int x, int y)
{
	const ConfigSet configSet = getConfigSet(true);
	if (!configSet.valid) return;

	QSharedPointer<DrawMetaData> execMeta(new DrawMetaData);
	execMeta->offset = QPoint{x, y};
	execMeta->clearAll = drawAreaMenu->autoClearToggle->isChecked() || ui->chkAutoPaint->isChecked();
	getAdditionalOptions(execMeta);
	execMeta->config = configSet;

	invokeExec(execMeta);
}

void LSystemUi::getAdditionalOptions(const QSharedPointer<MetaData> & execMeta)
{
	execMeta->showLastIter = ui->chkShowLastIter->isChecked();

	bool ok;
	const double lastIterOpacityPercent = ui->txtLastIterOpacity->text().toDouble(&ok);
	if (ok) execMeta->lastIterOpacy = lastIterOpacityPercent / 100.;

	const double opacityPercent = ui->txtOpacity->text().toDouble(&ok);
	if (ok) execMeta->opacity = opacityPercent / 100.;

	const double thickness = ui->txtThickness->text().toDouble(&ok);
	if (ok) execMeta->thickness = thickness;

	execMeta->antiAliasing = ui->chkAntiAliasing->isChecked();
}

void LSystemUi::openSymbolsDialog()
{
	if (!symbolsDialog) {
		symbolsDialog.reset(new SymbolsDialog(this));
		symbolsDialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
	}

	if (!symbolsDialog->isVisible()) {
		symbolsDialog->open();
	}
}

void LSystemUi::showSymbols()
{
	openSymbolsDialog();

	if (resultAvailable) {
		emit simulatorExecActionStr();
	}
}

void LSystemUi::showMarkedConfig()
{
	setConfigSet(drawArea->getMarkedDrawingResult()->config);
}

void LSystemUi::setBgColor()
{
	const QColor col = QColorDialog::getColor(drawArea->getBgColor(), this);
	if (col.isValid()) drawArea->setBgColor(col);
}

LSystemUi::DrawPlacement LSystemUi::getDrawPlacement() const
{
	LSystemUi::DrawPlacement rv;

	const auto drawing = highlightedDrawing.value();
	const auto& drawGeom = drawArea->geometry();
	rv.drawingSize = drawing.botRight - drawing.topLeft;
	rv.areaTopLeft = DrawPlacement::outerDist;
	rv.areaWidthHeight = QPoint(drawGeom.width(), drawArea->height());
	rv.areaBotRight = rv.areaWidthHeight - DrawPlacement::outerDist;
	rv.areaSize = rv.areaBotRight - rv.areaTopLeft;

	const double wdtFct = static_cast<double>(rv.areaSize.x()) / rv.drawingSize.x();
	const double hgtFct = static_cast<double>(rv.areaSize.y()) / rv.drawingSize.y();
	rv.fct = qMin(wdtFct, hgtFct);

	return rv;
}

void LSystemUi::showMessage(const QString & msg, MsgType msgType)
{
	ui->lblStatus->setText(msg);
	ui->lblStatus->setStyleSheet([&msgType]() {
		switch (msgType) {
		case MsgType::Info:
			return StatusDefaultStyle;
		case MsgType::Warning:
			return StatusWarningStyle;
		case MsgType::Error:
			return StatusErrorStyle;
		}
		return ""; // prevent compiler warning
	}());

	messageDecayTimer.start();
}

void LSystemUi::showVarError(const QString & errorVar, const QString & extraInfo) {
	showErrorInUi(QString("Invalid value for variable '%1'%2").arg(
		errorVar,
		extraInfo.isEmpty() ? "" : (" (" + extraInfo + ")")));
}

void LSystemUi::resetStatus()
{
	messageDecayTimer.stop();
	ui->lblStatus->setStyleSheet(StatusDefaultStyle);
	ui->lblStatus->setText(StatusDefaultText);
}

void LSystemUi::showSettings()
{
	SettingsDialog dia(this, configFileStore);
	dia.setModal(true);
	dia.exec();
}

void LSystemUi::removeAllSliders()
{
	if (quickAngle->isVisible()) unfocusAngleEdit();
	if (quickLinear->isVisible()) unfocusLinearEdit();
}

void LSystemUi::clearAll()
{
	drawArea->clear();
	if (symbolsDialog) {
		symbolsDialog->clearContent();
	}
}

ConfigSet LSystemUi::getConfigSet(bool storeAsLastValid)
{
	ConfigSet configSet;

	configSet.definitions = defModel.getDefinitions();

	bool ok;

	configSet.scaling = ui->txtScaleDown->text().toDouble(&ok);
	if (!ok) {
		showVarError("scaling");
		return configSet;
	}

	configSet.turn.left = ui->txtLeft->text().toDouble(&ok);
	if (!ok || configSet.turn.left < 0) {
		showVarError("turn left", "must be a positive number");
		return configSet;
	}

	// right angle can be a formula getting the left angle as an argument
	const AngleEvaluator::Result evalRes = angleEvaluator.evaluate(ui->txtLeft->text(), ui->txtRight->text());
	if (evalRes.isOk) {
		configSet.turn.right = evalRes.angle;
		if (configSet.turn.right > 0) {
			showVarError("turn right", "must be a negative number");
			return configSet;
		}
	} else {
		showVarError("turn right", evalRes.error);
		return configSet;
	}

	configSet.startAngle = ui->txtStartAngle->text().toDouble(&ok);
	if (!ok) {
		showVarError("start angle");
		return configSet;
	}

	configSet.numIter = ui->txtIter->text().toUInt(&ok);
	if (!ok) {
		showVarError("iterations");
		return configSet;
	}

	configSet.stepSize = ui->txtStep->text().toDouble(&ok);
	if (!ok) {
		showVarError("step size");
		return configSet;
	}

	configSet.valid = true;

	if (storeAsLastValid && configSet.valid) {
		lastValidConfigSet = configSet;
	}

	return configSet;
}

void LSystemUi::setConfigSet(const ConfigSet & configSet)
{
	disableConfigLiveEdit = true;
	defModel.setDefinitions(configSet.definitions);
	// clang-format off
	ui->txtScaleDown ->setText(QString::number(configSet.scaling));
	ui->txtLeft      ->setText(QString::number(configSet.turn.left));
	ui->txtRight     ->setText(QString::number(configSet.turn.right));
	ui->txtStartAngle->setText(QString::number(configSet.startAngle));
	ui->txtIter      ->setText(QString::number(configSet.numIter));
	ui->txtStep      ->setText(QString::number(configSet.stepSize));
	// clang-format on
	disableConfigLiveEdit = false;

	// still initialization phase => do not draw
	if (!drawArea) return;

	configLiveEdit();
}

void LSystemUi::on_cmdStore_clicked()
{
	const ConfigSet c = getConfigSet(false);
	if (!c.valid) return;

	ConfigNameKind currentConfig = configList.getConfigNameKindByIndex(ui->lstConfigs->currentIndex());
	bool ok;
	QString configName = QInputDialog::getText(this, "Config name",
			"Enter a name for the config:", QLineEdit::Normal,
			currentConfig.configName, &ok);

	if (!ok) return;

	configList.storeConfig(configName, c);
}

void LSystemUi::on_cmdLoad_clicked()
{
	loadConfigByLstIndex(ui->lstConfigs->currentIndex());
}

void LSystemUi::on_lstConfigs_doubleClicked(const QModelIndex & index)
{
	loadConfigByLstIndex(index);
}

void LSystemUi::drawAreaClick(int x, int y, Qt::MouseButton button, bool drawingMarked)
{
	ui->tblDefinitions->setFocus(); // removes focus from text fields
	ui->tblDefinitions->setCurrentIndex(QModelIndex()); // finishes editing

	if (button == Qt::MouseButton::LeftButton && !drawingMarked) {
		startPaint(x, y);
	} else if (button == Qt::MouseButton::RightButton) {
		drawAreaMenu->menu.exec(drawArea->mapToGlobal(QPoint(x, y)));
	};
}

void LSystemUi::loadConfigByLstIndex(const QModelIndex & index)
{
	ConfigSet config = configList.getConfigByIndex(index);
	if (config.valid) {
		setConfigSet(config);
	} else {
		showMessage("Config could not be loaded", MsgType::Error);
	}
}

void LSystemUi::on_cmdDelete_clicked()
{
	ConfigNameKind currentConfig = configList.getConfigNameKindByIndex(ui->lstConfigs->currentIndex());

	if (currentConfig.fromUser) {
		configList.deleteConfig(currentConfig.configName);
	} else {
		showMessage("predefined config can't be deleted", MsgType::Error);
	}
}

void LSystemUi::enableUndoRedo(bool undoOrRedo)
{
	drawAreaMenu->undoAction->setEnabled( undoOrRedo);
	drawAreaMenu->redoAction->setEnabled(!undoOrRedo);
}

void LSystemUi::highlightDrawing(std::optional<DrawResult> drawResult)
{
	highlightedDrawing = drawResult;

	lblDrawActions->setVisible(false);
	if (!drawResult.has_value()) {
		return;
	}

	drawPlacement = getDrawPlacement();
	const auto& dp = drawPlacement;

	// first guess label pos
	const auto& drawing = drawResult.value();
	QPoint labelPos = drawing.topLeft + DrawPlacement::outerDist;

	// Always display maxize button
	const bool showMax = true;

	// check if drawing can be moved left/right/top/down
	const bool moveRight = (drawing.topLeft.x() < dp.areaTopLeft.x());
	const bool moveDown = (drawing.topLeft.y() < dp.areaTopLeft.y());
	const bool moveLeft = (drawing.botRight.x() > dp.areaBotRight.x());
	const bool moveUp = (drawing.botRight.y() > dp.areaBotRight.y());

	QStringList texts;
	QStringList toolTips;

	const auto addLink = [&](const QString & symbol, const QString & link, const QString & toolTip) {
		texts << QString("<a href=\"%1\" style=\"color:black;text-decoration:none\">%2</a>").arg(link, symbol);
		toolTips << toolTip;
	};

	if (showMax) addLink("&#x25a2;", DrawLinks::Maximize, "Scale to maximum");

	// codes like "&searr;" are not supported
	if (moveRight && moveDown) addLink("&#8600;", DrawLinks::MoveRightDown, "Move right&down");
	if (moveLeft  && moveDown) addLink("&#8601;", DrawLinks::MoveLeftDown,  "Move left&down");
	if (moveRight && moveUp)   addLink("&#8599;", DrawLinks::MoveRightUp,   "Move right&up");
	if (moveLeft  && moveUp)   addLink("&#8598;", DrawLinks::MoveLeftUp,    "Move left&up");

	if (moveDown)  addLink("&darr;", DrawLinks::MoveDown,  "Move down");
	if (moveLeft)  addLink("&larr;", DrawLinks::MoveLeft,  "Move left");
	if (moveRight) addLink("&rarr;", DrawLinks::MoveRight, "Move right");
	if (moveUp)    addLink("&uarr;", DrawLinks::MoveUp,    "Move up");

	lblDrawActions->setText(texts.join("&nbsp;"));
	lblDrawActions->setToolTip(toolTips.join(" | "));
	lblDrawActions->adjustSize();

	// enforce label within visible area (after label is filled with text!)
	const QPoint labelDist(10, 10);
	const auto labelGeom = lblDrawActions->geometry();
	const auto labelMinPos = labelDist;
	const auto labelMaxPos = dp.areaWidthHeight - QPoint(labelGeom.width(), labelGeom.height()) - labelDist;
	labelPos.setX(qMin(qMax(labelMinPos.x(), labelPos.x()), labelMaxPos.x()));
	labelPos.setY(qMin(qMax(labelMinPos.y(), labelPos.y()), labelMaxPos.y()));
	lblDrawActions->setGeometry(labelPos.x(), labelPos.y(), lblDrawActions->geometry().width(), lblDrawActions->geometry().height());
	lblDrawActions->setVisible(true);
}

void LSystemUi::markDrawing()
{
	auto markedDrawing = drawArea->getMarkedDrawingResult();

	drawAreaMenu->setDrawingActionsVisible(markedDrawing.has_value());

	if (markedDrawing.has_value() && !ui->chkAutoPaint->isChecked()) {
		setConfigSet(markedDrawing->config);
		ui->playerControl->setPlaying(false);
		ui->playerControl->setMaxValueAndValue(markedDrawing->segmentsCount, markedDrawing->animStep);
	}
}

void LSystemUi::showErrorInUi(const QString & errString)
{
	showMessage(errString, MsgType::Error);
}

void LSystemUi::copyToClipboardMarked()
{
	bool transparent;
	if (transparencyOpt != TransparencyOpt::Ask) {
		transparent = (transparencyOpt == TransparencyOpt::Transparency);
	} else {
		bool doNotAskAnymore = false;
		// QMessageBox will take ownership (will delete the checkbox)
		QCheckBox * chkTransparency = new QCheckBox("Don't ask anymore until restart of lsystem");
		connect(chkTransparency, &QCheckBox::stateChanged, [&doNotAskAnymore](int state) { doNotAskAnymore = static_cast<bool>(state); });

		QMessageBox msgBox(QMessageBox::Icon::Question, "Transparency", "Do you want to export the drawing with transparent background (will not work in all programs)?",
						QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, parentWidget());
		msgBox.setDefaultButton(QMessageBox::Yes);
		msgBox.setCheckBox(chkTransparency);
		QMessageBox::StandardButton res = static_cast<QMessageBox::StandardButton>(msgBox.exec());
		if (res == QMessageBox::Cancel) return;
		transparent = (res == QMessageBox::Yes);
		if (doNotAskAnymore) {
			transparencyOpt = (transparent ? TransparencyOpt::Transparency : TransparencyOpt::NoTransparency);
		}
	}

	drawArea->copyToClipboardMarked(transparent);
}

void LSystemUi::processSimulatorResult(const common::ExecResult & execResult, const QSharedPointer<lsystem::common::MetaData> & metaData)
{
	QSharedPointer<DrawMetaData> drawMetaData = qSharedPointerDynamicCast<DrawMetaData>(metaData);

	if (execResult.resultKind == common::ExecResult::ExecResultKind::InvalidConfig) {
		resultAvailable = false;
		endInvokeExec();
		if (drawMetaData->clearAll) drawArea->clear();
		return;
	}
	resultAvailable = true;
	drawMetaData->resultOk = (execResult.resultKind == ExecResult::ExecResultKind::Ok);
	emit startDraw(execResult, metaData);
	// drawDone causes finally endInvokeExec

	if (symbolsDialog && symbolsDialog->isVisible()) emit simulatorExecActionStr();
}

void LSystemUi::processActionStr(const QString & actionStr)
{
	openSymbolsDialog();

	symbolsDialog->setContent(actionStr);
}

void LSystemUi::drawDone(const lsystem::ui::Drawing & drawing, const QSharedPointer<MetaData> & metaData)
{
	QSharedPointer<DrawMetaData> drawMetaData = qSharedPointerDynamicCast<DrawMetaData>(metaData);
	if (drawMetaData.isNull()) {
		showMessage("got wrong meta data", MsgType::Error);
		return;
	}

	drawArea->draw(drawing, drawMetaData->offset, drawMetaData->clearAll, drawMetaData->clearLast);

	ui->playerControl->setMaxValueAndValue(drawing.segments.size(), drawing.segments.size());

	if (drawMetaData->resultOk) {
		const QString msgPainted = printStr("Painted %1 segments, size is %2 px, <a href=\"%3\">show symbols</a>",
				drawing.segments.size(), drawing.size(), Links::ShowSymbols);

		showMessage(msgPainted, MsgType::Info);
	}

	endInvokeExec();

	// Needed if drawing was caused by a DrawLinks::Maximize operation.
	ui->playerControl->unstashState();
}

void LSystemUi::configLiveEdit()
{
	if (!ui->chkAutoPaint->isChecked() || disableConfigLiveEdit)
		return;

	ConfigSet configSet = getConfigSet(true);
	if (!configSet.valid) return;

	execConfig(configSet);
}

void LSystemUi::execConfig(const ConfigSet& configSet)
{
	auto optOffset = drawArea->getLastOffset();

	QSharedPointer<DrawMetaData> execMeta(new DrawMetaData);
	if (optOffset.has_value()) {
		execMeta->offset = optOffset.value();
	} else {
		execMeta->offset = QPoint{drawArea->width() / 2, drawArea->height() / 2};
	}

	execMeta->clearAll = true;
	execMeta->config = configSet;
	getAdditionalOptions(execMeta);

	invokeExec(execMeta);
}

void LSystemUi::focusAngleEdit(FocusableLineEdit * lineEdit)
{
	if (!ui->chkAutoPaint->isChecked()) return;

	// special case: forumla in right edit
	if (lineEdit == ui->txtRight) {
		const AngleEvaluator::Result res = angleEvaluator.evaluate(ui->txtLeft->text(), ui->txtRight->text());
		if (res.isFormula) {
			lineEdit->clearFocus();
			showRightAngleDialog();
			return;
		}
	}


	const QPoint lineditTopLeft = ui->wdgConfig->mapTo(ui->centralwidget, lineEdit->geometry().topLeft());
	const int x = lineditTopLeft.x() - 2;
	const int y = lineditTopLeft.y() - quickAngle->geometry().height() / 2 + lineEdit->geometry().height() / 2;

	// data
	quickAngle->setLineEdit(nullptr);
	quickAngle->setValueRestriction(lineEdit->valueRestriction()); // positive/negative
	quickAngle->setValue(lineEdit->text().toDouble());

	// control
	quickAngle->setLineEdit(lineEdit);
	quickAngle->placeAt(x, y);
	quickAngle->setVisible(true);
	quickAngle->setFocus();

	lineEdit->clearFocus();
	lineEdit->setSelection(0, 0);

	showMessage(SelAngleHintText, MsgType::Info);
}

void LSystemUi::focusLinearEdit(FocusableLineEdit * lineEdit)
{
	if (!ui->chkAutoPaint->isChecked()) return;

	const QPoint lineditTopLeft = lineEdit->parentWidget()->mapTo(ui->centralwidget, lineEdit->geometry().topLeft());

	const int x = lineditTopLeft.x() - 2;
	const int y = lineditTopLeft.y() - quickLinear->geometry().height() / 2 + lineEdit->geometry().height() / 2;

	double bigStep;
	double smallStep;
	double minValue;
	double maxValue;
	double extFactor = 0;
	double fineStepSize = 0;
	if (lineEdit == ui->txtStep) {
		smallStep = 1;
		bigStep = 3;
		minValue = 1;
		maxValue = qMax(static_cast<double>(qCeil(lastValidConfigSet.stepSize)), 30.);
		extFactor = 2;
		fineStepSize = 0.1;
	} else if (lineEdit == ui->txtIter) {
		smallStep = 1;
		bigStep = 3;
		minValue = 1;
		maxValue = qMax(lastValidConfigSet.numIter, 20u);
		extFactor = 1.5;
	} else if (lineEdit == ui->txtScaleDown) {
		smallStep = 0.05;
		bigStep = 0.15;
		minValue = 0;
		maxValue = 1;
	} else if (lineEdit == ui->txtLastIterOpacity || lineEdit == ui->txtOpacity) {
		smallStep = 1;
		bigStep = 10;
		minValue = 0;
		maxValue = 100;
	} else if (lineEdit == ui->txtThickness) {
		smallStep = 0.25;
		bigStep = 1;
		minValue = 0.5;
		maxValue = 5;
		extFactor = 1.5;
	} else if (lineEdit == ui->txtLatency) {
		smallStep = 0.01;
		bigStep = 0.1;
		minValue = 0.01;
		maxValue = 1;
	} else {
		// should not happen
		return;
	}

	// meta data (no line edit pointer yet)
	quickLinear->setLineEdit(nullptr);
	quickLinear->setSmallBigStep(smallStep, bigStep);
	quickLinear->setMinMaxValue(minValue, maxValue);
	quickLinear->setExtensionFactor(extFactor);
	quickLinear->setFineStepSize(fineStepSize);

	// value date (value changes like rounding should be applied)
	quickLinear->setLineEdit(lineEdit);
	quickLinear->setValue(lineEdit->text().toDouble());

	// control
	quickLinear->placeAt(x, y);
	quickLinear->setVisible(true);
	quickLinear->setFocus();

	lineEdit->clearFocus();
	lineEdit->setSelection(0, 0);

	showMessage(printStr(SelLinearHintText, bigStep), MsgType::Info);
}

void LSystemUi::unfocusAngleEdit()
{
	quickAngle->setVisible(false);
	quickAngle->setLineEdit(nullptr);
}

void LSystemUi::unfocusLinearEdit()
{
	quickLinear->setVisible(false);
	quickLinear->setLineEdit(nullptr);
}

void LSystemUi::checkAutoPaintChanged(int state)
{
	if (state == Qt::CheckState::Checked) {
		configLiveEdit();
	}
}

void LSystemUi::latencyChanged()
{
	bool ok;
	const auto latency = ui->txtLatency->text().toDouble(&ok);
	if (!ok || latency <= 0) {
		showVarError("latency", "must be a positive number");
	}
	std::chrono::milliseconds latencyMs{qRound(latency * 1000)};
	emit setAnimateLatency(latencyMs);
}

void LSystemUi::copyStatus()
{
	QClipboard * clipboard = QGuiApplication::clipboard();
	clipboard->setText(ui->lblStatus->text());
}

void LSystemUi::on_lblStatus_linkActivated(const QString & link)
{
	if (link == Links::NextIterations) {
		QSharedPointer<DrawMetaData> execMeta(new DrawMetaData);
		const auto optOffset = drawArea->getLastOffset();
		if (!optOffset.has_value()) return;
		execMeta->offset = optOffset.value();
		execMeta->clearLast = true;
		auto config = drawArea->getCurrentDrawing()->config;
		if (config.overrideStackSize) {
			config.overrideStackSize = *config.overrideStackSize * 2;
		} else {
			config.overrideStackSize = configFileStore.getSettings().maxStackSize * 2;
		}
		execMeta->config = config;
		invokeExec(execMeta);
	} else if (link == Links::ShowSymbols) {
		emit simulatorExecActionStr();
	} else if (link == Links::EditSettings) {
		showSettings();
	} else {
		QMessageBox::critical(this, "Unknown link action", QString("Link action '%1' was not found").arg(link));
	}
}

void LSystemUi::on_lblStatus_mousePressed(QMouseEvent * event)
{
	if (event->button() == Qt::RightButton) {
		statusMenu->menu.exec(event->globalPosition().toPoint());
	}
}

void LSystemUi::on_cmdAbout_clicked()
{
	AboutDialog dia(this);
	dia.setModal(true);
	dia.exec();
}

void LSystemUi::on_cmdSettings_clicked()
{
	showSettings();
}

void LSystemUi::toggleHelperFrame(QPushButton * button, QWidget * frame)
{
	if (!frame->isVisible()) {
		const auto butBottomLeft = ui->wdgConfig->mapTo(ui->centralwidget, button->geometry().bottomLeft());
		const auto & frameGeom = frame->geometry();
		frame->setGeometry(butBottomLeft.x() - frameGeom.width(), butBottomLeft.y() - frameGeom.height(), frameGeom.width(), frameGeom.height());
		frame->setVisible(true);
	} else {
		frame->setVisible(false);
	}

}

void LSystemUi::on_cmdAdditionalOptions_clicked()
{
	toggleHelperFrame(ui->cmdAdditionalOptions, ui->frmAdditionalOptions);
}

void LSystemUi::on_cmdPlayer_clicked()
{
	toggleHelperFrame(ui->cmdPlayer, ui->frmPlayer);
}

void LSystemUi::on_chkShowLastIter_stateChanged()
{
	configLiveEdit();

	const bool checked = ui->chkShowLastIter->isChecked();
	ui->txtLastIterOpacity->setEnabled(checked);
	ui->lblOpacity->setEnabled(checked);
}

void LSystemUi::on_cmdCloseAdditionalSettings_clicked()
{
	ui->frmAdditionalOptions->setVisible(false);
}

void LSystemUi::on_cmdClosePlayer_clicked()
{
	ui->frmPlayer->setVisible(false);
}

void LSystemUi::on_chkAntiAliasing_stateChanged()
{
	configLiveEdit();
}

void LSystemUi::processDrawAction(const QString & link)
{
	if (!highlightedDrawing.has_value()) return;

	ui->playerControl->stashState();

	const auto & dp = drawPlacement;
	const auto & drawing = highlightedDrawing.value();

	int xOff = drawing.offset.x();
	int yOff = drawing.offset.y();

	if (link == DrawLinks::Maximize) {

		const auto newStepSize = drawing.config.stepSize * dp.fct;

		// calculate new y position

		const auto predTopY = yOff - (yOff - drawing.topLeft.y()) * dp.fct;
		const auto predBottomY = predTopY + dp.drawingSize.y() * dp.fct;

		if (predTopY < dp.areaTopLeft.y()) {
			yOff += dp.areaTopLeft.y() - predTopY;
		} else if (predBottomY > dp.areaBotRight.y()) {
			yOff -= (predBottomY - dp.areaBotRight.y());
		}

		// calculate new x position

		const auto predLeftX = xOff - (xOff - drawing.topLeft.x()) * dp.fct;
		const auto predRightX = predLeftX + dp.drawingSize.x() * dp.fct;

		if (predLeftX < dp.areaTopLeft.x()) {
			xOff += dp.areaTopLeft.x() - predLeftX;
		} else if (predRightX > dp.areaBotRight.x()) {
			xOff -= (predRightX - dp.areaBotRight.x());
		}

		if (drawing.config.stepSize != newStepSize) {

			ConfigSet configSet = drawing.config;
			configSet.stepSize = newStepSize;

			if (ui->chkAutoPaint->isChecked()) {
				disableConfigLiveEdit = true;
				if (quickLinear->getLineEdit() == static_cast<QLineEdit *>(ui->txtStep)) {
					quickLinear->setValue(newStepSize);
				}
				ui->txtStep->setText(util::formatFixed(newStepSize, 2));
				disableConfigLiveEdit = false;
				lastValidConfigSet = configSet;
			}

			QSharedPointer<DrawMetaData> execMeta(new DrawMetaData);
			execMeta->offset = QPoint{xOff, yOff};
			if (ui->chkAutoPaint->isChecked()) {
				execMeta->clearAll = true;
			} else {
				execMeta->clearLast = true;
			}
			execMeta->config = configSet;

			getAdditionalOptions(execMeta);
			invokeExec(execMeta);
		}

	} else {
		bool moveLeft = false;
		bool moveRight = false;
		bool moveDown = false;
		bool moveUp = false;

		if (link == DrawLinks::MoveDown) {
			moveDown = true;
		} else if (link == DrawLinks::MoveUp) {
			moveUp = true;
		} else if (link == DrawLinks::MoveLeft) {
			moveLeft = true;
		} else if (link == DrawLinks::MoveRight) {
			moveRight = true;
		} else if (link == DrawLinks::MoveRightDown) {
			moveDown = true;
			moveRight = true;
		} else if (link == DrawLinks::MoveLeftDown) {
			moveDown = true;
			moveLeft = true;
		} else if (link == DrawLinks::MoveRightUp) {
			moveUp = true;
			moveRight = true;
		} else if (link == DrawLinks::MoveLeftUp) {
			moveUp = true;
			moveLeft = true;
		}

		if (moveDown)  yOff += dp.areaTopLeft .y() - drawing.topLeft .y();
		if (moveUp)    yOff += dp.areaBotRight.y() - drawing.botRight.y();
		if (moveLeft)  xOff += dp.areaBotRight.x() - drawing.botRight.x();
		if (moveRight) xOff += dp.areaTopLeft.x() - drawing.topLeft.x();

		drawArea->translateHighlighted(QPoint(xOff, yOff));
		ui->playerControl->unstashState();
	}
}

void LSystemUi::showRightAngleDialog()
{
	QString strRightAngle = ui->txtRight->text();
	AngleFormulaDialog dia(this, ui->txtLeft->text(), strRightAngle);
	dia.setModal(true);
	dia.exec();

	if (!strRightAngle.isNull()) {
		ui->txtRight->setText(strRightAngle);
	}
}

void LSystemUi::playPauseChanged(bool playing)
{
	if (playing) {
		if (!resultAvailable) {
			showErrorInUi("no drawing available");
			ui->playerControl->setPlaying(false);
			return;
		}

		latencyChanged();
		emit startAnimateCurrentDrawing();
	} else {
		emit stopAnimate();
	}
}

AnimatorResult LSystemUi::newAnimationStep(int step, bool relativeStep)
{
	// forward to DrawArea (same thread)
	const auto res = drawArea->newAnimationStep(step, relativeStep);

	// Update UI control
	ui->playerControl->setValue(res.step);
	const bool animationDone = res.nextStepResult == AnimatorResult::NextStepResult::Stopped;
	if (animationDone) ui->playerControl->setPlaying(false);

	return res;
}

void LSystemUi::playerValueChanged(int value) { emit goToAnimationStep(value); }

void LSystemUi::on_cmdRightFormula_clicked()
{
	showRightAngleDialog();
}


// -------------------- DrawAreaMenu --------------------

LSystemUi::DrawAreaMenu::DrawAreaMenu(LSystemUi * parent)
	: menu(parent)
{
	drawingActions
		<< menu.addAction("Delete drawing", Qt::Key_Delete, &*parent->drawArea, &DrawArea::deleteMarked)
		<< menu.addAction("Copy drawing", Qt::CTRL | Qt::Key_C, parent, &LSystemUi::copyToClipboardMarked)
		<< menu.addAction("Send to front", &*parent->drawArea, &DrawArea::sendToFrontMarked)
		<< menu.addAction("Send to back", &*parent->drawArea, &DrawArea::sendToBackMarked)
		<< menu.addAction("Show config", Qt::CTRL | Qt::SHIFT | Qt::Key_C, parent, &LSystemUi::showMarkedConfig)
		<< menu.addSeparator();

	setDrawingActionsVisible(false);

	undoAction = menu.addAction("Undo", Qt::CTRL | Qt::Key_Z, &*parent->drawArea, &DrawArea::restoreLastImage);
	undoAction->setEnabled(false);
	redoAction = menu.addAction("Redo", Qt::CTRL | Qt::Key_Y, &*parent->drawArea, &DrawArea::restoreLastImage);
	redoAction->setEnabled(false);
	menu.addSeparator();

	autoClearToggle = menu.addAction("Auto clear");
	autoClearToggle->setCheckable(true);
	menu.addSeparator();

	menu.addAction("Clear all", Qt::CTRL | Qt::Key_Delete, parent, &LSystemUi::clearAll);
	menu.addAction("Set Bg-Color", Qt::CTRL | Qt::Key_B, parent, &LSystemUi::setBgColor);
	menu.addSeparator();

	menu.addAction("Copy canvas", &*parent->drawArea, &DrawArea::copyToClipboardFull);
	menu.addSeparator();

	menu.addAction("Show symbols window", Qt::CTRL | Qt::SHIFT | Qt::Key_S, parent, &LSystemUi::showSymbols);

	// to get the shortcurts working
	parent->ui->menubar->addMenu(&menu);
	parent->ui->menubar->setNativeMenuBar(true);
}

void LSystemUi::DrawAreaMenu::setDrawingActionsVisible(bool visible)
{
	// shortcuts become enabled/disabled with making the actions (un)visible
	for (QAction * action : std::as_const(drawingActions)) {
		action->setVisible(visible);
	}
}

// -------------------- StatusMenu --------------------

LSystemUi::StatusMenu::StatusMenu(LSystemUi * parent)
	: menu(parent)
{
	menu.addAction("Copy to clipboard", &*parent, &LSystemUi::copyStatus);
}

// -------------------- DrawMetaData --------------------

QString LSystemUi::DrawMetaData::toString() const
{
	return printStr("[%1,DrawMetaData(offset: %2, clearAll: %3, clearLast: %4, resultOk: %5)]",
			MetaData::toString(), offset, clearAll, clearLast, resultOk);
}

// ------------------------------------------------------



