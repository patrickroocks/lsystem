#ifndef LSYSTEMUI_H
#define LSYSTEMUI_H

#include <definitionmodel.h>
#include <configstore.h>
#include <drawarea.h>
#include <simulator.h>
#include <aboutdialog.h>

#include <QMainWindow>
#include <QTimer>
#include <QMenu>

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

	void on_txtStartAngle_textChanged(const QString & arg1);
	void on_txtScaleDown_textChanged(const QString & arg1);
	void on_txtRight_textChanged(const QString & arg1);
	void on_txtIter_textChanged(const QString & arg1);
	void on_txtLeft_textChanged(const QString & arg1);
	void on_txtStep_textChanged(const QString  &arg1);

private:
	// Draw Area
	void drawAreaClick(int x, int y, Qt::MouseButton button);
	void startPaint(int x, int y);
	void setBgColorClear();
	void enableUndoRedo(bool undoOrRedo);

	void processResult(lsystem::Simulator::ExecResult execResult, int x, int y, bool clear);
	void configLiveEdit();
	void copyStatus();

	// Configs
	lsystem::common::ConfigSet getConfigSet();
	void setConfigSet(const lsystem::common::ConfigSet & configSet);
	void loadConfigByLstIndex(const QModelIndex & index);

	// Common helpers
	void showMessage(const QString & errorStr, MsgType msgType);
	void resetStatus();

private:
	Ui::LSystemUi * ui;
	QScopedPointer<DrawArea> drawArea;

	lsystem::DefinitionModel defModel;
	lsystem::ConfigStore configStore;
	lsystem::Simulator simulator;

	QTimer errorDecayTimer;

	struct DrawAreaMenu {
		DrawAreaMenu(LSystemUi * parent);
		QMenu menu;
		QAction * undoAction;
		QAction * redoAction;
		QAction * autoClearToggle;
		QAction * autoPaintToggle;
		QMenu clipboardMenu;
	};
	QScopedPointer<DrawAreaMenu> drawAreaMenu;

	struct StatusMenu {
		StatusMenu(LSystemUi * parent);
		QMenu menu;
	};
	QScopedPointer<StatusMenu> statusMenu;

	int lastX = -1;
	int lastY = -1;

};
#endif // LSYSTEMUI_H
