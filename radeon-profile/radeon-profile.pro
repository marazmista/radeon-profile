#-------------------------------------------------
#
# Project created by QtCreator 2013-03-01T15:55:52
#
#-------------------------------------------------

QT       += core gui network widgets charts

TARGET = radeon-profile
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

#   https://forum.qt.io/topic/10178/solved-qdebug-and-debug-release/2
#   http://doc.qt.io/qt-5/qtglobal.html#QtMsgType-enum
#   qDebug will work only when compiled for Debug
#   QtWarning, QtCritical and QtFatal will still work on Release
CONFIG(release, debug|release){
    message('Building for release')
    DEFINES += QT_NO_DEBUG_OUTPUT
} else {
    message('Building for debug')
    QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic
}

SOURCES += main.cpp\
        radeon_profile.cpp \
    uiElements.cpp \
    uiEvents.cpp \
    gpu.cpp \
    dxorg.cpp \
    settings.cpp \
    daemonComm.cpp \
    ioctlHandler.cpp \
    ioctl_radeon.cpp \
    ioctl_amdgpu.cpp \
    execTab.cpp \
    execbin.cpp \
    eventsTab.cpp \
    fanControlTab.cpp \
    plotsTab.cpp \
    dialogs/dialog_defineplot.cpp \
    dialogs/dialog_rpevent.cpp \
    dialogs/dialog_topbarcfg.cpp \
    dialogs/dialog_deinetopbaritem.cpp

HEADERS  += radeon_profile.h \
    gpu.h \
    dxorg.h \
    globalStuff.h \
    daemonComm.h \
    execbin.h \
    rpevent.h \
    ioctlHandler.h \
    components/rpplot.h \
    components/pieprogressbar.h \
    components/topbarcomponents.h \
    dialogs/dialog_defineplot.h \
    dialogs/dialog_rpevent.h \
    dialogs/dialog_topbarcfg.h \
    dialogs/dialog_deinetopbaritem.h

FORMS    += radeon_profile.ui \
    components/pieprogressbar.ui \
    dialogs/dialog_defineplot.ui \
    dialogs/dialog_rpevent.ui \
    dialogs/dialog_topbarcfg.ui \
    dialogs/dialog_deinetopbaritem.ui

RESOURCES += \
    radeon-resource.qrc

# NOTE FOR PACKAGING
# /usr/include/X11/extensions/Xrandr.h must be present at compile time
# /usr/lib/libXrandr.so must be present at runtime
# These are provided in libxrandr(Arch), libXrandr(RedHat,Fedora), libxrandr-dev(Debian,Ubuntu), libxrandr-devel(SUSE)
LIBS += -lXrandr -lX11

# NOTE FOR PACKAGING
# In this folder are present translation files (strings.*.ts)
# After editing a translatable string it is necessary to run "lupdate" in this folder (QtCreator: Tools > External > Linguist > Update)
# Then these files can be edited with the Qt linguist
# When the translations are ready it is necessary to run "lrelease" in this folder (QtCreator: Tools > External > Linguist > Release)
# This produces compiled translation files (strings.*.qm), that need to be packaged together with the runnable
# These can be placed in "/usr/share/radeon-profile/" or in the same folder of the binary (useful for development)
TRANSLATIONS += translations/strings.it.ts \
    translations/strings.pl.ts \
    translations/strings.hr.ts

DISTFILES += \
    translations/strings.it.ts \
    translations/strings.pl.ts \
    translations/strings.hr.ts
