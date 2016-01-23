#-------------------------------------------------
#
# Project created by QtCreator 2013-03-01T15:55:52
#
#-------------------------------------------------

QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = radeon-profile
TEMPLATE = app

#   https://forum.qt.io/topic/10178/solved-qdebug-and-debug-release/2
#   http://doc.qt.io/qt-5/qtglobal.html#QtMsgType-enum
#   qDebug will work only when compiled for Debug
#   QtWarning, QtCritical and QtFatal will still work on Release
@
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
@

SOURCES += main.cpp\
        radeon_profile.cpp \
    qcustomplot.cpp \
    uiElements.cpp \
    uiEvents.cpp \
    gpu.cpp \
    dxorg.cpp \
    dfglrx.cpp \
    settings.cpp \
    daemonComm.cpp \
    execTab.cpp \
    execbin.cpp

HEADERS  += radeon_profile.h \
    qcustomplot.h \
    gpu.h \
    dxorg.h \
    dfglrx.h \
    globalStuff.h \
    daemonComm.h \
    execbin.h

FORMS    += radeon_profile.ui

RESOURCES += \
    radeon-resource.qrc

LIBS += -lXrandr -lX11
