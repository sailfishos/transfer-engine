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

#ifndef SHARINGPLUGININFOV2_H
#define SHARINGPLUGININFOV2_H

#include <QObject>
#include "sharingmethodinfo.h"
#include "sharingcontenthints.h"

class SharingPluginInfoV2: public QObject
{
    Q_OBJECT
public:
    virtual QList<SharingMethodInfo> info() const = 0;
    // start query for sharing methods. contentHints contain information on content
    // about to be shared and can be used to limit what sharing methods are returned.
    virtual void query(const SharingContentHints &contentHints) = 0;
    virtual void cancelQuery() = 0;

Q_SIGNALS:
    void infoReady();
    void infoError(const QString &msg);
};

#endif // SHARINGPLUGININFOV2_H
