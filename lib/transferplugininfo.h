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

#ifndef TRANSFERPLUGININFO_H
#define TRANSFERPLUGININFO_H
#include <QObject>
#include "transfermethodinfo.h"

class TransferPluginInfo: public QObject
{
    Q_OBJECT
public:
    virtual QList<TransferMethodInfo> info() const = 0;
    virtual void query() = 0;
    virtual bool ready() const = 0;

Q_SIGNALS:
    void infoReady();
    void infoError(const QString &msg);
};

#endif // TRANSFERPLUGINSTATUS_H
