QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Animation.cpp \
    Export.cpp \
    Palette.cpp \
    Sprite.cpp \
    TabAlign.cpp \
    TabAnimation.cpp \
    dialogPickFamiPalette.cpp \
    dialogpickpalette.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Animation.h \
    Palette.h \
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

RESOURCES += \
    resources.qrc
