/*
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

#ifndef TRANSFERPLUGINLOADER_H
#define TRANSFERPLUGINLOADER_H

#include <QObject>

#include "sharingmethodinfo.h"

class SharingPluginLoaderPrivate;
class SharingPluginLoader : public QObject
{
    Q_OBJECT

public:
    explicit SharingPluginLoader(QObject *parent = nullptr);
    ~SharingPluginLoader();
    const QList<SharingMethodInfo> &plugins() const;
    void reload();
    bool loaded() const;

signals:
    void pluginsChanged();
    void pluginsLoaded();

private:
    SharingPluginLoaderPrivate *d_ptr = nullptr;
    Q_DECLARE_PRIVATE(SharingPluginLoader)
};

#endif // TRANSFERPLUGINLOADER_H
