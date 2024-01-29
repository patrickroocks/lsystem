QT = core testlib

TARGET = utiltestlib

TEMPLATE = lib
DEFINES += UTILTESTLIB_LIBRARY

CONFIG += c++17

HEADERS = \
	qsigwatcher/impl/sigbasewatcher.h \
	qsigwatcher/qsigwatcher.h \
	qsigwatcher/structures.h \
	qsigwatcher/impl/watcherhandler.h \
	qsigwatcher/impl/internalstructures.h \
	util/containerutils.h \
	util/destroyable.h \
	util/divutils.h \
	util/numericutils.h \
	util/print.h \
	util/qtcontutils.h \
	util/test/utils.h

SOURCES = \
	qsigwatcher/impl/internalstructures.cpp \
	qsigwatcher/impl/watcherhandler.cpp \
	qsigwatcher/impl/sigbasewatcher.cpp \
	qsigwatcher/qsigwatcher.cpp \
	util/test/utils.cpp
