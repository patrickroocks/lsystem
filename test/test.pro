QT += core testlib gui

CONFIG += c++17

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../lsystemapp

SOURCES +=  \
	simulator_base_test.cpp

HEADERS += \
	../lsystemapp/simulator.h \
	../lsystemapp/common.h \
	../lsystemapp/util/compareutils.h \
	../lsystemapp/util/div-utils.h \
	../lsystemapp/util/print.h \
	../lsystemapp/util/qt-cont-utils.h \

SOURCES +=  \
	../lsystemapp/simulator.cpp \
	../lsystemapp/common.cpp \
	../lsystemapp/util/compareutils.cpp \
