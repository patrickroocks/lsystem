#include "lsystemui.h"

#include <version.h>

#include <QApplication>

int main(int argc, char *argv[])
{
	qRegisterMetaType<lsystem::common::ConfigSet>("lsystem::common::ConfigSet");
	qRegisterMetaType<lsystem::common::ConfigSet>("common::ConfigSet");
	qRegisterMetaType<QSharedPointer<lsystem::common::MetaData>>("QSharedPointer<lsystem::common::MetaData>");
	qRegisterMetaType<QSharedPointer<lsystem::common::MetaData>>("QSharedPointer<common::MetaData>");
	qRegisterMetaType<lsystem::common::ExecResult>("lsystem::common::ExecResult");
	qRegisterMetaType<lsystem::common::ExecResult>("common::ExecResult");
	qRegisterMetaType<lsystem::common::LineSegs>("lsystem::common::LineSegs");
	qRegisterMetaType<lsystem::common::LineSegs>("common::LineSegs");
	qRegisterMetaType<lsystem::ui::Drawing>("lsystem::ui::Drawing");
	qRegisterMetaType<lsystem::ui::Drawing>("ui::Drawing");

	QApplication a(argc, argv);

	if (a.arguments().contains("--version")) {
		QTextStream(stdout) << "lsystem version " << lsystem::common::Version << Qt::endl;
		return 0;
	}

	LSystemUi w;
	w.show();
	return a.exec();
}
