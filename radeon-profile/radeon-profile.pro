#-------------------------------------------------
#
# Project created by QtCreator 2013-03-01T15:55:52
#
#-------------------------------------------------

QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = radeon-profile
TEMPLATE = app


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
