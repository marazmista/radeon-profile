TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    main.c \
    radeon_ioctl.c

HEADERS += \
    radeon_ioctl.h

QMAKE_CFLAGS += \
    -Wall \
    -Wextra \
    -Wpedantic

DISTFILES += \
    findCard.sh

QMAKE_INCDIR += \
    /usr/include/libdrm

