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
    nQuant/CIELABConvertor.cpp \
    nQuant/DivQuantizer.cpp \
    nQuant/Dl3Quantizer.cpp \
    nQuant/EdgeAwareSQuantizer.cpp \
    nQuant/MedianCut.cpp \
    nQuant/MoDEQuantizer.cpp \
    nQuant/NeuQuantizer.cpp \
    nQuant/PnnLABQuantizer.cpp \
    nQuant/PnnQuantizer.cpp \
    nQuant/SpatialQuantizer.cpp \
    nQuant/WuQuantizer.cpp \
    nQuant/bitmapUtilities.cpp

HEADERS += \
    Palette.h \
    Sprite.h \
    dialogPickFamiPalette.h \
    dialogpickpalette.h \
    mainwindow.h \
    nQuant/CIELABConvertor.h \
    nQuant/DivQuantizer.h \
    nQuant/Dl3Quantizer.h \
    nQuant/EdgeAwareSQuantizer.h \
    nQuant/MedianCut.h \
    nQuant/MoDEQuantizer.h \
    nQuant/NeuQuantizer.h \
    nQuant/PnnLABQuantizer.h \
    nQuant/PnnQuantizer.h \
    nQuant/SpatialQuantizer.h \
    nQuant/WuQuantizer.h \
    nQuant/bitmapUtilities.h \
    nQuant/stdafx.h

FORMS += \
    dialogPickFamiPalette.ui \
    dialogpickpalette.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -lgdiplus
