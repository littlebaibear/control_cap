#-------------------------------------------------
#
# Project created by QtCreator 2021-05-06T10:39:31
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Control_cap
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
RC_ICONS = contorl64.ico

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    serialwriteread.cpp \
    pgrcam.cpp

HEADERS += \
        mainwindow.h \
    serialwriteread.h \
    pgrcam.h

FORMS += \
        mainwindow.ui
#default appended
#CONFIG += C++11
INCLUDEPATH += "C:\Program Files\Point Grey Research\FlyCapture2\include"
LIBS += "C:\Program Files\Point Grey Research\FlyCapture2\lib64\FlyCapture2.lib"

RESOURCES += \
    imgs.qrc
