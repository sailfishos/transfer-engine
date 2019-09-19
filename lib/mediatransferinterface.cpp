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

#include "mediatransferinterface.h"
#include "mediaitem.h"
#include <QtDebug>

// Update progress (to dbus) only in every 5% progress changes
#define PROGRESS_THRESHOLD 0.05

class MediaTransferInterfacePrivate
{
public:
    MediaTransferInterfacePrivate():
        m_mediaItem(0),
        m_status(MediaTransferInterface::NotStarted),
        m_progress(0),
        m_prevProgress(0)
    {}

    MediaItem *m_mediaItem;
    MediaTransferInterface::TransferStatus m_status;
    qreal m_progress;
    qreal m_prevProgress;
};




/*!
    \class MediaTransferInterface
    \brief The MediaTransferInterface class is responsible of implementing

    \ingroup transfer-engine-lib

    MediaTransferInterface is an abstract class which must be implemented by
    each share plugin. The subclass of this class is resposible of providing:

    \list
        \li Status information of the sharing
        \li Progress of the sharing
        \li Information, if cancel and restart actions are supported
        \li Starting and/or canceling the sharing
    \endlist


    The flow:
    The instance of this class is created when SailfishShare::start() method is
    called.
*/

/*!
    \enum MediaTransferInterface::TransferStatus

    Transfers status

    \value NotStarted            Transfer not started
    \value TransferStarted       Transfer is ongoing
    \value TransferCanceled      Transfer is canceled usually via user actions
    \value TransferFinished      Transfer is finished successfully
    \value TransferInterrupted   Transfer failed
*/

/*!
    \fn virtual QString MediaTransferInterface::displayName() const = 0

    Subclass must implement this method. Return a human readable display
    name for the service e.g. "Facebook".
 */

/*!
    \fn virtual QUrl MediaTransferInterface::serviceIcon() const = 0

    Subclass must implement this method. Return the URL to the service
    icon. Service icon can be e.g. Facebook or BT icon.

 */


/*!
    \fn virtual bool MediaTransferInterface::cancelEnabled() const = 0

    Subclass must implement this method. Return true if cancel is supported
    or false otherwise.
 */

/*!
    \fn virtual bool MediaTransferInterface::restartEnabled() const = 0

    Subclass must implement this method. Return true if restart is supported
    or false otherwise.
 */

/*!
    \fn virtual void MediaTransferInterface::start() = 0

    Subclass must implement this to start the actual sharing operation. This
    same method is called when sharing is restarted.
 */

/*!
    \fn virtual void MediaTransferInterface::cancel() = 0

    Subclass must implement this to cancel the ongoing share operation.
*/


/*!
    \fn void MediaTransferInterface::statusChanged(MediaTransferInterface::TransferStatus status)

    This signal with changed \a status is emitted when setStatus() is called.

    Note that this signal is NOT meant to be emitted by the subclass. Instead of
    emitting it manually the subclass should use setStatus() method, which causes
    this signal to be emitted automatically.
*/

/*!
    \fn void MediaTransferInterface::progressUpdated(qreal progress)

    This signal is emitted when the \a progress changes. Progress is changed when the
    subclass calls setProgress() method.

    Note that, like the statusChanged() signal, also this signal is not to meant to be
    emitted by the subclass. It's emitted automatically when subclass calls setProgress().
*/



/*!
    Construct MediaTransferInterface object with optional \a parent object.
 */
MediaTransferInterface::MediaTransferInterface(QObject *parent):
    QObject(parent),
    d_ptr(new MediaTransferInterfacePrivate)
{
}

MediaTransferInterface::~MediaTransferInterface()
{
    delete d_ptr;
    d_ptr = 0;
}

/*!
    Set MediaItem for this object. MediaItem is a container for all the information related
    to the \a mediaItem (image, video, vcard,..) which is going to be shared. This information
    must contain e.g. url to the media item. For more details all the available values,
    see MediaItem::ValueKey.

    NOTE: That the values set to this media item depends on the related Share UI. For example
    BT share UI won't provide information related to the accounts because "accountId" or "description"
    are not required for the Bluetooh. But then again Facebook share UI provides this information.
    This means that because plugin provides the Share UI, it knows which fields are set for the
    MediaItem instance.

    MediaItem is passed to this item by TransferEngine.

 */
void MediaTransferInterface::setMediaItem(MediaItem *mediaItem)
{
    Q_D(MediaTransferInterface);
    if (d->m_mediaItem) {
        qWarning() << "MediaTransferInterface::setMediaItem: MediaItem is not null. Old media item will be deleted!";
        delete d->m_mediaItem;
    }
    d->m_mediaItem = mediaItem;
    d->m_mediaItem->setParent(this);
}

/*!
    Return the MediaItem instance or 0, if it hasn't been set.
 */
MediaItem *MediaTransferInterface::mediaItem()
{
    Q_D(MediaTransferInterface);
    return d->m_mediaItem;
}


/*!
    Returns the status of the media transfer.
*/
MediaTransferInterface::TransferStatus MediaTransferInterface::status() const
{
    Q_D(const MediaTransferInterface);
    return d->m_status;
}

/*!
    Returns the current progress of the ongoing media transfer.
*/
qreal MediaTransferInterface::progress() const
{
    Q_D(const MediaTransferInterface);
    return d->m_progress;
}


/*!
    Sets the \a status for the media transfer. Note that this method also
    sets the progress if status changes to MediaTransferInterface::TransferFinished.
    If status changes to MediaTransferInterface::TransferCanceled or
    MediaTransferInterface::TransferInterrupted, the progress is set to 0.

    This method emits statusChanged() and progressUpdated() signals automatically based
    on status changes and if this method marks progress to 1 or to 0.
*/
void MediaTransferInterface::setStatus(TransferStatus status)
{
    Q_D(MediaTransferInterface);

    // Make sure that progress is 1 if we are really finished
    if (status == MediaTransferInterface::TransferFinished &&
        d->m_prevProgress < 1 ) {
        d->m_progress = 1;
        d->m_prevProgress = 1;
        emit progressUpdated(1);
    }

    // Make sure that progress is set to 0 if transfer is canceled or
    // interrupted
    if (status == MediaTransferInterface::TransferCanceled ||
        status == MediaTransferInterface::TransferInterrupted) {
        d->m_progress = 0;
        d->m_prevProgress = 0;
        emit progressUpdated(0);
    }

    // And update the status
    if (d->m_status != status) {
        d->m_status = status;
        emit statusChanged(d->m_status);
    }

}

/*!
    Set the \a progress for the on going media transfer.

    Note that progressUpdated() signal might not be emitted in every call of this method
    because it might pollute dbus if progress changes too many times.
 */
void MediaTransferInterface::setProgress(qreal progress)
{
    Q_D(MediaTransferInterface);
    if (progress < 0 || 1 < progress) {
        qWarning() << "MediaTransferInterface::setProgress: progress must be in between 0 and 1";
        // Just show the warning and let the progress update. Sometimes there might be some
        // decimals which may not match exactly with 1, like 1.0001
    }

    if (d->m_progress == progress) {
        return;
    }

    d->m_progress = progress;

    // To avoid dbus overload let's not emit this signal in every progress change
    if (qAbs(d->m_progress - d->m_prevProgress) >= PROGRESS_THRESHOLD) {
        emit progressUpdated(progress);
        d->m_prevProgress = progress;
    }
}


