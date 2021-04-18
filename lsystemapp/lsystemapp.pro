QT       += core gui

QT += testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
QMAKE_CXXFLAGS += -Wno-deprecated-copy

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdialog.cpp \
    clickablelabel.cpp \
    common.cpp \
    configstore.cpp \
    definitionmodel.cpp \
    drawarea.cpp \
    main.cpp \
    lsystemui.cpp \
    simulator.cpp \
    util/compareutils.cpp

HEADERS += \
	aboutdialog.h \
	clickablelabel.h \
    common.h \
	configstore.h \
    definitionmodel.h \
    drawarea.h \
    lsystemui.h \
    simulator.h \
	util/compareutils.h \
	util/div-utils.h \
	util/print.h \
	util/qt-cont-utils.h \
	version.h

FORMS += \
    aboutdialog.ui \
    lsystemui.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
	data/configs.qrc
