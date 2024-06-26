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

#ifndef TRANSFERPLUGINLOADER_P_H
#define TRANSFERPLUGINLOADER_P_H

#include <Accounts/Manager>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

#include "sharingmethodinfo.h"
#include "sharingcontenthints.h"
#include "sharingplugininfo.h"
#include "sharingplugininfov2.h"
#include "sharingplugininterface.h"
#include "sharingplugininterfacev2.h"

class SharingPluginInfo;
class SharingPluginLoader;
class SharingPluginLoaderPrivate : public QObject
{
    Q_OBJECT

public:
    explicit SharingPluginLoaderPrivate(SharingPluginLoader *parent);
    ~SharingPluginLoaderPrivate();

public slots:
    void query();

signals:
    void pluginsChanged();
    void sharingMethodsInfoReady();

private slots:
    void pluginDirChanged();
    void pluginInfoReady();
    void pluginInfoError(const QString &msg);
    void pluginInfo2Ready();
    void pluginInfo2Error(const QString &msg);

private:
    static QStringList pluginList();

    SharingPluginLoader *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(SharingPluginLoader)

    bool m_loading = false;
    SharingContentHints m_hints;
    Accounts::Manager m_accountManager;
    QTimer m_fileWatcherTimer;
    QFileSystemWatcher m_fileWatcher;

    QList<SharingPluginInfo *> m_infoObjects;
    QList<SharingPluginInfoV2 *> m_infoObjects2;
    QList<SharingMethodInfo> m_sharingMethods;
};

#endif // TRANSFERPLUGINLOADER_P_H
