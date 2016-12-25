#-------------------------------------------------
#
# Project created by QtCreator 2013-03-01T15:55:52
#
#-------------------------------------------------

QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = radeon-profile
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++0x

#   https://forum.qt.io/topic/10178/solved-qdebug-and-debug-release/2
#   http://doc.qt.io/qt-5/qtglobal.html#QtMsgType-enum
#   qDebug will work only when compiled for Debug
#   QtWarning, QtCritical and QtFatal will still work on Release
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

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
    execbin.cpp \
    dialog_rpevent.cpp \
    eventsTab.cpp \
    fanControlTab.cpp

HEADERS  += radeon_profile.h \
    qcustomplot.h \
    gpu.h \
    dxorg.h \
    dfglrx.h \
    globalStuff.h \
    daemonComm.h \
    execbin.h \
    dialog_rpevent.h \
    rpevent.h

FORMS    += radeon_profile.ui \
    dialog_rpevent.ui

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

# http://doc.qt.io/qt-5/qmake-advanced-usage.html#installing-files
# binary: /usr/bin/radeon-profile
binary.path = $$PREFIX/$(DESTDIR)/usr/bin/
binary.files = radeon-profile
# icon: /usr/share/icons/radeon-profile.png
icon.path = $$PREFIX/$(DESTDIR)/usr/share/icons/
icon.files = extra/radeon-profile.png
# desktop: /usr/share/applications/radeon-profile.desktop
desktop.path = $$PREFIX/$(DESTDIR)/usr/share/applications/
desktop.files = extra/radeon-profile.desktop
# appdata: /usr/share/appdata/radeon-profile.appdata.xml
appdata.path = $$PREFIX/$(DESTDIR)/usr/share/appdata/
appdata.files = extra/radeon-profile.appdata.xml
# translate: /usr/share/radeon-profile/strings.*.qm
translate.path = $$PREFIX/$(DESTDIR)/usr/share/radeon-profile/
translate.commands = lrelease radeon-profile.pro || lrelease-qt5 radeon-profile.pro || lrelease-qt4 radeon-profile.pro;
translate.commands += for tr in translations/*.qm; do echo \$$tr; install -Dm644 \$$tr $(INSTALL_ROOT)$${translate.path}\$$tr; done
INSTALLS += binary icon desktop appdata translate

