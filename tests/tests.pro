PACKAGENAME = nemo-transfer-engine-qt5

TEMPLATE = app
TARGET = ut_nemo-transfer-engine
DEPENDPATH += .
INCLUDEPATH += . ../src ../lib
CONFIG += link_pkgconfig
PKGCONFIG += quillmetadata-qt5
QT += dbus

# Test files
HEADERS += \
    ut_imageoperation.h \
    ut_mediatransferinterface.h

SOURCES += \
    main.cpp \
    ut_imageoperation.cpp \
    ut_mediatransferinterface.cpp


# Import filess from the actual project
HEADERS += \
    ../lib/imageoperation.h \
    ../lib/mediatransferinterface.h \
    ../lib/mediaitem.h

SOURCES += \
    ../lib/imageoperation.cpp \
    ../lib/mediatransferinterface.cpp \
    ../lib/mediaitem.cpp


QT += testlib

PATH = /opt/tests/$${PACKAGENAME}

tests_xml.target = tests.xml
tests_xml.depends = $$PWD/tests.xml.in
tests_xml.commands = sed -e "s:@PACKAGENAME@:$${PACKAGENAME}:g" $< > $@

QMAKE_EXTRA_TARGETS = tests_xml
QMAKE_CLEAN += $$tests_xml.target
PRE_TARGETDEPS += $$tests_xml.target

tests_install.depends = tests_xml
tests_install.path = $$PATH
tests_install.files = $$tests_xml.target
tests_install.CONFIG += no_check_exist

resources.files = images/*.jpg
resources.path = $$PATH/images

target.path = $$PATH

INSTALLS += target resources tests_install
