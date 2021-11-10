TEMPLATE = lib
TARGET = $$qtLibraryTarget(exampletransferplugin)
CONFIG += plugin
DEPENDPATH += .

CONFIG += link_pkgconfig
PKGCONFIG += nemotransferengine-qt5

# Input
HEADERS += \
    exampleuploader.h \
    exampletransferplugin.h

SOURCES += \
    exampleuploader.cpp \
    exampletransferplugin.cpp

target.path = $$LIBDIR/nemo-transferengine/plugins/transfer
INSTALLS += target
