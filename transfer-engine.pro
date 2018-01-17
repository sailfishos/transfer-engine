TEMPLATE = subdirs
SUBDIRS  = lib src declarative tests

src.depends = lib
tests.depends = lib
declarative.depends = lib

include(doc/doc.pri)

systemd.files = transferengine.service
systemd.path  = /usr/lib/systemd/user/

OTHER_FILES += \
    rpm/*.spec \
    nemo-transfer-engine.conf \
    dbus/* \
    doc/src/* \
    doc/config/*

INSTALLS += systemd

