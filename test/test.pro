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

SOURCES +=  \
	../lsystemapp/simulator.cpp \
	../lsystemapp/common.cpp \


# include lib for utils&tests
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../utiltestlib/release/ -lutiltestlib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../utiltestlib/debug/ -lutiltestlib
else:unix: LIBS += -L$$OUT_PWD/../utiltestlib/ -lutiltestlib

INCLUDEPATH += $$PWD/../utiltestlib
DEPENDPATH += $$PWD/../utiltestlib
