TEMPLATE = lib
TARGET = nemotransferengine-qt5
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += shared qt create_pc create_prl no_install_prl link_pkgconfig
QT += dbus
PKGCONFIG += accounts-qt5 quillmetadata-qt5

DEFINES += SHARE_PLUGINS_PATH=\"\\\"$$[QT_INSTALL_LIBS]/nemo-transferengine/plugins/sharing\\\"\"

system(qdbusxml2cpp -v -c TransferEngineInterface -p transferengineinterface.h:transferengineinterface.cpp -i metatypedeclarations.h ../dbus/org.nemo.transferengine.xml)

PUBLIC_HEADERS += \
    transferdbrecord.h \
    metatypedeclarations.h \
    transfertypes.h \
    mediatransferinterface.h \
    transferplugininterface.h \
    mediaitem.h \
    sharingmethodinfo.h \
    sharingplugininfo.h \
    sharingplugininfov2.h \
    sharingcontenthints.h \
    sharingplugininterface.h \
    sharingplugininterfacev2.h \
    sharingpluginloader.h \
    transferengineclient.h \
    imageoperation.h

HEADERS += \
    sharingpluginloader_p.h \

SOURCES += \
    transferdbrecord.cpp \
    mediatransferinterface.cpp \
    mediaitem.cpp \
    sharingmethodinfo.cpp \
    sharingcontenthints.cpp \
    sharingpluginloader.cpp \
    transferengineclient.cpp \
    imageoperation.cpp

# generated files
PUBLIC_HEADERS += \
    transferengineinterface.h


SOURCES += \
   transferengineinterface.cpp

OTHER_FILES += nemotransferengine-qt5.pc nemotransferengine-plugin-qt5.prf

HEADERS += $$PUBLIC_HEADERS
headers.files = $$PUBLIC_HEADERS
headers.path = $$[QT_INSTALL_PREFIX]/include/TransferEngine-qt5

target.path = $$[QT_INSTALL_LIBS]

pkgconfigpc.path = $$[QT_INSTALL_LIBS]/pkgconfig/
pkgconfigpc.files = nemotransferengine-qt5.pc

prf.path = $$[QT_INSTALL_DATA]/mkspecs/features
prf.files = nemotransferengine-plugin-qt5.prf

QMAKE_PKGCONFIG_NAME = lib$$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Nemo transfer engine library for share plugins and transfer engine API
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_REQUIRES = Qt5Core Qt5DBus
QMAKE_PKGCONFIG_VERSION = $$VERSION

INSTALLS += target headers prf pkgconfigpc
