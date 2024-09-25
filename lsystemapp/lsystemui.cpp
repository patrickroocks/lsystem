#include "lsystemui.h"
#include "ui_lsystemui.h"

#include <aboutdialog.h>
#include <angleformuladialog.h>
#include <settingsdialog.h>
#include <util/containerutils.h>
#include <util/print.h>
#include <util/tableitemdelegate.h>
#include <version.h>

#include <QClipboard>
#include <QColorDialog>
#include <QDebug>
#include <QGuiApplication>
#include <QInputDialog>
#include <QMessageBox>

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

QString generateBgColorStyle(const QColor & col)
{
	return QString("background-color: rgb(") + QString::number(col.red()) + "," + QString::number(col.green()) + ","
		   + QString::number(col.blue()) + ");\nborder: 1px solid black;";
}

} // namespace

LSystemUi::LSystemUi(QWidget * parent)
	: QMainWindow(parent)
	, ui(new Ui::LSystemUi)
	, defModel(this)
	, configList(this)
{
	ui->setupUi(this);

	setWindowTitle(util::printStr("lsystem %1 - An interactive simulator for Lindenmayer systems", Version));

	setupServices();
	setupConfigList();
	setupMainControls();
	setupHelperControls();
	setupStatusAndTimers();
	setupInteractiveControls();
	setupDrawArea();
}

void LSystemUi::setupServices()
{
	// setup the background service for generating/animating the fractals
	simulator.moveToThread(&simulatorThread);
	connect(this, &LSystemUi::simulatorExec, &simulator, &Simulator::exec);
	connect(&simulator, &Simulator::segmentsReceived, this, &LSystemUi::processSimulatorSegments);
	connect(&simulator, &Simulator::actionStrReceived, this, &LSystemUi::processActionStr);
	connect(&simulator, &Simulator::errorReceived, this, &LSystemUi::showErrorInUi);
	simulatorThread.start();

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
}

void LSystemUi::setupConfigList()
{
	connect(&configList, &ConfigList::configMapUpdated, &configFileStore, &ConfigFileStore::newConfigMap);
	connect(&configFileStore, &ConfigFileStore::loadedPreAndUserConfigs, &configList, &ConfigList::newPreAndUserConfigs);
	connect(&configFileStore, &ConfigFileStore::showError, this, &LSystemUi::showErrorInUi);
	connect(&configFileStore, &ConfigFileStore::newStackSize, &simulator, &Simulator::setMaxStackSize);

	ui->lstConfigs->setModel(&configList);
	configFileStore.loadConfig();
	loadConfigByLstIndex(configList.index(0, 0));

	connect(ui->cmdDeleteConfig, &QPushButton::clicked, this, &LSystemUi::onCmdDeleteConfigClicked);
	connect(ui->lstConfigs, &QListView::clicked, this, &LSystemUi::loadConfigByLstIndex);
}

void LSystemUi::setupMainControls()
{
	// connections/initialization for the main controls

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

	connect(ui->tblDefinitions->selectionModel(), &QItemSelectionModel::selectionChanged, &defModel, &DefinitionModel::selectionChanged);
	connect(&defModel, &DefinitionModel::deselect, [this]() { ui->tblDefinitions->setCurrentIndex(QModelIndex()); });
	connect(&defModel, &DefinitionModel::getSelection, [this]() { return ui->tblDefinitions->currentIndex(); });
	connect(&defModel, &DefinitionModel::newStartSymbol, ui->lblStartSymbol, &QLabel::setText);
	connect(&defModel, &DefinitionModel::showError, this, &LSystemUi::showErrorInUi);
	connect(&defModel, &DefinitionModel::edited, this, &LSystemUi::configLiveEdit);

	connect(ui->cmdAdd, &QPushButton::clicked, &defModel, &DefinitionModel::add);
	connect(ui->cmdRemove, &QPushButton::clicked, &defModel, &DefinitionModel::remove);
	connect(ui->cmdRightFormula, &QPushButton::clicked, this, &LSystemUi::showRightAngleDialog);
}

void LSystemUi::setupHelperControls()
{
	// Color gradient
	connect(ui->lblGradientStart, &ClickableLabel::mousePressed, this, &LSystemUi::onLblGradientStartMousePressed);
	connect(ui->lblGradientEnd, &ClickableLabel::mousePressed, this, &LSystemUi::onLblGradientEndMousePressed);
	connect(ui->chkColorGradient, &QCheckBox::stateChanged, this, &LSystemUi::configLiveEdit);
	ui->wdgGradientPreview->setColorGradient(&colorGradient);
	updateGradientStyle();

	// Player control (UI control for segment animator)
	connect(ui->playerControl, &PlayerControl::playPauseChanged, this, &LSystemUi::playPauseChanged);
	connect(ui->playerControl, &PlayerControl::playerValueChanged, this, &LSystemUi::playerValueChanged);

	// Show/hide Popups
	connect(ui->cmdAdditionalOptions, &QPushButton::clicked, this, &LSystemUi::onCmdAdditionalOptionsClicked);
	connect(ui->cmdCloseAdditionalOptions, &QPushButton::clicked, this, &LSystemUi::onCmdCloseAdditionalOptionsClicked);
	connect(ui->cmdPlayer, &QPushButton::clicked, this, &LSystemUi::onCmdPlayerClicked);
	connect(ui->cmdClosePlayer, &QPushButton::clicked, this, &LSystemUi::onCmdClosePlayerClicked);

	// CheckBoxes
	connect(ui->chkShowSliders, &QCheckBox::stateChanged, this, &LSystemUi::onChkShowSlidersChanged);
	connect(ui->chkShowLastIter, &QCheckBox::stateChanged, this, &LSystemUi::onChkShowLastIterStateChanged);
	connect(ui->chkAntiAliasing, &QCheckBox::stateChanged, this, &LSystemUi::configLiveEdit);

	// Defaults
	connect(ui->cmdResetDefaultOptions, &QPushButton::clicked, this, &LSystemUi::onCmdResetDefaultOptionsClicked);

	// Helper popups are invisible by default
	ui->frmAdditionalOptions->setVisible(false);
	ui->frmPlayer->setVisible(false);

	// Buttons not related to config
	connect(ui->cmdSettings, &QPushButton::clicked, this, &LSystemUi::showSettings);
	connect(ui->cmdAbout, &QPushButton::clicked, this, &LSystemUi::onCmdAboutClicked);
}

void LSystemUi::setupStatusAndTimers()
{
	statusMenu.reset(new StatusMenu(this));
	resetStatus();

	messageDecayTimer.setInterval(StatusIntervalMs);
	connect(&messageDecayTimer, &QTimer::timeout, this, &LSystemUi::resetStatus);

	exec.pendingTimer.setInterval(ExecPendingIntervalMs);
	connect(&exec.pendingTimer, &QTimer::timeout, this, &LSystemUi::invokeExecPending);

	connect(ui->lblStatus, &ClickableLabel::linkActivated, this, &LSystemUi::onLblStatusLinkActivated);
	connect(ui->lblStatus, &ClickableLabel::mousePressed, this, &LSystemUi::onLblStatusMousePressed);
}

void LSystemUi::setupInteractiveControls()
{
	ui->txtStartAngle->setValueRestriction(ValueRestriction::Numbers);
	ui->txtLeft->setValueRestriction(ValueRestriction::PositiveNumbers);
	ui->txtRight->setValueRestriction(ValueRestriction::NegativeNumbers);

	quickAngle.reset(new QuickAngle(ui->wdgEntire));
	quickAngle->setVisible(false);
	connect(quickAngle.data(), &QuickAngle::focusOut, this, &LSystemUi::unfocusAngleEdit);

	quickLinear.reset(new QuickLinear(ui->wdgEntire));
	quickLinear->setVisible(false);
	connect(quickLinear.data(), &QuickLinear::focusOut, this, &LSystemUi::unfocusLinearEdit);

	const std::vector<FocusableLineEdit *> configEditsLinear
		= {ui->txtIter, ui->txtStep, ui->txtScaleDown, ui->txtLastIterOpacity, ui->txtThickness, ui->txtOpacity};

	const std::vector<FocusableLineEdit *> allLinearEdits = util::concatenateVectors(configEditsLinear, {ui->txtLatency});

	const std::vector<FocusableLineEdit *> configEditsAngle = {ui->txtStartAngle, ui->txtLeft, ui->txtRight};

	const std::vector<FocusableLineEdit *> allEdits = util::concatenateVectors(allLinearEdits, configEditsAngle);
	const std::vector<FocusableLineEdit *> allConfigEdits = util::concatenateVectors(configEditsLinear, configEditsAngle);

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
}

bool LSystemUi::eventFilter(QObject * obj, QEvent * event)
{
	if (obj == ui->lstLayers && event->type() == QEvent::KeyPress) {
		QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
		if (keyEvent->key() == Qt::Key_Delete) {
			onCmdDeleteLayerClicked();
		}
	}
	return QObject::eventFilter(obj, event);
}

void LSystemUi::setupDrawArea()
{
	drawArea = ui->wdgDraw;
	drawArea->setMouseTracking(true); // for mouse move event
	connect(drawArea, &DrawArea::mouseClick, this, &LSystemUi::drawAreaClick);
	connect(drawArea, &DrawArea::enableUndoRedo, this, &LSystemUi::enableUndoRedo);
	connect(drawArea, &DrawArea::markingChanged, this, &LSystemUi::markDrawing);
	connect(drawArea, &DrawArea::highlightChanged, this, &LSystemUi::highlightDrawing);

	// Label for move/maximize commands
	lblDrawActions.reset(new ClickableLabel(drawArea));
	lblDrawActions->setVisible(false);
	lblDrawActions->setMouseTracking(true);
	lblDrawActions->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
	lblDrawActions->setStyleSheet("QLabel { background-color: white; border: 2px solid white;}");
	connect(lblDrawActions.data(), &ClickableLabel::linkActivated, this, &LSystemUi::processDrawAction);

	drawAreaMenu.reset(new DrawAreaMenu(this)); // needs drawArea

	ui->lstLayers->setModel(&drawArea->getDrawingCollection());

	auto * layersSelModel = ui->lstLayers->selectionModel();
	connect(layersSelModel, &QItemSelectionModel::selectionChanged, drawArea, &DrawArea::layerSelectionChanged);

	connect(ui->cmdDeleteLayer, &QPushButton::clicked, this, &LSystemUi::onCmdDeleteLayerClicked);

	// Install event filter
	ui->lstLayers->installEventFilter(this);
}

void LSystemUi::onCmdDeleteLayerClicked()
{
	if (ui->lstLayers->selectionModel()->selectedIndexes().empty()) return;
	drawArea->deleteIndex(ui->lstLayers->selectionModel()->selectedIndexes().first().row());
}

LSystemUi::~LSystemUi()
{
	quitAndWait({&simulatorThread, &segDrawerThread});
	delete ui;
}

void LSystemUi::invokeExec(const QSharedPointer<AllDrawData> & drawData)
{
	exec.scheduledPending = false;

	// prevent that a queue of executions blocks everything.
	if (exec.active) {
		// When overwriting pending Meta, ensure that no tasks are lost.
		if (exec.pendingData) {
			drawData->meta.execActionStr |= exec.pendingData->meta.execActionStr;
			drawData->meta.execSegments |= exec.pendingData->meta.execSegments;
		}
		exec.pendingData = drawData;
		return;
	} else {
		exec.pendingData = nullptr;
		exec.active = true;
		if (drawData->meta.execActionStr) exec.waitForExecTasks.insert(ExecKind::ActionStr);
		if (drawData->meta.execSegments) exec.waitForExecTasks.insert(ExecKind::Segments);
		emit simulatorExec(drawData);
	}
}

void LSystemUi::endInvokeExec(ExecKind execKind)
{
	exec.waitForExecTasks.remove(execKind);

	if (exec.waitForExecTasks.empty()) {
		exec.active = false;
		if (exec.pendingData) {
			exec.scheduledPending = true;
			// This restarts the timer, even it was started before.
			// Don't use singleShot, it would start multiple timers.
			exec.pendingTimer.start();
		}
	}
}

void LSystemUi::invokeExecPending()
{
	exec.pendingTimer.stop();
	if (exec.scheduledPending) {
		// pendingMeta is deleted in invokeExec, preserve shared pointer (passed by reference)
		auto copiedPendingMeta = exec.pendingData;
		invokeExec(copiedPendingMeta);
	}
}

void LSystemUi::startPaint(int x, int y)
{
	const ConfigSet configSet = getConfigSet(true);
	if (!configSet.valid) return;

	QSharedPointer<AllDrawData> drawData(new AllDrawData);
	drawData->uiDrawData.offset = QPoint{x, y};
	getAdditionalOptionsForSegmentsMeta(drawData->meta);
	drawData->config = configSet;

	invokeExec(drawData);
}

void LSystemUi::getAdditionalOptionsForSegmentsMeta(MetaData & execMeta)
{
	execMeta.execSegments = true;
	execMeta.execActionStr = symbolsVisible();

	execMeta.showLastIter = ui->chkShowLastIter->isChecked();

	if (ui->chkColorGradient->isChecked()) execMeta.colorGradient = colorGradient;
	else execMeta.colorGradient = {};

	bool ok;
	const double lastIterOpacityPercent = ui->txtLastIterOpacity->text().toDouble(&ok);
	if (ok) execMeta.lastIterOpacy = lastIterOpacityPercent / 100.;

	const double opacityPercent = ui->txtOpacity->text().toDouble(&ok);
	if (ok) execMeta.opacity = opacityPercent / 100.;

	const double thickness = ui->txtThickness->text().toDouble(&ok);
	if (ok) execMeta.thickness = thickness;

	execMeta.antiAliasing = ui->chkAntiAliasing->isChecked();
}

void LSystemUi::showSymbols()
{
	// Open dialog
	if (!symbolsDialog) {
		// Ensure that the symbols window is in foreground of us, but the main window can be still used.
		symbolsDialog.reset(new SymbolsDialog(this));
		symbolsDialog->setWindowModality(Qt::WindowModality::NonModal);
		symbolsDialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
	}

	if (!symbolsDialog->isVisible()) {
		symbolsDialog->show();
	}

	if (resultAvailable) {

		QSharedPointer<AllDrawData> drawData = QSharedPointer<AllDrawData>::create();
		drawData->config = getConfigSet(true);
		drawData->meta.execActionStr = true;
		invokeExec(drawData);
	}
}

bool LSystemUi::symbolsVisible() const { return symbolsDialog && symbolsDialog->isVisible(); }

void LSystemUi::setBgColor()
{
	const QColor col = QColorDialog::getColor(drawArea->getBgColor(), this);
	if (col.isValid()) drawArea->setBgColor(col);
}

LSystemUi::DrawPlacement LSystemUi::getDrawPlacement() const
{
	LSystemUi::DrawPlacement rv;

	const auto drawing = highlightedDrawing.value();
	const auto & drawGeom = drawArea->geometry();
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

void LSystemUi::showVarError(const QString & errorVar, const QString & extraInfo)
{
	showErrorInUi(QString("Invalid value for variable '%1'%2").arg(errorVar, extraInfo.isEmpty() ? "" : (" (" + extraInfo + ")")));
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

	configSet.name = curConfigName;
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

void LSystemUi::showConfigSet(const ConfigSet & configSet)
{
	disableConfigLiveEdit = true;
	curConfigName = configSet.name;
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
}
void LSystemUi::setConfigSet(const ConfigSet & configSet)
{
	showConfigSet(configSet);

	// still initialization phase => do not draw
	if (!drawArea) return;

	configLiveEdit();
}

void LSystemUi::onCmdStoreConfigClicked()
{
	ConfigSet cfg = getConfigSet(false);
	if (!cfg.valid) return;

	ConfigNameKind currentConfig = configList.getConfigNameKindByIndex(ui->lstConfigs->currentIndex());
	bool ok;
	QString configName
		= QInputDialog::getText(this, "Config name", "Enter a name for the config:", QLineEdit::Normal, currentConfig.configName, &ok);

	if (!ok) return;

	curConfigName = configName;
	cfg.name = configName;
	configList.storeConfig(cfg);
	configLiveEdit();
}

void LSystemUi::drawAreaClick(int x, int y, Qt::MouseButton button, bool drawingMarked)
{
	ui->tblDefinitions->setFocus();						// removes focus from text fields
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
		showConfigSet(config);
	} else {
		showMessage("Config could not be loaded", MsgType::Error);
	}
}

void LSystemUi::onCmdDeleteConfigClicked()
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
	drawAreaMenu->undoAction->setEnabled(undoOrRedo);
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
	const auto & dp = drawPlacement;

	// first guess label pos
	const auto & drawing = drawResult.value();
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
	if (moveLeft && moveDown) addLink("&#8601;", DrawLinks::MoveLeftDown, "Move left&down");
	if (moveRight && moveUp) addLink("&#8599;", DrawLinks::MoveRightUp, "Move right&up");
	if (moveLeft && moveUp) addLink("&#8598;", DrawLinks::MoveLeftUp, "Move left&up");

	if (moveDown) addLink("&darr;", DrawLinks::MoveDown, "Move down");
	if (moveLeft) addLink("&larr;", DrawLinks::MoveLeft, "Move left");
	if (moveRight) addLink("&rarr;", DrawLinks::MoveRight, "Move right");
	if (moveUp) addLink("&uarr;", DrawLinks::MoveUp, "Move up");

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
	const auto markedDrawing = drawArea->getMarkedDrawingResult();

	drawAreaMenu->setDrawingActionsVisible(markedDrawing.has_value());

	if (markedDrawing.has_value()) {
		showConfigSet(markedDrawing->config);
		ui->playerControl->setPlaying(false);
		ui->playerControl->setMaxValueAndValue(markedDrawing->segmentsCount, markedDrawing->animStep);
	}

	// mark in layers list:
	drawArea->setIgnoreSelectionChange(true);
	ui->lstLayers->selectionModel()->clearSelection();
	if (markedDrawing.has_value()) {
		ui->lstLayers->selectionModel()->select(ui->lstLayers->model()->index(markedDrawing->listIndex, 0), QItemSelectionModel::Select);
	}
	drawArea->setIgnoreSelectionChange(false);
}

void LSystemUi::showErrorInUi(const QString & errString) { showMessage(errString, MsgType::Error); }

void LSystemUi::showWarningInUi(const QString & errString) { showMessage(errString, MsgType::Warning); }

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

		QMessageBox msgBox(QMessageBox::Icon::Question,
						   "Transparency",
						   "Do you want to export the drawing with transparent background (will not work in all programs)?",
						   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
						   parentWidget());
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

void LSystemUi::processSimulatorSegments(const common::ExecResult & execResult, const QSharedPointer<lsystem::common::AllDrawData> & data)
{
	endInvokeExec(ExecKind::Segments);

	if (execResult.resultKind == common::ExecResult::ExecResultKind::InvalidConfig) {
		resultAvailable = false;
		return;
	}

	resultAvailable = true;
	data->uiDrawData.resultOk = (execResult.resultKind == ExecResult::ExecResultKind::Ok);

	exec.waitForExecTasks.insert(ExecKind::Draw);
	emit startDraw(execResult, data); // drawDone also calls endInvokeExec
}

void LSystemUi::processActionStr(const QString & actionStr)
{
	endInvokeExec(ExecKind::ActionStr);

	if (symbolsVisible()) symbolsDialog->setContent(actionStr);
}

void LSystemUi::drawDone(const QSharedPointer<Drawing> & drawing, const QSharedPointer<AllDrawData> & data)
{
	endInvokeExec(ExecKind::Draw);

	lastDrawData = data;
	const auto & uiData = data->uiDrawData;
	drawing->offset = uiData.offset;
	drawing->num = uiData.drawingNumToEdit.value_or(0);

	drawArea->draw(drawing);

	ui->playerControl->setMaxValueAndValue(drawing->segments.size(), drawing->segments.size());

	if (uiData.resultOk) {
		const QString msgPainted = printStr("Painted %1 segments, size is %2 px, <a href=\"%3\">show symbols</a>",
											drawing->segments.size(),
											drawing->size(),
											Links::ShowSymbols);

		showMessage(msgPainted, MsgType::Info);
	}

	// Needed if drawing was caused by a DrawLinks::Maximize operation.
	ui->playerControl->unstashState();
}

void LSystemUi::configLiveEdit()
{
	if (disableConfigLiveEdit) return;

	ConfigSet configSet = getConfigSet(true);
	if (!configSet.valid) return;

	ui->playerControl->setPlaying(false);

	execConfigLive(configSet);
}

void LSystemUi::execConfigLive(const ConfigSet & configSet)
{
	const auto markedDrawing = drawArea->getMarkedDrawingResult();

	// No live edit, if no marked drawing
	if (!markedDrawing.has_value()) {
		showWarningInUi("no drawing selected");
		return;
	}

	QSharedPointer<AllDrawData> drawData = QSharedPointer<AllDrawData>::create();
	drawData->uiDrawData.drawingNumToEdit = markedDrawing->drawingNum;
	drawData->uiDrawData.offset = markedDrawing->offset;

	drawData->config = configSet;
	getAdditionalOptionsForSegmentsMeta(drawData->meta);

	invokeExec(drawData);
}

void LSystemUi::focusAngleEdit(FocusableLineEdit * lineEdit)
{
	if (!ui->chkShowSliders->isChecked()) return;

	// special case: forumla in right edit
	if (lineEdit == ui->txtRight) {
		const AngleEvaluator::Result res = angleEvaluator.evaluate(ui->txtLeft->text(), ui->txtRight->text());
		if (res.isFormula) {
			lineEdit->clearFocus();
			showRightAngleDialog();
			return;
		}
	}

	const QPoint lineditTopLeft = ui->wdgConfig->mapTo(ui->wdgEntire, lineEdit->geometry().topLeft());
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
	if (!ui->chkShowSliders->isChecked()) return;

	const QPoint lineditTopLeft = lineEdit->parentWidget()->mapTo(ui->wdgEntire, lineEdit->geometry().topLeft());

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

void LSystemUi::onChkShowSlidersChanged(int state)
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

void LSystemUi::onLblStatusLinkActivated(const QString & link)
{
	if (link == Links::NextIterations) {
		if (!lastDrawData) return;
		QSharedPointer<AllDrawData> data = QSharedPointer<AllDrawData>::create();
		*data = *lastDrawData;
		auto & config = data->config;

		if (config.overrideStackSize) {
			config.overrideStackSize = *config.overrideStackSize * 2;
		} else {
			config.overrideStackSize = configFileStore.getSettings().maxStackSize * 2;
		}
		data->config = config;
		getAdditionalOptionsForSegmentsMeta(data->meta);
		invokeExec(data);
	} else if (link == Links::ShowSymbols) {
		showSymbols();
	} else if (link == Links::EditSettings) {
		showSettings();
	} else {
		QMessageBox::critical(this, "Unknown link action", QString("Link action '%1' was not found").arg(link));
	}
}

void LSystemUi::onLblStatusMousePressed(QMouseEvent * event)
{
	if (event->button() == Qt::RightButton) {
		statusMenu->menu.exec(event->globalPosition().toPoint());
	}
}

void LSystemUi::updateGradientStyle(bool updateAfterClick)
{
	ui->lblGradientStart->setStyleSheet(generateBgColorStyle(colorGradient.startColor));
	ui->lblGradientEnd->setStyleSheet(generateBgColorStyle(colorGradient.endColor));
	ui->wdgGradientPreview->updateGradient();

	if (updateAfterClick) {
		if (!ui->chkColorGradient->isChecked()) ui->chkColorGradient->setCheckState(Qt::Checked); // raises liveEdit
		else configLiveEdit();
	}
}

void LSystemUi::onLblGradientStartMousePressed(QMouseEvent * event)
{
	if (event->button() != Qt::LeftButton) return;

	const QColor newColor = QColorDialog::getColor(colorGradient.startColor, this);
	if (newColor.isValid() && newColor != colorGradient.startColor) {
		colorGradient.startColor = newColor;
		updateGradientStyle(true);
	}
}

void LSystemUi::onLblGradientEndMousePressed(QMouseEvent * event)
{
	if (event->button() != Qt::LeftButton) return;

	const QColor newColor = QColorDialog::getColor(colorGradient.endColor, this);
	if (newColor.isValid() && newColor != colorGradient.endColor) {
		colorGradient.endColor = newColor;
		updateGradientStyle(true);
	}
}

void LSystemUi::onCmdResetDefaultOptionsClicked()
{
	disableConfigLiveEdit = true;

	ui->chkShowSliders->setCheckState(Qt::Checked);
	ui->chkAntiAliasing->setCheckState(Qt::Unchecked);
	ui->chkShowLastIter->setCheckState(Qt::Unchecked);
	ui->chkColorGradient->setCheckState(Qt::Unchecked);
	ui->txtLastIterOpacity->setText("100");
	ui->txtThickness->setText("1");
	ui->txtOpacity->setText("100");
	colorGradient.setDefault();
	updateGradientStyle();

	disableConfigLiveEdit = false;
	configLiveEdit();
}

void LSystemUi::onCmdAboutClicked()
{
	AboutDialog dia(this);
	dia.setModal(true);
	dia.exec();
}

void LSystemUi::toggleHelperFrame(QPushButton * button, QWidget * frame)
{
	static const QVector<QFrame *> allFrames{ui->frmAdditionalOptions, ui->frmPlayer};

	// close all other frames
	for (QFrame * frm : allFrames) {
		if (frm != frame) {
			frm->setVisible(false);
		}
	}

	// toggle visibility and place frame
	if (!frame->isVisible()) {
		const auto butBottomLeft = ui->wdgConfig->mapTo(ui->wdgEntire, button->geometry().bottomLeft());
		const auto & frameGeom = frame->geometry();
		frame->setGeometry(
			butBottomLeft.x() - frameGeom.width(), butBottomLeft.y() - frameGeom.height(), frameGeom.width(), frameGeom.height());
		frame->setVisible(true);
	} else {
		frame->setVisible(false);
	}
}

void LSystemUi::onCmdAdditionalOptionsClicked() { toggleHelperFrame(ui->cmdAdditionalOptions, ui->frmAdditionalOptions); }

void LSystemUi::onCmdPlayerClicked() { toggleHelperFrame(ui->cmdPlayer, ui->frmPlayer); }

void LSystemUi::onCmdCloseAdditionalOptionsClicked() { ui->frmAdditionalOptions->setVisible(false); }

void LSystemUi::onCmdClosePlayerClicked() { ui->frmPlayer->setVisible(false); }

void LSystemUi::onChkShowLastIterStateChanged()
{
	configLiveEdit();

	const bool checked = ui->chkShowLastIter->isChecked();
	ui->txtLastIterOpacity->setEnabled(checked);
	ui->lblLastIterOpacity->setEnabled(checked);
}


void LSystemUi::processDrawAction(const QString & link)
{
	if (!highlightedDrawing.has_value()) return;

	drawArea->markHighlighted();

	ui->playerControl->stashState();

	const auto & dp = drawPlacement;
	const auto & drawing = highlightedDrawing.value();

	int xOff = drawing.offset.x();
	int yOff = drawing.offset.y();

	const auto translate = [&]() {
		drawArea->translateHighlighted(QPoint(xOff, yOff));
		ui->playerControl->unstashState();
	};

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

			disableConfigLiveEdit = true;
			if (quickLinear->getLineEdit() == static_cast<QLineEdit *>(ui->txtStep)) {
				quickLinear->setValue(newStepSize);
			}
			ui->txtStep->setText(util::formatFixed(newStepSize, 2));
			disableConfigLiveEdit = false;
			lastValidConfigSet = configSet;

			QSharedPointer<AllDrawData> data = QSharedPointer<AllDrawData>::create();
			data->uiDrawData.offset = QPoint{xOff, yOff};
			data->uiDrawData.drawingNumToEdit = drawing.drawingNum;
			data->config = configSet;

			getAdditionalOptionsForSegmentsMeta(data->meta);
			invokeExec(data);
		} else {
			translate();
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

		if (moveDown) yOff += dp.areaTopLeft.y() - drawing.topLeft.y();
		if (moveUp) yOff += dp.areaBotRight.y() - drawing.botRight.y();
		if (moveLeft) xOff += dp.areaBotRight.x() - drawing.botRight.x();
		if (moveRight) xOff += dp.areaTopLeft.x() - drawing.topLeft.x();

		translate();
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

void LSystemUi::undoRedo()
{
	drawArea->restoreLastImage();
	if (drawArea->getCurrentDrawing()) {
		showConfigSet(drawArea->getCurrentDrawing()->config);

		if (symbolsVisible()) showSymbols();
	}
}

// -------------------- DrawAreaMenu --------------------

LSystemUi::DrawAreaMenu::DrawAreaMenu(LSystemUi * parent)
	: menu(parent)
{
	drawingActions << menu.addAction("Delete drawing", Qt::Key_Delete, &*parent->drawArea, &DrawArea::deleteMarked)
				   << menu.addAction("Copy drawing", Qt::CTRL | Qt::Key_C, parent, &LSystemUi::copyToClipboardMarked)
				   << menu.addAction("Send to front", &*parent->drawArea, &DrawArea::sendToFrontMarked)
				   << menu.addAction("Send to back", &*parent->drawArea, &DrawArea::sendToBackMarked) << menu.addSeparator();

	setDrawingActionsVisible(false);

	undoAction = menu.addAction("Undo", Qt::CTRL | Qt::Key_Z, parent, &LSystemUi::undoRedo);
	undoAction->setEnabled(false);
	redoAction = menu.addAction("Redo", Qt::CTRL | Qt::Key_Y, parent, &LSystemUi::undoRedo);
	redoAction->setEnabled(false);
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

// ------------------------------------------------------
