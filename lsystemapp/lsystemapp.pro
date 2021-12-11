QT += core widgets gui quick quickcontrols2 quickwidgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000	# disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
	aboutdialog.cpp \
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
	util/clickablelabel.cpp \
	util/focusablelineedit.cpp \
	util/quickangle.cpp \
	util/tableitemdelegate.cpp

HEADERS += \
	aboutdialog.h \
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
	util/clickablelabel.h \
	util/focusablelineedit.h \
	util/qpointenhance.h \
	util/quickangle.h \
	util/tableitemdelegate.h \
	util/valuerestriction.h \
	version.h

FORMS += \
	aboutdialog.ui \
	lsystemui.ui \
	settingsdialog.ui

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
