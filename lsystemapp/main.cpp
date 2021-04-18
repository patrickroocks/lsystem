#include "lsystemui.h"

#include <version.h>

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	if (a.arguments().contains("--version")) {
		QTextStream(stdout) << "lsystem version " << lsystem::common::Version << endl;
		return 0;
	}

	LSystemUi w;
	w.show();
	return a.exec();
}
