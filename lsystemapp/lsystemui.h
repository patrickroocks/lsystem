#pragma once

#include <angleevaluator.h>
#include <configfilestore.h>
#include <configlist.h>
#include <definitionmodel.h>
#include <drawarea.h>
#include <segmentanimator.h>
#include <segmentdrawer.h>
#include <simulator.h>
#include <symbolsdialog.h>
#include <util/clickablelabel.h>
#include <util/focusablelineedit.h>
#include <util/gradientpreview.h>
#include <util/quickangle.h>
#include <util/quicklinear.h>
#include <util/tableitemdelegate.h>

#include <QCheckBox>
#include <QMainWindow>
#include <QMenu>
#include <QShortcut>
#include <QTimer>

#include <optional>

QT_BEGIN_NAMESPACE
namespace Ui {
class LSystemUi;
}
QT_END_NAMESPACE

class LSystemUi : public QMainWindow
{
	Q_OBJECT

public:
	LSystemUi(QWidget * parent = nullptr);
	~LSystemUi();

private:
	enum class MsgType
	{
		Info,
		Warning,
		Error
	};

private slots:

	// from simulator
	void processSimulatorSegments(const lsystem::common::ExecResult & execResult,
								  const QSharedPointer<lsystem::common::AllDrawData> & data);
	void processActionStr(const QString & actionStr);

	// from segdrawer
	void drawDone(const QSharedPointer<lsystem::ui::Drawing> & drawing, const QSharedPointer<lsystem::common::AllDrawData> & data);

	// from drawarea
	void enableUndoRedo(bool undoOrRedo);
	void highlightDrawing(std::optional<lsystem::ui::DrawResult> drawResult);
	void markDrawing();
	void processDrawAction(const QString & link);

	// from different other components
	void showErrorInUi(const QString & errString);

	// from animator
	lsystem::common::AnimatorResult newAnimationStep(int step, bool relativeStep);

	// from myself (menu)
	void copyToClipboardMarked();

	// controls
	void onLblGradientStartMousePressed(QMouseEvent * event);
	void onLblGradientEndMousePressed(QMouseEvent * event);
	void onCmdAboutClicked();
	void onCmdAdditionalOptionsClicked();
	void onCmdCloseAdditionalOptionsClicked();
	void onCmdPlayerClicked();
	void onCmdClosePlayerClicked();
	void onChkShowLastIterStateChanged();
	void onCmdStoreConfigClicked();
	void onCmdDeleteConfigClicked();
	void onLblStatusLinkActivated(const QString & link);
	void onLblStatusMousePressed(QMouseEvent * event);
	void onCmdDeleteLayerClicked();
	void onCmdResetDefaultOptionsClicked();

signals:
	void simulatorExec(const QSharedPointer<lsystem::common::AllDrawData> & data);
	void startDraw(const lsystem::common::ExecResult & execResult, const QSharedPointer<lsystem::common::AllDrawData> & data);

	void setAnimateLatency(std::chrono::milliseconds latency);
	void startAnimateCurrentDrawing();
	void stopAnimate();
	void goToAnimationStep(int step);

private:
	struct DrawPlacement
	{
		constexpr static const QPoint outerDist{5, 5};
		QPoint drawingSize;
		QPoint areaWidthHeight;
		QPoint areaTopLeft;
		QPoint areaBotRight;
		QPoint areaSize;
		double fct = 0;
	};

	struct DrawLinks
	{
		static const constexpr char * Maximize = "maximize";

		static const constexpr char * MoveDown = "move_down";
		static const constexpr char * MoveUp = "move_up";
		static const constexpr char * MoveLeft = "move_left";
		static const constexpr char * MoveRight = "move_right";

		static const constexpr char * MoveRightDown = "move_right_down";
		static const constexpr char * MoveLeftDown = "move_left_down";
		static const constexpr char * MoveRightUp = "move_right_up";
		static const constexpr char * MoveLeftUp = "move_left_up";
	};

private:
	// Setup phases
	void setupConfigList();
	void setupServices();
	void setupMainControls();
	void setupHelperControls();
	void setupStatusAndTimers();
	void setupInteractiveControls();
	void setupDrawArea();

	// Draw Area
	void drawAreaClick(int x, int y, Qt::MouseButton button, bool drawingMarked);
	void startPaint(int x, int y);
	void setBgColor();
	DrawPlacement getDrawPlacement() const;

	// current Config
	void configLiveEdit();
	void execConfigLive(const lsystem::common::ConfigSet & configSet);
	void focusAngleEdit(FocusableLineEdit * lineEdit);
	void focusLinearEdit(FocusableLineEdit * lineEdit);
	void unfocusAngleEdit();
	void unfocusLinearEdit();
	void onChkShowSlidersChanged(int state);
	void latencyChanged();

	// additional options & windows
	void getAdditionalOptionsForSegmentsMeta(lsystem::common::MetaData & execMeta);
	void showSymbols();
	bool symbolsVisible() const;
	void showRightAngleDialog();
	void updateGradientStyle(bool updateAfterClick = false);

	// player
	void playPauseChanged(bool playing);
	void playerValueChanged(int value);

	// Status
	void copyStatus();

	// Config-List
	lsystem::common::ConfigSet getConfigSet(bool storeAsLastValid);
	void setConfigSet(const lsystem::common::ConfigSet & configSet);
	void showConfigSet(const lsystem::common::ConfigSet & configSet);
	void loadConfigByLstIndex(const QModelIndex & index);

	// Common helpers
	void showMessage(const QString & msg, MsgType msgType);
	void showWarningInUi(const QString & errString);
	void showVarError(const QString & errorVar, const QString & extraInfo = QString());
	void resetStatus();
	void showSettings();
	void clearAll();
	void toggleHelperFrame(QPushButton * button, QWidget * frame);
	void undoRedo();

	// Execution helpers
	enum class ExecKind
	{
		Segments,
		ActionStr,
		Draw
	};

	void invokeExec(const QSharedPointer<lsystem::common::AllDrawData> & drawData);
	void endInvokeExec(ExecKind execKind);
	void invokeExecPending();

private:
	Ui::LSystemUi * const ui;
	lsystem::ui::DrawArea * drawArea = nullptr;
	DrawPlacement drawPlacement;

	QScopedPointer<TableItemDelegateAutoUpdate> tableItemDelegate;
	QScopedPointer<QuickAngle> quickAngle;
	QScopedPointer<QuickLinear> quickLinear;

	lsystem::DefinitionModel defModel;
	lsystem::ConfigList configList;
	lsystem::ConfigFileStore configFileStore;
	lsystem::Simulator simulator;
	QThread simulatorThread;
	lsystem::SegmentDrawer segDrawer;
	QThread segDrawerThread;
	QScopedPointer<lsystem::SegmentAnimator> segAnimator;

	bool resultAvailable = false;
	bool disableConfigLiveEdit = false;

	struct ExecInfo
	{
		QSet<ExecKind> waitForExecTasks;
		bool active = false;
		bool scheduledPending = false;
		QSharedPointer<lsystem::common::AllDrawData> pendingData;
		QTimer pendingTimer;
	} exec;

	QTimer messageDecayTimer;

	class DrawAreaMenu
	{
	public:
		DrawAreaMenu(LSystemUi * parent);
		QMenu menu;
		QAction * undoAction;
		QAction * redoAction;

		void setDrawingActionsVisible(bool visible);

	private:
		QList<QAction *> drawingActions;
	};
	QScopedPointer<DrawAreaMenu> drawAreaMenu;

	struct StatusMenu
	{
		StatusMenu(LSystemUi * parent);
		QMenu menu;
	};
	QScopedPointer<StatusMenu> statusMenu;

	lsystem::common::ConfigSet lastValidConfigSet;
	QString curConfigName;
	std::optional<lsystem::ui::DrawResult> highlightedDrawing;
	QScopedPointer<ClickableLabel> lblDrawActions;

	enum class TransparencyOpt
	{
		Ask = 0,
		NoTransparency = 1,
		Transparency = 2
	} transparencyOpt
		= TransparencyOpt::Ask;

	QScopedPointer<SymbolsDialog> symbolsDialog;
	AngleEvaluator angleEvaluator;
	lsystem::common::ColorGradient colorGradient;
	QSharedPointer<lsystem::common::AllDrawData> lastDrawData;
};
