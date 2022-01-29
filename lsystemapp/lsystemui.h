#ifndef LSYSTEMUI_H
#define LSYSTEMUI_H

#include <configfilestore.h>
#include <configlist.h>
#include <definitionmodel.h>
#include <drawarea.h>
#include <segmentdrawer.h>
#include <simulator.h>
#include "util/tableitemdelegate.h"
#include "util/quickangle.h"
#include "util/quicklinear.h"
#include "util/focusablelineedit.h"

#include <QMainWindow>
#include <QTimer>
#include <QMenu>
#include <QShortcut>
#include <QCheckBox>


QT_BEGIN_NAMESPACE
namespace Ui { class LSystemUi; }
QT_END_NAMESPACE

class LSystemUi : public QMainWindow
{
	Q_OBJECT

public:
	LSystemUi(QWidget * parent = nullptr);
	~LSystemUi();

protected:
	void resizeEvent(QResizeEvent * event) override;

private:
	enum class MsgType {
		Info,
		Warning,
		Error
	};

	struct DrawMetaData : public lsystem::common::MetaData {
		int x = 0;
		int y = 0;
		bool clear = false;
		bool resultOk = false;
		QString toString() const override;
	};

private slots:
	void on_cmdAdd_clicked();
	void on_cmdRemove_clicked();
	void on_cmdStore_clicked();
	void on_cmdLoad_clicked();
	void on_cmdDelete_clicked();
	void on_cmdAbout_clicked();

	void on_lstConfigs_doubleClicked(const QModelIndex & index);
	void on_lblStatus_linkActivated(const QString & link);
	void on_lblStatus_mousePressed(QMouseEvent * event);

signals:
	void simulatorExecActionStr();
	void simulatorExec(const lsystem::common::ConfigSet & newConfig, const QSharedPointer<lsystem::common::MetaData> & metaData);
	void simulatorExecDoubleStackSize(const QSharedPointer<lsystem::common::MetaData> & metaData);
	void startDraw(const lsystem::common::ExecResult & execResult, const QSharedPointer<lsystem::common::MetaData> & metaData);

private slots:

	// from simulator
	void processSimulatorResult(const lsystem::common::ExecResult & execResult, const QSharedPointer<lsystem::common::MetaData> & metaData);
	void processActionStr(const QString & actionStr);

	// from segdrawer
	void drawDone(const lsystem::ui::Drawing & drawing, const QSharedPointer<lsystem::common::MetaData> & metaData);

	// from drawarea
	void enableUndoRedo(bool undoOrRedo);
	void translateActiveDrawing(int diffX, int diffY);

	// from different other components
	void showErrorInUi(const QString& errString);

	// from myself (menu)
	void copyToClipboardMarked();

	// controls
	void on_cmdSettings_clicked();
	void on_cmdCloseAdditionalSettings_clicked();
	void on_cmdAdditionalOptions_clicked();
	void on_chkShowLastIter_stateChanged();

private:

	// Draw Area
	void drawAreaClick(int x, int y, Qt::MouseButton button, bool drawingMarked);
	void startPaint(int x, int y);
	void setBgColor();

	// current Config
	void configLiveEdit();
	void focusAngleEdit(FocusableLineEdit * lineEdit);
	void focusLinearEdit(FocusableLineEdit * lineEdit);
	void unfocusAngleEdit();
	void unfocusLinearEdit();
	void checkAutoPaintChanged(int state);

	// additional options
	void getAdditionalOptions(const QSharedPointer<lsystem::common::MetaData> & execMeta);

	// Status
	void copyStatus();

	// Config-List
	lsystem::common::ConfigSet getConfigSet();
	void setConfigSet(const lsystem::common::ConfigSet & configSet);
	void loadConfigByLstIndex(const QModelIndex & index);

	// Common helpers
	void showMessage(const QString & msg, MsgType msgType);
	void resetStatus();
	void showSettings();
	void removeAllSliders();

private:
	Ui::LSystemUi * ui;
	QScopedPointer<lsystem::ui::DrawArea> drawArea;
	QThread drawAreaThread;

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

	QTimer errorDecayTimer;

	class DrawAreaMenu {
	public:
		DrawAreaMenu(LSystemUi * parent);
		QMenu menu;
		QAction * undoAction;
		QAction * redoAction;
		QAction * autoClearToggle;

		void setDrawingActionsVisible(bool visible);

	private:
		QList<QAction *> drawingActions;
	};
	QScopedPointer<DrawAreaMenu> drawAreaMenu;

	struct StatusMenu {
		StatusMenu(LSystemUi * parent);
		QMenu menu;
	};
	QScopedPointer<StatusMenu> statusMenu;

	int lastX = -1;
	int lastY = -1;
	lsystem::common::ConfigSet lastValidConfigSet;

	enum class TransparencyOpt {
		Ask = 0,
		NoTransparency = 1,
		Transparency = 2
	} transparencyOpt = TransparencyOpt::Ask;
};
#endif // LSYSTEMUI_H
