/*
 * Copyright (c) 2013 - 2019 Jolla Ltd.
 *
 * All rights reserved.
 *
 * This file is part of Sailfish Transfer Engine package.
 *
 * You may use this file under the terms of the GNU Lesser General
 * Public License version 2.1 as published by the Free Software Foundation
 * and appearing in the file license.lgpl included in the packaging
 * of this file.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file license.lgpl included in the packaging
 * of this file.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 */

#include <QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDebug>
#include "transferengine.h"


Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString translationPath("/usr/share/translations/");
    QTranslator engineeringEnglish;
    engineeringEnglish.load("nemo-transfer-engine_eng_en", translationPath);
    qApp->installTranslator(&engineeringEnglish);

    QTranslator translator;
    translator.load(QLocale(), "nemo-transfer-engine", "-", translationPath);
    app.installTranslator(&translator);

    //% "Transfers"
    app.setApplicationName(qtTrId("transferengine-ap-transfers"));

    TransferEngine engine;
    return app.exec();
}
