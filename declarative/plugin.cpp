/*
 * Copyright (c) 2014 - 2019 Jolla Ltd.
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

#include <QtGlobal>
#include <QtQml>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>

#include "declarativetransfermodel.h"

class Q_DECL_EXPORT DeclarativePlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.nemomobile.transferengine")

public:

    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_UNUSED(engine)
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("org.nemomobile.transferengine"));

    }

    void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("org.nemomobile.transferengine"));

        qmlRegisterType<TransferModel>(uri, 1, 0, "TransferModel");
    }
};

#include "plugin.moc"
