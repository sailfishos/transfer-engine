/****************************************************************************************
**
** Copyright (C) 2013-2016 Jolla Ltd.
** Contact: Marko Mattila <marko.mattila@jollamobile.com>
** All rights reserved.
**
** This file is part of Nemo Transfer Engine package.
**
** You may use this file under the terms of the GNU Lesser General
** Public License version 2.1 as published by the Free Software Foundation
** and appearing in the file license.lgpl included in the packaging
** of this file.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file license.lgpl included in the packaging
** of this file.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Lesser General Public License for more details.
**
****************************************************************************************/

#include "transferengine.h"
#include "transferengine_p.h"
#include "transferplugininterface.h"
#include "mediaitem.h"
#include "dbmanager.h"
#include "transferengine_adaptor.h"
#include "transfertypes.h"
#include "transferplugininfo.h"

#include <QDir>
#include <QtDebug>
#include <QPluginLoader>
#include <QDBusMessage>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QSettings>

#include <notification.h>

#include <signal.h>

#include <Accounts/Manager>

#define SHARE_PLUGINS_PATH "/usr/lib/nemo-transferengine/plugins"
#define CONFIG_PATH "/usr/share/nemo-transferengine/nemo-transfer-engine.conf"
#define FILE_WATCHER_TIMEOUT 5000
#define ACTIVITY_MONITOR_TIMEOUT 1*60*1000 // 1 minute in ms
#define TRANSFER_EXPIRATION_THRESHOLD 3*60 // 3 minutes in seconds

#define TRANSFER_EVENT_CATEGORY "transfer"
#define TRANSFER_COMPLETE_EVENT_CATEGORY "transfer.complete"
#define TRANSFER_ERROR_EVENT_CATEGORY "transfer.error"

#define TRANSFER_PROGRESS_HINT "x-nemo-progress"

TransferEngineSignalHandler * TransferEngineSignalHandler::instance()
{
    static TransferEngineSignalHandler instance;
    return &instance;
}

void TransferEngineSignalHandler::signalHandler(int signal)
{
    if (signal == SIGUSR1) {
        QMetaObject::invokeMethod(TransferEngineSignalHandler::instance(),
                                  "exitSafely",
                                  Qt::AutoConnection);
    }
}

TransferEngineSignalHandler::TransferEngineSignalHandler()
{
    signal(SIGUSR1, TransferEngineSignalHandler::signalHandler);
}

// ---------------------------

// ClientActivityMonitor runs periodic checks if there are transfers which are expired.
// A transfer can be expired e.g. when a client has been crashed in the middle of Sync,
// Download or Upload operation or the client API isn't used properly.
//
// NOTE: This class only monitors if there are expired transfers and emit signal to indicate
// that it's cleaning time.  It is up to Transfer Engine to remoce expired ids from the
// ClientActivityMonitor instance.
ClientActivityMonitor::ClientActivityMonitor(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkActivity()));
    m_timer->start(ACTIVITY_MONITOR_TIMEOUT);
}

ClientActivityMonitor::~ClientActivityMonitor()
{
    m_activityMap.clear();
}

void ClientActivityMonitor::newActivity(int transferId)
{
    // Update or add a new timestamp
    m_activityMap.insert(transferId, QDateTime::currentDateTimeUtc().toTime_t());
}

void ClientActivityMonitor::activityFinished(int transferId)
{
    if (!m_activityMap.contains(transferId)) {
        qWarning() << Q_FUNC_INFO << "Could not find matching TransferId. This is probably an error!";
        return;
    }

    m_activityMap.remove(transferId);
}

bool ClientActivityMonitor::activeTransfers() const
{
    return !m_activityMap.isEmpty();
}

void ClientActivityMonitor::checkActivity()
{
    // Check if there are existing transfers which are not yet finished and
    // they've been around too long. Notify TransferEngine about these transfers.
    QList<int> ids;
    quint32 currTime = QDateTime::currentDateTimeUtc().toTime_t();
    QMap<int, quint32>::const_iterator i = m_activityMap.constBegin();
    while (i != m_activityMap.constEnd()) {
        if ((currTime - i.value()) >= TRANSFER_EXPIRATION_THRESHOLD) {
            ids << i.key();
        }
        i++;
    }

    if (!ids.isEmpty()) {
        emit transfersExpired(ids);
    }
}


// ----------------------------

TransferEnginePrivate::TransferEnginePrivate(TransferEngine *parent):
    m_notificationsEnabled(true),
    q_ptr(parent)
{
    m_fileWatcherTimer = new QTimer(this);
    m_fileWatcherTimer->setSingleShot(true);
    connect(m_fileWatcherTimer, SIGNAL(timeout()), this, SLOT(enabledPluginsCheck()));

    m_delayedExitTimer = new QTimer(this);
    m_delayedExitTimer->setSingleShot(true);
    m_delayedExitTimer->setInterval(5000);
    m_delayedExitTimer->start(); // Exit if nothing happens within 5 sec
    connect(m_delayedExitTimer, SIGNAL(timeout()), this, SLOT(delayedExitSafely()));

    m_fileWatcher = new QFileSystemWatcher(this);
    m_fileWatcher->addPath(SHARE_PLUGINS_PATH);
    connect(m_fileWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(pluginDirChanged()));

    m_accountManager = new Accounts::Manager("sharing", this);
    connect(m_accountManager, SIGNAL(accountCreated(Accounts::AccountId)), this, SLOT(enabledPluginsCheck()));
    connect(m_accountManager, SIGNAL(accountRemoved(Accounts::AccountId)), this, SLOT(enabledPluginsCheck()));
    connect(m_accountManager, SIGNAL(accountUpdated(Accounts::AccountId)), this, SLOT(enabledPluginsCheck()));

    // Exit safely stuff if we recieve certain signal or there are no active transfers
    Q_Q(TransferEngine);
    connect(TransferEngineSignalHandler::instance(), SIGNAL(exitSafely()), this, SLOT(exitSafely()));
    connect(q, SIGNAL(statusChanged(int,int)), this, SLOT(exitSafely()));

    // Monitor expired transfers and cleanup them if required
    m_activityMonitor = new ClientActivityMonitor(this);
    connect(m_activityMonitor, SIGNAL(transfersExpired(QList<int>)), this, SLOT(cleanupExpiredTransfers(QList<int>)));

    QSettings settings(CONFIG_PATH, QSettings::IniFormat);

    if (settings.status() != QSettings::NoError) {
        qWarning() << Q_FUNC_INFO << "Failed to read settings!" << settings.status();
    } else {
        settings.beginGroup("transfers");
        const QString service = settings.value("service").toString();
        const QString path = settings.value("path").toString();
        const QString iface = settings.value("interface").toString();
        const QString method = settings.value("method").toString();
        settings.endGroup();

        if (!service.isEmpty() && !path.isEmpty() && !iface.isEmpty() && !method.isEmpty()) {
            m_defaultActions << Notification::remoteAction("default", "", service, path, iface, method)
                             << Notification::remoteAction("app", "", service, path, iface, method);
        }
    }
}

void TransferEnginePrivate::pluginDirChanged()
{
    // We need to check our plugins, but only after a short period of time
    // because some operations may cause calling this slot over 10 times.
    // E.g. reinstallation of plugins from the RPM package
    m_fileWatcherTimer->start(FILE_WATCHER_TIMEOUT);
}

void TransferEnginePrivate::exitSafely()
{
    if (!m_activityMonitor->activeTransfers()) {
        qDebug() << "Scheduling exit in" << m_delayedExitTimer->interval() << "ms";
        m_delayedExitTimer->start();
    } else {
        m_delayedExitTimer->stop();
    }
}

void TransferEnginePrivate::delayedExitSafely()
{
    if (getenv("TRANSFER_ENGINE_KEEP_RUNNING")) {
        qDebug() << "Keeping transfer engine running";
    } else {
        qDebug() << "Stopping transfer engine";
        qApp->exit();
    }
}

void TransferEnginePrivate::enabledPluginsCheck()
{
    Q_Q(TransferEngine);
    if (m_fileWatcherTimer->isActive()) {
        m_fileWatcherTimer->stop();
    }

    if (m_infoObjects.count() > 0) {
        qWarning() << Q_FUNC_INFO << "Already quering account info" << m_infoObjects.count();
        return;
    }

    // First clear old data
    m_enabledPlugins.clear();
    qDeleteAll(m_infoObjects);
    m_infoObjects.clear();

    QPluginLoader loader;
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::ExportExternalSymbolsHint);

    // Handle the case if all the plugins have been removed.
    QStringList plugins = pluginList();
    if (plugins.isEmpty()) {
        emit q->transferMethodListChanged();
        return;
    }

    // We have plugins
    Q_FOREACH(QString plugin, plugins) {
        loader.setFileName(plugin);
        TransferPluginInterface *interface =
                qobject_cast<TransferPluginInterface*>(loader.instance());

        if (interface && interface->enabled()) {

            TransferPluginInfo *info = interface->infoObject();
            if (!info) {
                qWarning() << Q_FUNC_INFO << "NULL Info object!";
                continue;
            }

            if (info->ready()) {
                if (info->info().count() > 0) {
                    m_enabledPlugins << info->info();
                }
                delete info;
            } else {
                // These object will be cleaned in pluginInfoReady() slot.
                m_infoObjects << info;
                connect(info, SIGNAL(infoReady()), this, SLOT(pluginInfoReady()));
                connect(info, SIGNAL(infoError(QString)), this, SLOT(pluginInfoError(QString)));
                info->query();
            }
        }

        if (!interface) {
            qWarning() << Q_FUNC_INFO << loader.errorString();
        }
    }
}

void TransferEnginePrivate::cleanupExpiredTransfers(const QList<int> &expiredIds)
{
    // Clean up expired items from the database by changing the status to TransferInterrupted. This way
    // database doesn't maintain objects with unifinished state and failed items can be cleaned by the
    // user manually from the UI.
    Q_Q(TransferEngine);
    Q_FOREACH(int id, expiredIds) {
        if (DbManager::instance()->updateTransferStatus(id, TransferEngineData::TransferInterrupted)) {
            m_activityMonitor->activityFinished(id);
            emit q->statusChanged(id, TransferEngineData::TransferInterrupted);
        }
    }
}

void TransferEnginePrivate::recoveryCheck()
{
    QList<TransferDBRecord> records = DbManager::instance()->transfers();
    // Check all transfer which are not properly finished and mark those as
    //  interrupted
    Q_Q(TransferEngine);
    Q_FOREACH(TransferDBRecord record, records) {
        if (record.status == TransferEngineData::TransferStarted ||
            record.status == TransferEngineData::NotStarted) {
            if (DbManager::instance()->updateTransferStatus(record.transfer_id, TransferEngineData::TransferInterrupted)) {
                emit q->statusChanged(record.transfer_id, TransferEngineData::TransferInterrupted);
                if (record.status == TransferEngineData::TransferStarted) {
                    emit q_ptr->activeTransfersChanged(); // It's not active anymore
                }
            }
        }
    }
}

void TransferEnginePrivate::sendNotification(TransferEngineData::TransferType type,
                                             TransferEngineData::TransferStatus status,
                                             qreal progress,
                                             const QString &fileName,
                                             int transferId)
{
    if (!m_notificationsEnabled || fileName.isEmpty()) {
        return;
    }

    QString category;
    QString body;
    QString summary;
    QString previewBody;
    QString previewSummary;
    bool useProgress = false;
    Notification::Urgency urgency = Notification::Normal;
    QString appIcon = QStringLiteral("icon-lock-information");
    QString icon = QStringLiteral("icon-lock-transfer");

    // TODO: explicit grouping of transfer notifications is now removed, as grouping
    // will now be performed by lipstick.  We may need to reinstate group summary
    // notifications at some later point...

    // Notification & Banner rules:
    //
    // Show Banner:
    // - For succesfull uploads and for downloads
    // - For failed Uploads, Downloads, Syncs
    //
    // Show an event in the EventView:
    // - For downloads
    // - For failed uploads, downloads and syncs


    int notificationId = DbManager::instance()->notificationId(transferId);

    if (status == TransferEngineData::TransferFinished) {
        switch(type) {
        case TransferEngineData::Upload:
            //: Notification for successful file upload
            //% "File uploaded"
            previewBody = qtTrId("transferengine-no-file-upload-success");
            previewSummary = fileName;
            category = TRANSFER_EVENT_CATEGORY; // Use "generic" transfer event for uploads
            break;
        case TransferEngineData::Download:
            category = TRANSFER_COMPLETE_EVENT_CATEGORY;
            //: Notification for successful file download
            //% "File downloaded"
            body = qtTrId("transferengine-no-file-download-success");
            summary = fileName;
            previewBody = body;
            previewSummary = summary;
            break;
        case TransferEngineData::Sync:
            // Ok exit
            break;
        default:
            qWarning() << "TransferEnginePrivate::sendNotification: unknown state";
            break;
        }

    } else if (status == TransferEngineData::TransferInterrupted) {
        urgency = Notification::Critical;
        appIcon = QStringLiteral("icon-lock-information");
        category = TRANSFER_ERROR_EVENT_CATEGORY;
        icon.clear();

        switch (type) {
        case TransferEngineData::Upload:
            //: Notification for failed file upload
            //% "Upload failed"
            body = qtTrId("transferengine-la-file_upload_failed");
            break;
        case TransferEngineData::Download:
            //: Notification for failed file download
            //% "Download failed"
            body = qtTrId("transferengine-la-file_download_failed");
            break;
        case TransferEngineData::Sync:
            // no notification required
            category.clear();
            break;
        default:
            qWarning() << "TransferEnginePrivate::sendNotification: unknown state";
            category.clear();
            break;
        }

        summary = fileName;
        previewSummary = summary;
        previewBody = body;

    } else if (status == TransferEngineData::TransferStarted) {
        if (type == TransferEngineData::Upload || type == TransferEngineData::Download) {
            category = TRANSFER_EVENT_CATEGORY;

            if (type == TransferEngineData::Upload) {
                //: Notification for ongoing upload
                //% "File uploading"
                body = qtTrId("transferengine-no-file-uploading");
            } else {
                //: Notification for ongoing file download
                //% "File downloading"
                body = qtTrId("transferengine-no-file-downloading");
            }

            summary = fileName;

            if (progress > 0)
                useProgress = true;
        }

    } else if (status == TransferEngineData::TransferCanceled && notificationId > 0) {
        // Exit, no banners or events when user has canceled a transfer
        // Remove any existing notification
        Notification notification;
        notification.setReplacesId(notificationId);
        notification.close();
        DbManager::instance()->setNotificationId(transferId, 0);
        notificationId = 0;
    }

    if (!category.isEmpty()) {
        Notification notification;

        if (notificationId > 0) {
            notification.setReplacesId(notificationId);
        }

        if (!m_defaultActions.isEmpty()) {
            notification.setRemoteActions(m_defaultActions);
        }

        //: Group name for notifications of successful transfers
        //% "Transfers"
        const QString transfersGroup(qtTrId("transferengine-notification_group"));
        //: Group name for notifications of failed transfers
        //% "Warnings"
        const QString errorsGroup(qtTrId("transferengine-notification_errors_group"));

        notification.setCategory(category);
        notification.setAppName(category == TRANSFER_ERROR_EVENT_CATEGORY ? errorsGroup : transfersGroup);
        notification.setSummary(summary);
        notification.setBody(body);
        notification.setPreviewSummary(previewSummary);
        notification.setPreviewBody(previewBody);
        notification.setUrgency(urgency);

        if (!icon.isEmpty()) {
            notification.setIcon(icon);
        }

        if (useProgress) {
            notification.setHintValue(TRANSFER_PROGRESS_HINT, static_cast<double>(progress));
        }
        notification.publish();
        int newId = notification.replacesId();

        if (newId != notificationId) {
            DbManager::instance()->setNotificationId(transferId, newId);
        }
    }
}

int TransferEnginePrivate::uploadMediaItem(MediaItem *mediaItem,
                                           MediaTransferInterface *muif,
                                           const QVariantMap &userData)
{
    Q_Q(TransferEngine);

    if (mediaItem == 0) {
        qWarning() << "TransferEngine::uploadMediaItem invalid MediaItem";
        return -1;
    }
    if (muif == 0) {
        qWarning() << "TransferEngine::uploadMediaItem Failed to get MediaTransferInterface";
        return -1;
    }

    mediaItem->setValue(MediaItem::TransferType,        TransferEngineData::Upload);
    mediaItem->setValue(MediaItem::DisplayName,         muif->displayName());
    mediaItem->setValue(MediaItem::ServiceIcon,         muif->serviceIcon());
    mediaItem->setValue(MediaItem::CancelSupported,     muif->cancelEnabled());
    mediaItem->setValue(MediaItem::RestartSupported,    muif->restartEnabled());

    // Get and set data from user data if that's set. The following user data values
    // are stored to the database so that's why they are set to the media item too.
    // If the user data is fully custom for plugin it won't be stored to the database and
    // it's up to the plugin to handle or ignore it.
    QString title = userData.value("title").toString();
    QString desc  = userData.value("description").toString();
    qint64  accId = userData.value("accountId").toInt();
    qreal scale   = userData.value("scalePercent").toReal();

    mediaItem->setValue(MediaItem::Title,               title);
    mediaItem->setValue(MediaItem::Description,         desc);
    mediaItem->setValue(MediaItem::AccountId,           accId);
    mediaItem->setValue(MediaItem::ScalePercent,        scale);
    muif->setMediaItem(mediaItem);

    connect(muif, SIGNAL(statusChanged(MediaTransferInterface::TransferStatus)),
            this, SLOT(uploadItemStatusChanged(MediaTransferInterface::TransferStatus)));
    connect(muif, SIGNAL(progressUpdated(qreal)),
            this, SLOT(updateProgress(qreal)));

    // Let's create an entry into Transfer DB
    const int key = DbManager::instance()->createTransferEntry(mediaItem);
    m_keyTypeCache.insert(key, TransferEngineData::Upload);

    if (key < 0) {
        qWarning() << "TransferEngine::uploadMediaItem: Failed to create an entry to transfer database!";
        delete muif;
        return key;
    }

    m_activityMonitor->newActivity(key);
    emit q->transfersChanged();
    emit q->statusChanged(key, TransferEngineData::NotStarted);

    // For now, we just store our uploader to a map. It'll be removed from it when
    // the upload has finished.
    m_plugins.insert(muif, key);
    muif->start();
    return key;
}

QStringList TransferEnginePrivate::pluginList() const
{
    QDir dir(SHARE_PLUGINS_PATH);
    QStringList plugins = dir.entryList(QStringList() << "*.so",
                                        QDir::Files,
                                        QDir::NoSort);
    QStringList filePaths;
    Q_FOREACH(QString plugin, plugins) {
        filePaths << dir.absolutePath() + QDir::separator() + plugin;
    }

    return filePaths;
}

QList <TransferMethodInfo> TransferEnginePrivate::enabledPlugins() const
{
    return m_enabledPlugins;
}

MediaTransferInterface *TransferEnginePrivate::loadPlugin(const QString &pluginId) const
{
    QPluginLoader loader;
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::ExportExternalSymbolsHint);
    Q_FOREACH(QString plugin, pluginList()) {
        loader.setFileName(plugin);
        TransferPluginInterface *interface =
                qobject_cast<TransferPluginInterface*>(loader.instance());


        if (interface && interface->enabled() && interface->pluginId() == pluginId) {
            return interface->transferObject();
        }

        if (!interface) {
            qWarning() << "TransferEngine::loadPlugin: " + loader.errorString();
        }

        if (loader.isLoaded()) {
            loader.unload();
        }
    }

    return 0;
}

QString TransferEnginePrivate::mediaFileOrResourceName(MediaItem *mediaItem) const
{
    if (!mediaItem) {
        return QString();
    }
    QUrl url = mediaItem->value(MediaItem::Url).toUrl();
    if (!url.isEmpty()) {
        QStringList split = url.toString().split(QDir::separator());
        return split.at(split.length()-1);
    }
    return mediaItem->value(MediaItem::ResourceName).toString();
}

void TransferEnginePrivate::uploadItemStatusChanged(MediaTransferInterface::TransferStatus status)
{
    MediaTransferInterface *muif = qobject_cast<MediaTransferInterface*>(sender());
    const int key = m_plugins.value(muif);
    const TransferEngineData::TransferType type =
            static_cast<TransferEngineData::TransferType>(muif->mediaItem()->value(MediaItem::TransferType).toInt());

    TransferEngineData::TransferStatus tStatus = static_cast<TransferEngineData::TransferStatus>(status);

    bool ok = false;
    switch(tStatus) {
    case TransferEngineData::TransferStarted:
        ok = DbManager::instance()->updateTransferStatus(key, tStatus);
        m_activityMonitor->newActivity(key);
        break;

    case TransferEngineData::TransferInterrupted:
    case TransferEngineData::TransferCanceled:
    case TransferEngineData::TransferFinished:
    {
        // If the flow ends up here, we are not interested in any signals the same object
        // might emit. Let's just disconnect them.
        muif->disconnect();
        sendNotification(type, tStatus, muif->progress(), mediaFileOrResourceName(muif->mediaItem()), key);
        ok = DbManager::instance()->updateTransferStatus(key, tStatus);
        if (m_plugins.remove(muif) == 0) {
            qWarning() << "TransferEnginePrivate::uploadItemStatusChanged: Failed to remove media upload object from the map!";
            // What to do here.. Let's just delete it..
        }
        muif->deleteLater();
        muif = 0;
        m_activityMonitor->activityFinished(key);
    } break;

    default:
        qWarning() << "TransferEnginePrivate::uploadItemStatusChanged: unhandled status: "  << tStatus;
        break;
    }

    if (!ok) {
        qWarning() << "TransferEnginePrivate::uploadItemStatusChanged: Failed update share status for the item: " + key;
        return;
    }

    Q_Q(TransferEngine);
    emit q->statusChanged(key, tStatus);
}

void TransferEnginePrivate::updateProgress(qreal progress)
{
    MediaTransferInterface *muif = qobject_cast<MediaTransferInterface*>(sender());
    const int key = m_plugins.value(muif);

    if (!DbManager::instance()->updateProgress(key, progress)) {
        qWarning() << "TransferEnginePrivate::updateProgress: Failed to update progress";
        return;
    }

    m_activityMonitor->newActivity(key);
    Q_Q(TransferEngine);
    q->updateTransferProgress(key, progress);
}

void TransferEnginePrivate::pluginInfoReady()
{
    TransferPluginInfo *infoObj = qobject_cast<TransferPluginInfo*>(sender());

    QList<TransferMethodInfo> infoList = infoObj->info();
    if (!infoList.isEmpty()) {
        m_enabledPlugins << infoList;
    }

    if (m_infoObjects.removeOne(infoObj)) {
        delete infoObj;
    } else {
        qWarning() << Q_FUNC_INFO << "Failed to remove info object!";
        delete infoObj;
    }

    if (m_infoObjects.isEmpty()) {
        Q_Q(TransferEngine);
        emit q->transferMethodListChanged();
    }
}

void TransferEnginePrivate::pluginInfoError(const QString &msg)
{
    qWarning() << "TransferEnginePrivate::pluginInfoError:" << msg;
    TransferPluginInfo *infoObj = qobject_cast<TransferPluginInfo*>(sender());
    m_infoObjects.removeOne(infoObj);
    infoObj->deleteLater();

    if (m_infoObjects.isEmpty()) {
        Q_Q(TransferEngine);
        emit q->transferMethodListChanged();
    }
}


TransferEngineData::TransferType TransferEnginePrivate::transferType(int transferId)
{
    if (!m_keyTypeCache.contains(transferId)) {
        TransferEngineData::TransferType type = DbManager::instance()->transferType(transferId);
        m_keyTypeCache.insert(transferId, type);
        return type;
    } else {
        return m_keyTypeCache.value(transferId);
    }
}

void TransferEnginePrivate::callbackCall(int transferId, CallbackMethodType method)
{
    // Get DBus callback information. Callback list must contain at least
    // service, path, interface and one callback method name. Note that even
    // if the cancel or restart method is missing, it'll be indicated as an
    // empty string. So the list length is always 5.
    QStringList callback = DbManager::instance()->callback(transferId);
    if (callback.length() != 5) {
        qWarning() << "TransferEnginePrivate:callbackCall: Invalid callback interface";
        return;
    }

    QDBusInterface remoteInterface(callback.at(Service),
                                   callback.at(Path),
                                   callback.at(Interface));

    if (!remoteInterface.isValid()) {
        qWarning() << "TransferEnginePrivate::callbackCall: DBus interface is not valid!";
        return;
    }

    if (method >= callback.size()) {
        qWarning() << "TransferEnginePrivate::callbackCall: method index out of range!";
        return;
    }

    const QString methodName = callback.at(method);
    if (methodName.isEmpty()) {
        qWarning() << "TransferEnginePrivate::callbackCall: Failed to get callback method name!";
        return;
    }    
    remoteInterface.call(methodName, transferId);    
}


/*!
    \class TransferEngine
    \brief The TransferEngine class implements the functionality for different transfer types.

    \ingroup transfer-engine

    TransferEngine is the central place for:
    \list
        \li Sharing - Provides requires plugin interfaces for share plugins
        \li Downloads - Provides an API to create Download entries
        \li Syncs - Provides an API to create Sync entries
    \endlist

    For Downloads and Syncs, the Transfer Engine acts only a place to keep track of these operations.
    The actual Download and Sync is executed by a client using TransferEngine API. For sharing
    the TransferEngine provides an API containing a few interaces, which a share plugin must implement.
    TransferEngine also takes care of loading and executing the sharing, based on the API it defines.

    The most essential thing to remember is that Transfer Engine provides share plugin API, DBus API e.g.
    for creating Transfer UI or Share UIs, it stores data to the local sqlite database using DbManager
    and that's it.

    How to implement a share plugin see: TransferPluginInterface, MediaTransferInterface, MediaItem,
    TransferPluginInfo

    TransferEngine provides DBus API, but instead of using it directly, it's recommended to use
    TransferEngineClient. If there is a need to create UI to display e.g. transfer statuses, then
    the DBus API is the recommend way to implement it.

 */

/*!
    \fn void TransferEngine::progressChanged(int transferId, double progress)

    The signal is emitted when \a progress for a transfer with a \a transferId has changed.
*/

/*!
    \fn void TransferEngine::statusChanged(int transferId, int status)

    The signal is emitted when \a status for a transfer with a \a transferId has changed.
*/

/*!
    \fn void TransferEngine::transferMethodListChanged()

    The signal is emitted when transfer methods have changed. Usually tranfer methods change
    when a new plugin is installed to the system or an account has been enabled or disabled.
*/

/*!
    \fn void TransferEngine::transfersChanged()

    The signal is emitted when there is a new transfer or a transfer has been removed from the
    database.
*/

/*!
    Constructor with optional \a parent arguement.
 */
TransferEngine::TransferEngine(QObject *parent) :
    QObject(parent),
    d_ptr(new TransferEnginePrivate(this))
{
    TransferMethodInfoDeprecated::registerType();
    TransferMethodInfo::registerType();
    TransferDBRecord::registerType();

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerObject("/org/nemo/transferengine", this)) {
        qFatal("Could not register object \'/org/nemo/transferengine\'");
    }

    if (!connection.registerService("org.nemo.transferengine")) {
        qFatal("DBUS service already taken. Kill the other instance first.");
    }

    new TransferEngineAdaptor(this);

    // Let's make sure that db is open by creating
    // DbManager singleton instance.
    DbManager::instance();
    Q_D(TransferEngine);
    d->recoveryCheck();
    d->enabledPluginsCheck();    
}

/*!
    Destroys the TransferEngine object.
 */
TransferEngine::~TransferEngine()
{
    Q_D(TransferEngine);
    d->recoveryCheck();
    delete d_ptr;
    d_ptr = 0;

    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.unregisterObject("/org/nemo/transferengine");

    if (!connection.unregisterService("org.nemo.transferengine")) {
        qWarning() << "Failed to unregister org.nemo.tranferengine service";
    }
}


/*!
    DBus adaptor calls this method to start uploading a media item. The minimum information
    needed to start an upload and to create an entry to the transfer database is:
    \a source the path to the media item to be downloaded. \a serviceId the ID of the share
    plugin. See TransferPluginInterface::pluginId() for more details. \a mimeType is the MimeType
    of the media item e.g. "image/jpeg". \a metadataStripped boolean to indicate if the metadata
    should be kept or removed before uploading. \a userData is various kind of data which share UI
    may provide to the engine. UserData is QVariant map i.e. the data must be provided as key-value
    pairs, where the keys must be QStrings.

    TransferEngine handles the following user defined data automatically and stores them to the database:
    \list
        \li "title" Title for the media
        \li "description" Description for the media
        \li "accountId" The ID of the account which is used for sharing. See qt-accounts for more details.
        \li "scalePercent" The scale percent e.g. downscale image to 50% from original before uploading.
    \endlist

    In practice this method instantiates a share plugin with \a serviceId and passes a MediaItem instance filled
    with required data to it. When the plugin has been loaded, the MediaTransferInterface::start() method is called
    and the actual sharing starts.

    This method returns a transfer ID which can be used later to fetch information of this specific transfer.
 */
int TransferEngine::uploadMediaItem(const QString &source,
                                    const QString &serviceId,
                                    const QString &mimeType,
                                    bool metadataStripped,
                                    const QVariantMap &userData)
{
    Q_D(TransferEngine);
    d->exitSafely();

    MediaTransferInterface *muif = d->loadPlugin(serviceId);
    if (muif == 0) {
        qWarning() << "TransferEngine::uploadMediaItem Failed to get MediaTransferInterface";
        return -1;
    }

    QUrl filePath(source);
    QFileInfo fileInfo(filePath.toLocalFile());
    if (!fileInfo.exists()) {
        qWarning() << "TransferEnginePrivate::uploadMediaItem file " << source << " doesn't exist!";
    }

    MediaItem *mediaItem = new MediaItem(muif);
    mediaItem->setValue(MediaItem::Url,                 filePath);
    mediaItem->setValue(MediaItem::MetadataStripped,    metadataStripped);
    mediaItem->setValue(MediaItem::ResourceName,        fileInfo.fileName());
    mediaItem->setValue(MediaItem::MimeType,            mimeType);
    mediaItem->setValue(MediaItem::FileSize,            fileInfo.size());
    mediaItem->setValue(MediaItem::PluginId,            serviceId);
    mediaItem->setValue(MediaItem::UserData,            userData);
    return d->uploadMediaItem(mediaItem, muif, userData);
}

/*!
    DBus adaptor calls this method to start uploading media item content. Sometimes the content
    to be shared is not a file, but data e.g. contact information in vcard format. In order to
    avoid serializing data to a file, pass url to the file, reading the data, deleting the file,
    TransferEngine provides this convenience API.

    \a content is the media item content to be shared. \a serviceId is the id of the share plugin. See
    TransferPluginInterface::pluginId() for more details. \a userData is a QVariantMap containing
    share plugin specific data. See TransferEngine::uploadMediaItem for more details.

    This method returns a transfer ID which can be used later to fetch information of this specific transfer.
*/
int TransferEngine::uploadMediaItemContent(const QVariantMap &content,
                                           const QString &serviceId,
                                           const QVariantMap &userData)
{
    Q_D(TransferEngine);
    d->exitSafely();

    MediaTransferInterface *muif = d->loadPlugin(serviceId);
    if (muif == 0) {
        qWarning() << "TransferEngine::uploadMediaItemContent Failed to get MediaTransferInterface";
        return -1;
    }

    MediaItem *mediaItem = new MediaItem(muif);
    mediaItem->setValue(MediaItem::ContentData,     content.value("data"));
    mediaItem->setValue(MediaItem::ResourceName,    content.value("name"));
    mediaItem->setValue(MediaItem::MimeType,        content.value("type"));
    mediaItem->setValue(MediaItem::ThumbnailIcon,   content.value("icon"));
    mediaItem->setValue(MediaItem::PluginId,        serviceId);
    mediaItem->setValue(MediaItem::UserData,        userData);
    return d->uploadMediaItem(mediaItem, muif, userData);
}

/*!
    DBus adaptor calls this method to create a download entry. Note that this is purely write-only
    method and doesn't involve anything else from TransferEngine side than creating a new DB record
    of type 'Download'.

    \list
        \li \a displayName  The name for Download which may be used by the UI displaying the download
        \li \a applicationIcon The application icon of the application created the download
        \li \a serviceIcon The service icon, which provides the file to be downloaded
        \li \a filePath The filePath e.g. url to the file to be downloaded
        \li \a mimeType the MimeType of the file to be downloaded
        \li \a expectedFileSize The file size of the file to be downloaded
        \li \a callback QStringList containing DBus callback information such as: service, path and interface
        \li \a cancelMethod The name of the cancel callback method, which DBus callback provides
        \li \a restartMethod The name of the restart callback method, which DBus callback provides
    \endlist

    This method returns the transfer id of the created Download transfer. Note that this method only
    creates an entry to the database. To start the actual transfer, the startTransfer() method must
    be called.

    \sa startTransfer(), restartTransfer(), finishTransfer(), updateTransferProgress()
 */
int TransferEngine::createDownload(const QString &displayName,
                                   const QString &applicationIcon,
                                   const QString &serviceIcon,
                                   const QString &filePath,
                                   const QString &mimeType,
                                   qlonglong expectedFileSize,
                                   const QStringList &callback,
                                   const QString &cancelMethod,
                                   const QString &restartMethod)
{
    Q_D(TransferEngine);
    QUrl url(filePath);
    QFileInfo fileInfo(filePath);

    MediaItem *mediaItem = new MediaItem();
    mediaItem->setValue(MediaItem::Url,             url);
    mediaItem->setValue(MediaItem::ResourceName,    fileInfo.fileName());
    mediaItem->setValue(MediaItem::MimeType,        mimeType);
    mediaItem->setValue(MediaItem::TransferType,    TransferEngineData::Download);
    mediaItem->setValue(MediaItem::FileSize,        expectedFileSize);
    mediaItem->setValue(MediaItem::DisplayName,     displayName);
    mediaItem->setValue(MediaItem::ApplicationIcon, applicationIcon);
    mediaItem->setValue(MediaItem::ServiceIcon,     serviceIcon);
    mediaItem->setValue(MediaItem::Callback,        callback);
    mediaItem->setValue(MediaItem::CancelCBMethod,  cancelMethod);
    mediaItem->setValue(MediaItem::RestartCBMethod, restartMethod);
    mediaItem->setValue(MediaItem::CancelSupported, !cancelMethod.isEmpty());
    mediaItem->setValue(MediaItem::RestartSupported,!restartMethod.isEmpty());

    const int key = DbManager::instance()->createTransferEntry(mediaItem);
    d->m_activityMonitor->newActivity(key);
    d->m_keyTypeCache.insert(key, TransferEngineData::Download);
    emit transfersChanged();
    emit statusChanged(key, TransferEngineData::NotStarted);    
    return key;
}

/*!
    DBus adaptor calls this method to create a Sync entry. Note that this is purely write-only
    method and doesn't involve anything else from TransferEngine side than creating a new DB record
    of type 'Download'.

    \list
        \li \a displayName  The name for download which may be used by the UI displaying the download
        \li \a applicationIcon The application icon of the application created the download
        \li \a serviceIcon The service icon, which provides the file to be downloaded
        \li \a callback QStringList containing DBus callback information such as: service, path and interface
        \li \a cancelMethod The name of the cancel callback method, which DBus callback provides
        \li \a restartMethod The name of the restart callback method, which DBus callback provides
    \endlist

    This method returns the transfer id of the created Download transfer. Note that this method only
    creates an entry to the database. To start the actual transfer, the startTransfer() method must
    be called.

    \sa startTransfer(), restartTransfer(), finishTransfer(), updateTransferProgress()
 */
int TransferEngine::createSync(const QString &displayName,
                               const QString &applicationIcon,
                               const QString &serviceIcon,
                               const QStringList &callback,
                               const QString &cancelMethod,
                               const QString &restartMethod)
{
    MediaItem *mediaItem = new MediaItem();
    mediaItem->setValue(MediaItem::TransferType,    TransferEngineData::Sync);
    mediaItem->setValue(MediaItem::DisplayName,     displayName);
    mediaItem->setValue(MediaItem::ApplicationIcon, applicationIcon);
    mediaItem->setValue(MediaItem::ServiceIcon,     serviceIcon);
    mediaItem->setValue(MediaItem::Callback,        callback);
    mediaItem->setValue(MediaItem::CancelCBMethod,  cancelMethod);
    mediaItem->setValue(MediaItem::RestartCBMethod, restartMethod);
    mediaItem->setValue(MediaItem::CancelSupported, !cancelMethod.isEmpty());
    mediaItem->setValue(MediaItem::RestartSupported,!restartMethod.isEmpty());

    const int key = DbManager::instance()->createTransferEntry(mediaItem);
    delete mediaItem;

    Q_D(TransferEngine);
    d->m_activityMonitor->newActivity(key);
    d->m_keyTypeCache.insert(key, TransferEngineData::Sync);
    emit transfersChanged();
    emit statusChanged(key, TransferEngineData::NotStarted);
    return key;
}

/*!
    DBus adaptor calls this method to start the actual transfer. This method changes the transfer
    status of the existing  transfer with a \a transferId to TransferEngineData::TransferStarted. This
    method can only be called for Sync and Download transfers.

    Calling this method causes the corresponding statusChanged() signal to be emitted.
*/
void TransferEngine::startTransfer(int transferId)
{
    Q_D(TransferEngine);
    d->exitSafely();

    TransferEngineData::TransferType type = d->transferType(transferId);
    if (type == TransferEngineData::Undefined) {
        qWarning() << "TransferEngine::startTransfer: failed to get transfer type";
        return;
    }

    if (type == TransferEngineData::Upload) {
        qWarning() << "TransferEngine::startTransfer: starting upload isn't supported";
        return;
    }

    TransferEngineData::TransferStatus status = DbManager::instance()->transferStatus(transferId);
    // First check if this is a new transfer
    if (status == TransferEngineData::NotStarted ||
        status == TransferEngineData::TransferCanceled ||
        status == TransferEngineData::TransferInterrupted) {
        d->m_activityMonitor->newActivity(transferId);
        DbManager::instance()->updateTransferStatus(transferId, TransferEngineData::TransferStarted);
        emit statusChanged(transferId, TransferEngineData::TransferStarted);
        emit activeTransfersChanged();
    } else {
        qWarning() << "TransferEngine::startTransfer: could not start transfer";
    }
}

/*!
    DBus adaptor calls this method to restart a canceled or failed transfer with a \a transferId. In
    a case of Upload, this method creates MediaItem instance of the existing transfer and instantiates
    the required share plugin. The MediaItem instance is passed to the plugin and sharing is restarted.

    For Sync and Download entries, this method calls their callbacks methods, if a callback interface
    has been defined by the client originally created the Sync or Download entry.
*/
void TransferEngine::restartTransfer(int transferId)
{
    Q_D(TransferEngine);
    d->exitSafely();

    TransferEngineData::TransferType type = d->transferType(transferId);
    if (type == TransferEngineData::Undefined) {
        qWarning() << "TransferEngine::restartTransfer: failed to get transfer type";
        return;
    }

    if (type == TransferEngineData::Upload) {

        MediaItem * item = DbManager::instance()->mediaItem(transferId);
        if (!item) {
            qWarning() << "TransferEngine::restartTransfer: failed to reload media item from db!";
            return;
        }

        Q_D(TransferEngine);
        MediaTransferInterface *muif = d->loadPlugin(item->value(MediaItem::PluginId).toString());
        muif->setMediaItem(item);

        connect(muif, SIGNAL(statusChanged(MediaTransferInterface::TransferStatus)),
                d, SLOT(uploadItemStatusChanged(MediaTransferInterface::TransferStatus)));
        connect(muif, SIGNAL(progressUpdated(qreal)),
                d, SLOT(updateProgress(qreal)));

        d->m_activityMonitor->newActivity(transferId);
        d->m_keyTypeCache.insert(transferId, TransferEngineData::Upload);
        d->m_plugins.insert(muif, transferId);
        muif->start();
        return;
    }

    TransferEngineData::TransferStatus status = DbManager::instance()->transferStatus(transferId);
    // Check if this is canceled or interrupted transfer
    // and make a callback call to the client. It's client's
    // responsibility to update states properly
    if (status == TransferEngineData::TransferCanceled ||
        status == TransferEngineData::TransferInterrupted) {
        DbManager::instance()->updateProgress(transferId, 0);
        d->m_activityMonitor->newActivity(transferId);
        d->callbackCall(transferId, TransferEnginePrivate::RestartCallback);
    }
}

/*!
    Finishes an existing Sync or Download transfer with a \a transferId. Transfer can be finished
    with different \a status e.g for successfully finish status can be set to
    TransferEngineData::TransferFinished, for canceling TransferEngineData::Canceled and for
    failure with TransferEngineData::TransferInterrupted. In a case of failure, the client can
    also provide a \a reason.

    This method causes statusChanged() signal to be emitted. If a sync has been successfully
    finished, then it will also be removed from the database automatically which causes
    transferChanged() signal to be emitted.
 */
void TransferEngine::finishTransfer(int transferId, int status, const QString &reason)
{
    Q_UNUSED(reason);
    Q_D(TransferEngine);
    d->exitSafely();

    TransferEngineData::TransferType type = d->transferType(transferId);
    if (type == TransferEngineData::Undefined || type == TransferEngineData::Upload) {
        return; // We don't handle plugins here
    }

    QString fileName;
    // Read the file path from the database for download
    if (type == TransferEngineData::Download) {
        MediaItem *mediaItem = DbManager::instance()->mediaItem(transferId);
        if (!mediaItem) {
            qWarning() << "TransferEngine::finishTransfer: Failed to fetch MediaItem";
            return;
        }
        fileName = d->mediaFileOrResourceName(mediaItem);
    }

    TransferEngineData::TransferStatus transferStatus = static_cast<TransferEngineData::TransferStatus>(status);
    if (transferStatus == TransferEngineData::TransferFinished ||
        transferStatus == TransferEngineData::TransferCanceled ||
        transferStatus == TransferEngineData::TransferInterrupted) {
        DbManager::instance()->updateTransferStatus(transferId, transferStatus);
        d->sendNotification(type, transferStatus, DbManager::instance()->transferProgress(transferId), fileName, transferId);
        d->m_activityMonitor->activityFinished(transferId);
        emit statusChanged(transferId, status);

        bool notify = false;

        // Clean up old failed syncs from the database and leave only the latest one there
        if (type == TransferEngineData::Sync) {
            if (DbManager::instance()->clearFailedTransfers(transferId, type)) {
                notify = true;
            }

            // We don't want to leave successfully finished syncs to populate the database, just remove it.
            if (transferStatus == TransferEngineData::TransferFinished) {
                if (DbManager::instance()->removeTransfer(transferId)) {
                    notify = true;
                }
            }
        }

        emit activeTransfersChanged(); // Assume that the transfer was active

        if (notify) {
            emit transfersChanged();
        }
    }
}

/*!
    DBus adaptor calls this method to update transfer progress of the transfer with a \a transferId and
    with a new \a progress.
*/
void TransferEngine::updateTransferProgress(int transferId, double progress)
{
    Q_D(TransferEngine);
    d->exitSafely();

    TransferEngineData::TransferType type = d->transferType(transferId);
    if (type != TransferEngineData::Download && type != TransferEngineData::Upload) {
        return;
    }

    MediaItem *mediaItem = DbManager::instance()->mediaItem(transferId);
    if (!mediaItem) {
        qWarning() << "TransferEngine::finishTransfer: Failed to fetch MediaItem";
        return;
    }
    QString fileName = d->mediaFileOrResourceName(mediaItem);

    int oldProgressPercentage = DbManager::instance()->transferProgress(transferId) * 100;
    if (DbManager::instance()->updateProgress(transferId, progress)) {
        d->m_activityMonitor->newActivity(transferId);
        emit progressChanged(transferId, progress);

        if (oldProgressPercentage != (progress * 100)) {
            d->sendNotification(type, DbManager::instance()->transferStatus(transferId), progress, fileName, transferId);
        }
    } else {
         qWarning() << "TransferEngine::updateTransferProgress: Failed to update progress for " << transferId;
    }
}

/*!
    DBus adaptor calls this method to fetch a list of transfers. This method returns QList<TransferDBRecord>.
 */
QList<TransferDBRecord> TransferEngine::transfers()
{    
    Q_D(TransferEngine);
    d->exitSafely();
    return DbManager::instance()->transfers();
}

/*!
    DBus adaptor calls this method to fetch a list of active transfers. This method returns QList<TransferDBRecord>.
 */
QList<TransferDBRecord> TransferEngine::activeTransfers()
{
    Q_D(TransferEngine);
    d->exitSafely();
    return DbManager::instance()->activeTransfers();
}

/*!
    DBus adaptor calls this method to fetch a list of transfer methods. This method returns QList<TransferMethodInfo>.

    Transfer methods are basically a list of share plugins installed to the system.
 */
QList <TransferMethodInfoDeprecated> TransferEngine::transferMethods()
{
    Q_D(TransferEngine);
    d->exitSafely();

    QList <TransferMethodInfo> newPlugins = d->enabledPlugins();
    QList <TransferMethodInfoDeprecated> oldPlugins;
    for (auto info : newPlugins) {
        TransferMethodInfoDeprecated oldInfo;
        oldInfo.displayName     = info.displayName;
        oldInfo.userName        = info.userName;
        oldInfo.methodId        = info.methodId;
        oldInfo.shareUIPath     = info.shareUIPath;
        oldInfo.capabilitities  = info.capabilitities;
        oldInfo.accountId       = info.accountId;
        oldPlugins << oldInfo;
    }
    return oldPlugins;
}

QList <TransferMethodInfo> TransferEngine::transferMethods2()
{
    Q_D(TransferEngine);
    d->exitSafely();
    return d->enabledPlugins();
}

/*!
    DBus adaptor calls this method to clear all the finished, canceled or interrupted transfers in the database.
 */
void TransferEngine::clearTransfers()
{    
    Q_D(TransferEngine);
    d->exitSafely();
    const int count = DbManager::instance()->transferCount();
    if (count > 0) {
        const int active = DbManager::instance()->activeTransferCount();
        if (DbManager::instance()->clearTransfers()) {
            if (active > 0) {
                emit activeTransfersChanged();
            }
            emit transfersChanged();
        }
    }
}

/*!
    DBus adaptor calls this method to remove a finished, canceled or interrupted transfer from the database.
 */
void TransferEngine::clearTransfer(int transferId)
{
    Q_D(TransferEngine);
    d->exitSafely();
    if (DbManager::instance()->clearTransfer(transferId)) {
        emit transfersChanged();
    }
}

/*!
    DBus adaptor calls this method to cancel an existing transfer with a \a transferId.

    If the transfer is Upload, then this method calls MediaTransferInterface instance's
    cancel method. In a case of Sync or Download this method calls client's cancel callback
    method, if the one exists.

    Calling this method causes statusChanged() signal to be emitted.
 */
void TransferEngine::cancelTransfer(int transferId)
{
    Q_D(TransferEngine);
    d->exitSafely();

    TransferEngineData::TransferType type = d->transferType(transferId);

    // Handle canceling of Download or Sync
    if (type == TransferEngineData::Download || type == TransferEngineData::Sync) {
        d->callbackCall(transferId, TransferEnginePrivate::CancelCallback);
        d->m_activityMonitor->activityFinished(transferId);
        return;
    }

    // Let plugin handle canceling of up upload
    if (type == TransferEngineData::Upload) {
        MediaTransferInterface *muif = d->m_plugins.key(transferId);
        if (muif == 0) {
            qWarning() << "TransferEngine::cancelTransfer: Failed to get MediaTransferInterface!";
            return;
        }
        d->m_activityMonitor->activityFinished(transferId);
        muif->cancel();
    }
}
/*!
    DBus adaptor calls this method to enable or disable transfer specific notifications
    based on \a enable argument.
*/
void TransferEngine::enableNotifications(bool enable)
{
    Q_D(TransferEngine);
    d->exitSafely();
    if (d->m_notificationsEnabled != enable) {
        d->m_notificationsEnabled = enable;
    }
}

/*!
    DBus adaptor calls this method.
    Returns true or false depending if notifications are enabled or disabled.
*/
bool TransferEngine::notificationsEnabled()
{
    Q_D(TransferEngine);
    d->exitSafely();
    return d->m_notificationsEnabled;
}
