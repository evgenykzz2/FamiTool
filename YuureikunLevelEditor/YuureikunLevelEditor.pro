QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Export.cpp \
    Palette.cpp \
    Sprite.cpp \
    dialogPickFamiPalette.cpp \
    dialogpickpalette.cpp \
    main.cpp \
    mainwindow.cpp \
    wnd_block.cpp \
    wnd_chr.cpp \
    wnd_general.cpp \
    wnd_palette.cpp \
    wnd_screen.cpp \
    wnd_tileset.cpp

HEADERS += \
    Palette.h \
    Screen.h \
    Sprite.h \
    dialogPickFamiPalette.h \
    dialogpickpalette.h \
    mainwindow.h

FORMS += \
    dialogPickFamiPalette.ui \
    dialogpickpalette.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -lgdiplus

RESOURCES += \
    res.qrc
