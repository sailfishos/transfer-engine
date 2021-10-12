/*
 * Copyright (c) 2013 - 2019 Jolla Ltd.
 * Copyright (c) 2021 Open Mobile Platform LLC.
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

#ifndef SHARINGPLUGININFO_H
#define SHARINGPLUGININFO_H
#include <QObject>
#include "sharingmethodinfo.h"

class SharingPluginInfo: public QObject
{
    Q_OBJECT
public:
    virtual QList<SharingMethodInfo> info() const = 0;
    virtual void query() = 0;

Q_SIGNALS:
    void infoReady();
    void infoError(const QString &msg);
};

#endif // SHARINGPLUGININFO_H
