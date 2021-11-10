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

#include <QDebug>
#include <QDir>
#include <QPluginLoader>
#include <QString>

#include "sharingplugininfo.h"
#include "sharingplugininterface.h"
#include "sharingpluginloader_p.h"
#include "sharingpluginloader.h"

namespace {
const int FileWatcherTimeout = 5000;
const auto SharePluginsPath = QStringLiteral(SHARE_PLUGINS_PATH);
} // namespace

SharingPluginLoaderPrivate::SharingPluginLoaderPrivate(SharingPluginLoader *parent)
    : QObject(parent)
    , q_ptr(parent)
    , m_loading(true)
    , m_accountManager("sharing")
{
    m_fileWatcherTimer.setSingleShot(true);
    m_fileWatcherTimer.setInterval(FileWatcherTimeout);
    connect(&m_fileWatcherTimer, &QTimer::timeout,
            this, &SharingPluginLoaderPrivate::pluginsChanged);

    if (!m_fileWatcher.addPath(SharePluginsPath))
        qWarning() << Q_FUNC_INFO << "Can not monitor" << SharePluginsPath;
    connect(&m_fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, &SharingPluginLoaderPrivate::pluginDirChanged);

    connect(&m_accountManager, &Accounts::Manager::accountCreated,
            this, &SharingPluginLoaderPrivate::pluginsChanged);
    connect(&m_accountManager, &Accounts::Manager::accountRemoved,
            this, &SharingPluginLoaderPrivate::pluginsChanged);
    connect(&m_accountManager, &Accounts::Manager::accountUpdated,
            this, &SharingPluginLoaderPrivate::pluginsChanged);
}

SharingPluginLoaderPrivate::~SharingPluginLoaderPrivate()
{
    qDeleteAll(m_infoObjects);
}

QStringList SharingPluginLoaderPrivate::pluginList()
{
    QStringList paths;
    QDir dir(SharePluginsPath);
    for (const QString &plugin : dir.entryList(QStringList() << "*.so", QDir::Files, QDir::NoSort))
        paths << dir.absoluteFilePath(plugin);
    return paths;
}

void SharingPluginLoaderPrivate::load()
{
    if (m_fileWatcherTimer.isActive())
        m_fileWatcherTimer.stop();

    m_loading = true;

    QPluginLoader loader;
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::ExportExternalSymbolsHint);

    qDeleteAll(m_infoObjects);
    m_infoObjects.clear();
    m_plugins.clear();

    for (const QString &plugin : pluginList()) {
        loader.setFileName(plugin);
        SharingPluginInterface *interface = qobject_cast<SharingPluginInterface *>(loader.instance());

        if (!interface) {
            qWarning() << Q_FUNC_INFO << loader.errorString();
        } else {
            SharingPluginInfo *info = interface->infoObject();
            if (!info) {
                qWarning() << Q_FUNC_INFO << "NULL Info object!";
                continue;
            }

            /* Calling queue() should trigger one of these signals */
            m_infoObjects << info;
            connect(info, &SharingPluginInfo::infoReady,
                    this, &SharingPluginLoaderPrivate::pluginInfoReady);
            connect(info, &SharingPluginInfo::infoError,
                    this, &SharingPluginLoaderPrivate::pluginInfoError);
            info->query();
        }
    }

    m_loading = false;
    if (m_infoObjects.isEmpty())
        emit pluginsLoaded();
}

void SharingPluginLoaderPrivate::pluginDirChanged()
{
    m_fileWatcherTimer.start();
}

void SharingPluginLoaderPrivate::pluginInfoReady()
{
    SharingPluginInfo *info = qobject_cast<SharingPluginInfo *>(sender());

    if (info->info().count() > 0)
        m_plugins << info->info();

    if (!m_infoObjects.removeOne(info))
        qWarning() << Q_FUNC_INFO << "Failed to remove info object!";

    delete info;

    if (!m_loading && m_infoObjects.isEmpty())
        emit pluginsLoaded();
}

void SharingPluginLoaderPrivate::pluginInfoError(const QString &msg)
{
    qWarning() << Q_FUNC_INFO << msg;
    SharingPluginInfo *info = qobject_cast<SharingPluginInfo *>(sender());
    m_infoObjects.removeOne(info);

    info->deleteLater();

    if (!m_loading && m_infoObjects.isEmpty())
        emit pluginsLoaded();
}

SharingPluginLoader::SharingPluginLoader(QObject *parent)
    : QObject(parent)
    , d_ptr(new SharingPluginLoaderPrivate(this))
{
    Q_D(SharingPluginLoader);
    connect(d, &SharingPluginLoaderPrivate::pluginsChanged,
            this, &SharingPluginLoader::pluginsChanged);
    connect(d, &SharingPluginLoaderPrivate::pluginsLoaded,
            this, &SharingPluginLoader::pluginsLoaded);
    QMetaObject::invokeMethod(d, "load", Qt::QueuedConnection);
}

SharingPluginLoader::~SharingPluginLoader()
{
}

/*!
    Returns list of installed sharing plugins.
 */
const QList<SharingMethodInfo> &SharingPluginLoader::plugins() const
{
    Q_D(const SharingPluginLoader);
    return d->m_plugins;
}

/*
   Reload sharing plugins.

   Call this after receiving SharingPluginLoader::pluginsChanged to
   reload plugins.
 */
void SharingPluginLoader::reload()
{
    Q_D(SharingPluginLoader);
    QMetaObject::invokeMethod(d, "load", Qt::QueuedConnection);
}

/*
   Returns whether all plugins have been loaded.

   Plugins are loaded asynchronously, thus this may return false after
   construction or reload(). After all plugins have been loaded, i.e.
   SharingPluginLoader::pluginsLoaded is signaled, this returns true.
 */
bool SharingPluginLoader::loaded() const
{
    Q_D(const SharingPluginLoader);
    return !d->m_loading && d->m_infoObjects.isEmpty();
}
