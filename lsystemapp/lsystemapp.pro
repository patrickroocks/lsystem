QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000	# disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
	aboutdialog.cpp \
	clickablelabel.cpp \
	common.cpp \
	configfilestore.cpp \
	configlist.cpp \
	definitionmodel.cpp \
	drawarea.cpp \
	drawingcollection.cpp \
	main.cpp \
	lsystemui.cpp \
	segmentdrawer.cpp \
	simulator.cpp

HEADERS += \
	aboutdialog.h \
	clickablelabel.h \
	common.h \
	configfilestore.h \
	configlist.h \
	definitionmodel.h \
	drawarea.h \
	drawingcollection.h \
	lsystemui.h \
	segmentdrawer.h \
	simulator.h \
	util/qpointenhance.h \
	version.h

FORMS += \
	aboutdialog.ui \
	lsystemui.ui

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
    data/config.qrc
