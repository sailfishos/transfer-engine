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

#include "transferengineclient.h"
#include "transferengineinterface.h"


class CallbackInterfacePrivate {
public:
    CallbackInterfacePrivate()
    {
    }

    CallbackInterfacePrivate(const QString &server, const QString &path, const QString &interface,
                             const QString &cancelMethod, const QString &restartMethod)
    {
        callback << server << path << interface;
        m_cancelMethod = cancelMethod;
        m_restartMethod = restartMethod;
    }

    QStringList callback;
    QString m_cancelMethod;
    QString m_restartMethod;
};


/*!
    \class CallbackInterface
    \brief The CallbackInterface class is a convenience class to wrap DBus callback infromation for
           TransferEngineClient API.

    \ingroup transfer-engine-lib

    This class should be used with TransferEngineClient API.

    NOTE: If creating an instance of this class to the heap, the caller is responsible of freeing
    the allocated memory.

    \sa TransferEngineClient::createSyncEvent() TransferEngineClient::createDownloadEvent()
 */


/*!
    Construct an empty CallbackInterface. This can be used if client doesn't want to provide callback
    interface to the Sync or Download events.
 */
CallbackInterface::CallbackInterface()
    : d_ptr(new CallbackInterfacePrivate())
{
}


/*!
    Construct CallbackInterface instance to provide callback information to the TransferEngineClient. Setup
    arguments as:
    \list
        \li \a server  e.g. "com.jolla.myapp"
        \li \a path   e.g. "/com/jolla/myapp"
        \li \a interface  e.g. "com.jolla.myapp"
        \li \a cancelMethod   Cancel method name e.g. "cancelSync"
        \li \a restartMethod Restart method name e.g. "restartSync"
    \endlist
 */
CallbackInterface::CallbackInterface(const QString &server, const QString &path, const QString &interface,
                  const QString &cancelMethod, const QString &restartMethod)
    : d_ptr(new CallbackInterfacePrivate(server, path, interface, cancelMethod, restartMethod))
{
}

CallbackInterface::~CallbackInterface()
{
    delete d_ptr;
}


class TransferEngineClientPrivate
{
public:
    TransferEngineInterface *m_client;    
};

/*!
    \class TransferEngineClient
    \brief The TransferEngineClient class is a simple client API for creating Download and
    Sync events to TransferEngine.

    TransferEngineClient is a convenience API for create and updating Download and
    Sync entries to the Nemo TransferEngine. For Upload entries, it is required to
    create share plugins by implementing required interfaces provided by this TransferEngine
    library.

    If the client using this interface wants to enable cancel and restart functionality, it
    means that the client process must provide DBus API, which Nemo Transfer Engine can call
    in a case of canceling or restarting the operation. In order to do that client must provide
    CallbackInterface object with properly set dbus information. See the example below.

    For share plugin implementation:
    \sa TransferPluginInterface, MediaTransferInterface, TransferPluginInfo

    To use this API to create e.g. Sync entry see the example below:

    \code

    // Create the instance of the client
    TransferEngineClient *client = new TransferEngineClient(this);

    // Setup callback information. This is dbus interface
    CallbackInterface callback("com.jolla.myapp", "/com/jolla/myapp", "com.jolla.myapp",
                               "cancel", "restart");

    // Create the sync event
    int transferId = client->createSyncEvent("Syncing data from my service",
                                           QUrl("image://theme/icon-launcher-my-app"),
                                           QUrl("image://theme/icon-s-service-icon"),
                                           callback);

    // Start the actual transfer i.e. sync
    client->startTransfer(transferId)

    // Update sync progress. Usually this is done e.g. in a slot which receives sync
    // progress from a signal.
    bool ok;
    qreal progress = 0;
    while (progress <= 1) {
    client->updateProgress(transferId, progress);
    progress = getProgressFoo(&ok);

    if (!ok)
        break;
    }

    // End the sync
    TransferEngineClient status;
    QString reason;
    if (ok) {
     status = TransferEngineClient::TransferFinished;
    } else {
     status = TransferEngineClient::TransferInterrupted;
     reason = "Something went wrong";
    }

    client->finishTransfer(transferId, status, reason);

    \endcode
*/
/*!
    \enum TransferEngineClient::Status

    This enum type describes different values for the Status.

    \value TransferFinished Transfer finished successfully
    \value TransferCanceled Transfer canceled usually due user actions
    \value TransferInterrupted Transfer interrupted because of an error
*/

/*!
    Construct an instance of TransferEngineClient with optional \a parent argument.
 */
TransferEngineClient::TransferEngineClient(QObject *parent) :
    QObject(parent),
    d_ptr(new TransferEngineClientPrivate)
{
    Q_D(TransferEngineClient);
    d->m_client = new TransferEngineInterface("org.nemo.transferengine",
                                              "/org/nemo/transferengine",
                                              QDBusConnection::sessionBus(),
                                              this);
}

TransferEngineClient::~TransferEngineClient()
{
    Q_D(TransferEngineClient);
    delete d->m_client;
    delete d_ptr;
    d_ptr = 0;
}

/*!
    Creates a download event to the TransferEngine. This method requires the following parameters
    \a displayName, a human readable name for the entry. \a applicationIcon is the \c QUrl to the icon
    of the application, who's calling this method. Usually it can be in format "image://theme/icon-s-something".
    \a serviceIcon is a service specific icon such as DropBox. \a url is the url to the media to be downloaded.
    \a mimeType is the mimeType of the media and \a expectedFileSize is the file size of the file to be downloaded.

    Client can define callback functions for canceling and restarting download. For that client can provide
    optional parameter \a callback, which is \c CallbackInterface object containing information about the dbus
    service.

    Returns transfer id of the download event.

    Create a download event to the TransferEngine. This only creates and entry, and client
    needs still call:
    \list
        \li \l TransferEngineClient::startTransfer()
        \li \l TransferEngineClient::updateTransferProgress()
        \li \l TransferEngineClient::finishTransfer()
    \endlist

    \sa createSyncEvent(), startTransfer(), updateTransferProgress(), finishTransfer()
 */
int TransferEngineClient::createDownloadEvent(const QString &displayName,
                                                      const QUrl &applicationIcon,
                                                      const QUrl &serviceIcon,
                                                      const QUrl &url,
                                                      const QString &mimeType,
                                                      qlonglong expectedFileSize,
                                                      const CallbackInterface &callback)
{
    Q_D(const TransferEngineClient);
    QDBusPendingReply<int> reply = d->m_client->createDownload(displayName,
                                                               applicationIcon.toString(),
                                                               serviceIcon.toString(),
                                                               url.toString(),
                                                               mimeType,
                                                               expectedFileSize,
                                                               callback.d_func()->callback,
                                                               callback.d_func()->m_cancelMethod,
                                                               callback.d_func()->m_restartMethod);
    reply.waitForFinished();

    if (reply.isError()) {
        qWarning() << "TransferEngineClient::createDownloadEvent: failed to get transfer ID!";
        return false;
    }

    return reply.value();
}

/*!
    Create a sync event to the TransferEngine. This method is very similar to createDownload(),
    except createSyncEvent() takes less parameters.

    \a displayName, a human readable name for the entry. \a applicationIcon is the \c QUrl to the icon
    of the application, who's calling this method. Usually it can be in format "image://theme/icon-s-something".
    \a serviceIcon is a service specific icon such as email account.

    Client can define callback functions for canceling and restarting sync. For that client can provide
    optional parameter \a callback, which is \c CallbackInterface object containing information about the dbus
    service.

    Returns transfer id of the sync event.

    \sa startTransfer(), updateTransferProgress(), finishTransfer()
 */
int TransferEngineClient::createSyncEvent(const QString &displayName,
                                                  const QUrl &applicationIcon,
                                                  const QUrl &serviceIcon,
                                                  const CallbackInterface &callback)
{
    Q_D(const TransferEngineClient);
    QDBusPendingReply<int> reply = d->m_client->createSync(displayName,
                                                           applicationIcon.toString(),
                                                           serviceIcon.toString(),
                                                           callback.d_func()->callback,
                                                           callback.d_func()->m_cancelMethod,
                                                           callback.d_func()->m_restartMethod);
    reply.waitForFinished();

    if (reply.isError()) {
        qWarning() << "TransferEngineClient::createSyncEvent: failed to get transfer ID!";
        return false;
    }

    return reply.value();
}

/*!
    Start the transfer for the existing transfer entry with \a transferId. This changes the status of the
    transfer from idle to started.These status changes are handled by Nemo TransferEngine internally, but
    after this method has been called, the transfer is possible to cancel, interrupt or finish.
 */
void TransferEngineClient::startTransfer(int transferId)
{
    Q_D(const TransferEngineClient);
    d->m_client->startTransfer(transferId);
}

/*!
    Update the progress of the existing transfer with \a transferId. The \a progress must be a \c qreal value
    between 0 to 1.

 */
void TransferEngineClient::updateTransferProgress(int transferId, qreal progress)
{
    if (progress < 0 || 1 < progress) {
        qWarning() << Q_FUNC_INFO << "Progress must be between 0 and 1!";
        return;
    }
    Q_D(const TransferEngineClient);
    d->m_client->updateTransferProgress(transferId, progress);
}

/*!
    Finalize the transfer with \a transferId. There are three options for finalizing the transfer by setting
    the \a status parameter value:
    \list
        \li \l TransferEngineClient::TransferFinished - success
        \li \l TransferEngineClient::TransferCanceled - user cancelation
        \li \l TransferEngineClient::TransferInterrupted - an error
    \endlist

    If the client wants to provide reason for finishing the transfer, it's possible to provide \a reason
    parameter.
 */
void TransferEngineClient::finishTransfer(int transferId, Status status, const QString &reason)
{
    Q_D(const TransferEngineClient);
    d->m_client->finishTransfer(transferId, static_cast<int>(status), reason);
}

/*!
    Private method for QML interface to cancel an ongoing transfer using \a transferId.
 */
void TransferEngineClient::cbCancelTransfer(int transferId)
{
    Q_D(TransferEngineClient);
    d->m_client->cancelTransfer(transferId);
}

/*!
    Private method for QML interface to restart canceled or interrupted transfer using \a transferId.
 */
void TransferEngineClient::cbRestartTransfer(int transferId)
{
    Q_D(TransferEngineClient);
    d->m_client->restartTransfer(transferId);
}

/*!
    Private method for QML interface to clear all canceled or interrupted events.
 */
void TransferEngineClient::clearTransfers()
{
    Q_D(TransferEngineClient);
    d->m_client->clearTransfers();
}

/*!
    Private method for QML interface to clear a finished, canceled or interrupted event.
 */
void TransferEngineClient::clearTransfer(int transferId)
{
    Q_D(TransferEngineClient);
    d->m_client->clearTransfer(transferId);
}

/*!
    Private method for QML interface to enable notifications.
 */
void TransferEngineClient::enableNotifications(bool enable)
{
    Q_D(TransferEngineClient);
    d->m_client->enableNotifications(enable);
}


/*!
    Private method for QML interface.

    Returns true if notifications are enabled, otherwise false is returned.
 */
bool TransferEngineClient::notificationsEnabled() const
{
    Q_D(const TransferEngineClient);
    QDBusPendingReply<bool> reply = d->m_client->notificationsEnabled();
    reply.waitForFinished();

    if (reply.isError()) {
        qWarning() << Q_FUNC_INFO << "failed to get notifications!";
        return false;
    }

    return reply.value();
}
