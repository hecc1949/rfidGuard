#-------------------------------------------------
#
# Project created by QtCreator 2020-11-27T09:13:20
#
#-------------------------------------------------

QT       += core gui webenginewidgets webchannel \
    websockets serialport network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rfidguard
TEMPLATE = app

VERSION = 0.1.0
DEFINES += VERSION_STRING=\\\"$${VERSION}\\\"

INCLUDEPATH += $$PWD/../urfidLib/inc
DEPENDPATH += $$PWD/../urfidLib/inc

if(contains(DEFINES,ARM))   {
    target.path=/usr/rfidguard
    INSTALLS += target
    LIBS += -L$$PWD/../urfidLib-build-arm -lurfid
    nodefiles.path=/usr/rfidguard
    viewfiles.path=/usr/rfidguard/view
}   else    {
    LIBS += -L$$PWD/../urfidLib-build-pc -lurfid
    nodefiles.path = $${OUT_PWD}
    viewfiles.path=$${OUT_PWD}/view
}

nodefiles.files = wod-index.js schemetimer.js
viewfiles.files = view/wod.html view/wod.css view/wod-rfidmon.js view/utils.js view/configs.json

INSTALLS +=nodefiles viewfiles

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    webpageview.cpp \
    websocketchannel.cpp \
    devwrapper.cpp \
    networkchecker.cpp \
    localtoolbar.cpp

HEADERS += \
        mainwindow.h    \
        webpageview.h \
    websocketchannel.h \
    devwrapper.h \
    networkchecker.h \
    localtoolbar.h  \
    ../urfidLib/inc/srcdatformat.h \
    ../urfidLib/inc/writebooktags.h    \
    ../urfidLib/inc/dev_r200.h \
    ../urfidLib/inc/rfidreadermod.hpp  \
    ../urfidLib/inc/gpiodev.h \
    ../urfidLib/inc/dbstore.h  \
    ../urfidLib/inc/inventproxy.h


FORMS += \
        mainwindow.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    ReadMe.md \
    wod-index.js \
    schemetimer.js \
    view/configs.json \
    view/utils.js
