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
		npcinsert.cpp \
    annotationinsert.cpp \
    monsterinsert.cpp \
    utils.cpp \
    Logger.cpp \
    diceroll.cpp

HEADERS  += mainwindow.h \
			pcinsert.h \
			npcinsert.h \
			structs.h \
    annotationinsert.h \
	InitiativeListModel.h \
	AnnotationsModel.h \
	txtDatabase.h \
    monsterinsert.h \
    utils.h \
    Logger.h \
    diceroll.h

FORMS    += mainwindow.ui \
			 pcinsert.ui \
			 npcinsert.ui \
    annotationinsert.ui \
    monsterinsert.ui \
    diceroll.ui

QMAKE_CXXFLAGS += -std=c++0x

LIBS += -llua5.2 -lboost_regex -lboost_date_time -lboost_chrono -lboost_system

INCLUDEPATH += /usr/include/lua5.2/

RESOURCES += \
	resources.qrc
