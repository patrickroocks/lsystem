#include "lsystemui.h"

#include <version.h>

#include <QApplication>

int main(int argc, char *argv[])
{
	lsystem::common::registerCommonTypes();
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
