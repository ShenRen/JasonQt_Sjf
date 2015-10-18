TEMPLATE = app

QT += qml quick widgets

include(../JasonQt_SjfFoundation/JasonQt/JasonQt.pri)

SOURCES += ./cpp/main.cpp

RESOURCES += ./qml/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)
