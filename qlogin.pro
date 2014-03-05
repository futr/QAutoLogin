#-------------------------------------------------
#
# Project created by QtCreator 2013-05-07T17:14:13
#
#-------------------------------------------------

QT       += network core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32 {
    RC_FILE = app.rc
}

macx {
    ICON = appicon.icns
}

TARGET = QAutoLogin
TEMPLATE = app

TRANSLATIONS = qautologin_ja.ts

SOURCES += main.cpp\
        mainwidget.cpp

HEADERS  += mainwidget.h

FORMS    += mainwidget.ui

RESOURCES += \
    resource.qrc
