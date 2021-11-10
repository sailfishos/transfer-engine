TEMPLATE = lib
TARGET = $$qtLibraryTarget(exampleshareplugin)
CONFIG += plugin
DEPENDPATH += .
INCLUDEPATH += ../translatorplugin

CONFIG += link_pkgconfig
PKGCONFIG += nemotransferengine-qt5

# Input
HEADERS += \
    ../translatorplugin/exampleplugintranslator.h \
    exampleplugininfo.h \
    exampleshareplugin.h

SOURCES += \
    ../translatorplugin/exampleplugintranslator.cpp \
    exampleplugininfo.cpp \
    exampleshareplugin.cpp

OTHER_FILES += \
    ExampleShareUI.qml

shareui.files = *.qml
shareui.path = /usr/share/nemo-transferengine/plugins/sharing

target.path = $$LIBDIR/nemo-transferengine/plugins/sharing
INSTALLS += target shareui
