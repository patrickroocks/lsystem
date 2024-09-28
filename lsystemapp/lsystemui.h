#pragma once

#include <angleevaluator.h>

#include <drawing.h>
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

namespace lsystem::ui {
class DrawArea;
} // namespace lsystem::ui

namespace lsystem {
class ConfigList;
class ConfigFileStore;
class DefinitionModel;
class SegmentAnimator;
class SegmentDrawer;
class Simulator;
} // namespace lsystem

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
	void drawFrameDone(const QSharedPointer<lsystem::ui::DrawingFrame> & drawing, const QSharedPointer<lsystem::common::AllDrawData> & data);

	// from drawarea
	void highlightChanged(std::optional<lsystem::ui::DrawingSummary> drawResult);
	void markingChanged();
	void processDrawAction(const QString & link);
	void showSymbols();

	// from different other components
	void showErrorInUi(const QString & errString);

	// from animator
	lsystem::common::AnimatorResult newAnimationStep(int step, bool relativeStep);

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
	void setupDrawAreaAndLayers();

	// Draw Area
	void drawAreaClick(int x, int y, Qt::MouseButton button, bool drawingMarked);
	void startPaint(int x, int y);
	DrawPlacement getDrawPlacement(const lsystem::ui::DrawingFrameSummary & drawingFrameResult) const;
	void maximizeDrawing(const lsystem::ui::DrawingFrameSummary & drawing, std::optional<qint64> drawingNumToEdit, bool causedByLink);

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
	void getAdditionalOptionsForSegmentsMeta(lsystem::common::MetaData & execMeta, bool noMaximize = false);
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
	void toggleHelperFrame(QPushButton * button, QWidget * frame);

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

	bool eventFilter(QObject * obj, QEvent * event) override;

private:
	Ui::LSystemUi * const ui;
	lsystem::ui::DrawArea * drawArea = nullptr; // instance lives within ui

	QScopedPointer<TableItemDelegateAutoUpdate> tableItemDelegate;
	QScopedPointer<QuickAngle> quickAngle;
	QScopedPointer<QuickLinear> quickLinear;

	QScopedPointer<lsystem::DefinitionModel> defModel;
	QScopedPointer<lsystem::ConfigList> configList;
	QScopedPointer<lsystem::ConfigFileStore> configFileStore;
	QScopedPointer<lsystem::Simulator> simulator;
	QThread simulatorThread;
	QScopedPointer<lsystem::SegmentDrawer> segDrawer;
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

	struct StatusMenu
	{
		StatusMenu(LSystemUi * parent);
		QMenu menu;
	};
	QScopedPointer<StatusMenu> statusMenu;

	lsystem::common::ConfigSet lastValidConfigSet;
	QString curConfigName;
	std::optional<lsystem::ui::DrawingSummary> highlightedDrawing;
	QScopedPointer<ClickableLabel> lblDrawActions;
	QScopedPointer<SymbolsDialog> symbolsDialog;
	AngleEvaluator angleEvaluator;
	lsystem::common::ColorGradient colorGradient;
	QSharedPointer<lsystem::common::AllDrawData> lastDrawData;
};
