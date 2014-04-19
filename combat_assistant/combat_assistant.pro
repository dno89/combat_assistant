#-------------------------------------------------
#
# Project created by QtCreator 2014-04-19T10:47:53
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = combat_assistant
TEMPLATE = app


SOURCES += main.cpp\
		mainwindow.cpp \
		pcinsert.cpp \
		npcinsert.cpp

HEADERS  += mainwindow.h \
			pcinsert.h \
			npcinsert.h \
			structs.h

FORMS    += mainwindow.ui \
			 pcinsert.ui \
			 npcinsert.ui

			 QMAKE_CXXFLAGS += -std=c++0x

LIBS += -llua5.2

INCLUDEPATH += /usr/include/lua5.2/

RESOURCES += \
	resources.qrc
