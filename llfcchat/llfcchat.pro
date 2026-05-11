#-------------------------------------------------
#
# Project created by QtCreator 2024-02-27T14:01:20
#
#-------------------------------------------------

QT       += core gui network
QT += core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = llfcchat
TEMPLATE = app
RC_ICONS = icon.ico
DESTDIR = ./bin
DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
        clickedlabel.cpp \
        global.cpp \
        httpmgr.cpp \
        logindialog.cpp \
        main.cpp \
        mainwindow.cpp \
        registerdialog.cpp \
        tcpmgr.cpp \
        userdata.cpp \
        usermgr.cpp

HEADERS += \
        clickedlabel.h \
        global.h \
        httpmgr.h \
        logindialog.h \
        mainwindow.h \
        registerdialog.h \
        singleton.h \
        tcpmgr.h \
        userdata.h \
        usermgr.h

FORMS += \
        logindialog.ui \
        mainwindow.ui \
        registerdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS = target

RESOURCES += \
    rc.qrc

DISTFILES += \
    config.ini

CONFIG(debug, debug|release) {
    message("debug mode")
    TargetConfig = $${PWD}/config.ini
    TargetConfig = $$replace(TargetConfig, /, \\)
    OutputDir =  $${OUT_PWD}/$${DESTDIR}
    OutputDir = $$replace(OutputDir, /, \\)
    QMAKE_POST_LINK += copy /Y \"$$TargetConfig\" \"$$OutputDir\" &
}else{
    message("release mode")
    TargetConfig = $${PWD}/config.ini
    TargetConfig = $$replace(TargetConfig, /, \\)
    OutputDir =  $${OUT_PWD}/$${DESTDIR}
    OutputDir = $$replace(OutputDir, /, \\)
    QMAKE_POST_LINK += copy /Y \"$$TargetConfig\" \"$$OutputDir\"
}

win32-msvc*:QMAKE_CXXFLAGS += /wd"4819" /utf-8
