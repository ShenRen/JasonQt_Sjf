QT       += core network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

include(../JasonQt_SjfFoundation/JasonQt/JasonQt.pri)

SOURCES += main.cpp

HEADERS += \
    MyServer.h
