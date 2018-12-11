/****************************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
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

#include "mediaitem.h"
#include <QtDebug>

class MediaItemPrivate
{
public:
    QMap <MediaItem::ValueKey, QVariant> m_values;
};

/*!
    \class MediaItem
    \brief The MediaItem class is a container for all the media item related data needed for sharing.

    \ingroup transfer-engine-lib

    MediaItem is used internally by Nemo Transfer Engine, but MediaItem object instance is
    passed to each share plugin via MediaTransferInterface and it is meant to be a read-only object.

    MediaItem can be accessed via MediaTransferInterface::mediaItem() method. MediaItem stores internal
    data as key-value pairs. Data can be accessed via value() method using pre-defined ValueKey keys.

    Note: Depending on the use case (sharing, download, sync) MediaItem may not contain data for all
    the keys. For example when MediaItem is created for the sync event, it doesn't contain Url or
    MimeType values or if plugin doesn't require account, then AccountId won't be defined for this
    MediaItem.

    Depending on the share UI which plugins provides, the content of the MediaItem varies. Bluetooth or
    NFC sharing doesn't need account or descriptio, but then again Facebook sharing needs that information.

    \sa value() MediaTransferInterface::MediaItem()
 */

/*!
    \enum MediaItem::ValueKey

    The following enum values can be used for accessing MediaItem data. Modifying MediaItem data is not
    allowed for the plugins.

    \value TransferType     Type of the transfer
    \value Timestamp        Time stamp when event has been created
    \value Status           Transfer status
    \value DisplayName      Name of the share plugin or for sync and download event
    \value Url              URL of the media item to be shared or downloaded
    \value MimeType         Mimetype of the media item to be shared or downloaded
    \value FileSize         Filesize of the media item
    \value PluginId         Id of the share plugin
    \value MetadataStripped Flag to indicate if metadata should be stripped or not
    \value Title            Title for the media item to be transfered
    \value Description      Description for the media item to be transfered
    \value ServiceIcon      Service icon URL e.g. email service
    \value ApplicationIcon  Application icon url
    \value AccountId,       Account Id
    \value UserData         User specific data that is passed from the UI
    \value Callback         Callback service, path, interface packed to QStringList
    \value CancelCBMethod   Cancel callback method name
    \value RestartCBMethod  Restart callback method name
    \value CancelSupported  Bool to indicate if cancel is supported by the share plugin
    \value RestartSupported Bool to indicate if restart is supported by the share plugin
*/

/*!
    Create MediaItem object.
 */
MediaItem::MediaItem(QObject *parent):
    QObject(parent),
    d_ptr(new MediaItemPrivate)
{
}

MediaItem::~MediaItem()
{
    delete d_ptr;
    d_ptr = 0;
}

/*!
    Set \a value for the \a key.
 */
void MediaItem::setValue(ValueKey key, const QVariant &value)
{
    Q_D(MediaItem);
    d->m_values.insert(key, value);
}

/*!
    Returns the value of the \a key.
*/
QVariant MediaItem::value(ValueKey key) const
{
    Q_D(const MediaItem);
    if (!d->m_values.contains(key)) {
        return QVariant();
    }
    return d->m_values.value(key);
}
