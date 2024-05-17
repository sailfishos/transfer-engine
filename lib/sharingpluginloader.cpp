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
    qDeleteAll(m_infoObjects2);
}

QStringList SharingPluginLoaderPrivate::pluginList()
{
    QStringList paths;
    QDir dir(SharePluginsPath);
    for (const QString &plugin : dir.entryList(QStringList() << "*.so", QDir::Files, QDir::NoSort))
        paths << dir.absoluteFilePath(plugin);
    return paths;
}

void SharingPluginLoaderPrivate::query()
{
    if (m_fileWatcherTimer.isActive())
        m_fileWatcherTimer.stop();

    m_loading = true;

    QPluginLoader loader;
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::ExportExternalSymbolsHint);

    qDeleteAll(m_infoObjects);
    m_infoObjects.clear();
    qDeleteAll(m_infoObjects2);
    m_infoObjects2.clear();

    m_sharingMethods.clear();

    for (const QString &plugin : pluginList()) {
        loader.setFileName(plugin);

        SharingPluginInterfaceV2 *interfaceV2 = qobject_cast<SharingPluginInterfaceV2 *>(loader.instance());
        if (interfaceV2) {
            SharingPluginInfoV2 *infoV2 = interfaceV2->infoObject();
            if (!infoV2) {
                qWarning() << Q_FUNC_INFO << "NULL Info object!";
                continue;
            }

            m_infoObjects2 << infoV2;
            connect(infoV2, &SharingPluginInfoV2::infoReady,
                    this, &SharingPluginLoaderPrivate::pluginInfo2Ready);
            connect(infoV2, &SharingPluginInfoV2::infoError,
                    this, &SharingPluginLoaderPrivate::pluginInfo2Error);
            infoV2->query(m_hints);
            continue;
        }

        SharingPluginInterface *interface = qobject_cast<SharingPluginInterface *>(loader.instance());

        if (interface) {
            SharingPluginInfo *info = interface->infoObject();
            if (!info) {
                qWarning() << Q_FUNC_INFO << "NULL Info object!";
                continue;
            }

            m_infoObjects << info;
            connect(info, &SharingPluginInfo::infoReady,
                    this, &SharingPluginLoaderPrivate::pluginInfoReady);
            connect(info, &SharingPluginInfo::infoError,
                    this, &SharingPluginLoaderPrivate::pluginInfoError);
            info->query();
            continue;
        }

        qWarning() << Q_FUNC_INFO << loader.errorString();
    }

    m_loading = false;
    if (m_infoObjects.isEmpty() && m_infoObjects2.isEmpty())
        emit sharingMethodsInfoReady();
}

void SharingPluginLoaderPrivate::pluginDirChanged()
{
    m_fileWatcherTimer.start();
}

void SharingPluginLoaderPrivate::pluginInfoReady()
{
    SharingPluginInfo *info = qobject_cast<SharingPluginInfo *>(sender());

    if (info->info().count() > 0)
        m_sharingMethods << info->info();

    if (!m_infoObjects.removeOne(info))
        qWarning() << Q_FUNC_INFO << "Failed to remove info object!";

    delete info;

    if (!m_loading && m_infoObjects.isEmpty() && m_infoObjects2.isEmpty())
        emit sharingMethodsInfoReady();
}

void SharingPluginLoaderPrivate::pluginInfoError(const QString &msg)
{
    qWarning() << Q_FUNC_INFO << msg;
    SharingPluginInfo *info = qobject_cast<SharingPluginInfo *>(sender());
    m_infoObjects.removeOne(info);

    info->deleteLater();

    if (!m_loading && m_infoObjects.isEmpty())
        emit sharingMethodsInfoReady();
}

void SharingPluginLoaderPrivate::pluginInfo2Ready()
{
    SharingPluginInfoV2 *info = qobject_cast<SharingPluginInfoV2 *>(sender());

    if (info->info().count() > 0)
        m_sharingMethods << info->info();

    if (!m_infoObjects2.removeOne(info))
        qWarning() << Q_FUNC_INFO << "Failed to remove info object!";

    delete info;

    if (!m_loading && m_infoObjects.isEmpty() && m_infoObjects2.isEmpty())
        emit sharingMethodsInfoReady();
}

void SharingPluginLoaderPrivate::pluginInfo2Error(const QString &msg)
{
    qWarning() << Q_FUNC_INFO << msg;
    SharingPluginInfoV2 *info = qobject_cast<SharingPluginInfoV2 *>(sender());
    m_infoObjects2.removeOne(info);

    info->deleteLater();

    if (!m_loading && m_infoObjects.isEmpty())
        emit sharingMethodsInfoReady();

}

SharingPluginLoader::SharingPluginLoader(QObject *parent)
    : QObject(parent)
    , d_ptr(new SharingPluginLoaderPrivate(this))
{
    Q_D(SharingPluginLoader);
    connect(d, &SharingPluginLoaderPrivate::pluginsChanged,
            this, &SharingPluginLoader::pluginsChanged);
    connect(d, &SharingPluginLoaderPrivate::sharingMethodsInfoReady,
            this, &SharingPluginLoader::sharingMethodsInfoReady);
}

SharingPluginLoader::~SharingPluginLoader()
{
}

/*!
    Returns list of sharing methods.
 */
const QList<SharingMethodInfo> &SharingPluginLoader::sharingMethods() const
{
    Q_D(const SharingPluginLoader);
    return d->m_sharingMethods;
}

/*
   Request sharing methods.

   Call this after receiving SharingPluginLoader::pluginsChanged to
   reload plugins, or when needing methods matching a new query.
 */
void SharingPluginLoader::querySharingMethods(const SharingContentHints &hints)
{
    Q_D(SharingPluginLoader);
    d->m_hints = hints;
    QMetaObject::invokeMethod(d, "query", Qt::QueuedConnection);
}

/*
   Returns whether sharing method queries are done

   Plugins are loaded asynchronously, thus this may return false after
   construction or querySharingMethods(). After all plugins have processed queries, i.e.
   SharingPluginLoader::isSharingMethodsInfoReady is signaled, this returns true.
 */
bool SharingPluginLoader::isSharingMethodsInfoReady() const
{
    Q_D(const SharingPluginLoader);
    return !d->m_loading && d->m_infoObjects.isEmpty();
}
