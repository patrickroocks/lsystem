QT += core widgets gui quick quickwidgets

CONFIG += c++17

SOURCES += \
	aboutdialog.cpp \
	angleevaluator.cpp \
	angleformuladialog.cpp \
	common.cpp \
	configfilestore.cpp \
	configlist.cpp \
	definitionmodel.cpp \
	drawarea.cpp \
	drawingcollection.cpp \
	lsystemui.cpp \
	main.cpp \
	segmentdrawer.cpp \
	settingsdialog.cpp \
	simulator.cpp \
	symbolsdialog.cpp \
	util/clickablelabel.cpp \
	util/focusablelineedit.cpp \
	util/quickangle.cpp \
	util/quickbase.cpp \
	util/quicklinear.cpp \
	util/tableitemdelegate.cpp

HEADERS += \
	aboutdialog.h \
	angleevaluator.h \
	angleformuladialog.h \
	common.h \
	configfilestore.h \
	configlist.h \
	definitionmodel.h \
	drawarea.h \
	drawingcollection.h \
	lsystemui.h \
	segmentdrawer.h \
	settingsdialog.h \
	simulator.h \
	symbolsdialog.h \
	util/clickablelabel.h \
	util/focusablelineedit.h \
	util/qpointenhance.h \
	util/quickangle.h \
	util/quickbase.h \
	util/quicklinear.h \
	util/tableitemdelegate.h \
	util/valuerestriction.h \
	version.h

FORMS += \
	aboutdialog.ui \
	angleformuladialog.ui \
	lsystemui.ui \
	settingsdialog.ui \
	symbolsdialog.ui

# include lib for utils&tests
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../utiltestlib/release/ -lutiltestlib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../utiltestlib/debug/ -lutiltestlib
else:unix: LIBS += -L$$OUT_PWD/../utiltestlib/ -lutiltestlib

INCLUDEPATH += $$PWD/../utiltestlib
DEPENDPATH += $$PWD/../utiltestlib

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    data/config.qrc \
    util/util.qrc
