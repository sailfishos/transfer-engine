TEMPLATE = lib
QT += dbus sql
CONFIG += qt plugin

INCLUDEPATH += ../lib
LIBS += -lnemotransferengine-qt5 -L../lib

TARGET = declarativetransferengine
QT += qml

uri = org.nemomobile.transferengine


# Input
SOURCES += \
    plugin.cpp \
    declarativetransfermodel.cpp

HEADERS += \
    synchronizelists_p.h \
    declarativetransfermodel.h

OTHER_FILES = qmldir *.qml *.js

!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.target = $$OUT_PWD/qmldir
    copy_qmldir.depends = $$_PRO_FILE_PWD_/qmldir
    copy_qmldir.commands = $(COPY_FILE) \"$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)\" \"$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)\"
    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}

target.path = $$[QT_INSTALL_QML]/$$replace(uri, \\., /)

qmldir.files = qmldir plugins.qmltypes *.qml *.js
qmldir.path = $$target.path

INSTALLS += target qmldir

qmltypes.commands = qmlplugindump -nonrelocatable $$uri 1.0 > $$PWD/plugins.qmltypes
QMAKE_EXTRA_TARGETS += qmltypes
