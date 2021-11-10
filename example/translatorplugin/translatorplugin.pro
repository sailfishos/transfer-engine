TEMPLATE = lib
TARGET = example-qml-plugin
CONFIG += qt plugin
QT += qml

HEADERS = \
    exampleplugintranslator.h

SOURCES = \
    exampletranslatorplugin.cpp \
    exampleplugintranslator.cpp

# Use your own path
target.path = $$[QT_INSTALL_QML]/Sailfish/TransferEngine/Example

import.files = qmldir
import.path = $$target.path

INSTALLS += import target
