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

#include "mediaitem.h"

#include <QtDebug>
#include <QDBusUnixFileDescriptor>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QBuffer>

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

/*!
    Returns true if a value exists for \a key.
*/
bool MediaItem::hasValue(ValueKey key) const
{
    Q_D(const MediaItem);
    return d->m_values.contains(key);
}

/*!
    Returns an I/O device to read the data for this media item.

    If any of the \a dataKeys are found in this item, this returns an I/O device to read the data
    for this key. Supported keys are:

    \list
    \li FileDescriptor
    \li ContentData
    \li Url
    \endlist

    If \a dataKeys is empty, this searches all of the supported keys.

    If there are multiple key matches (i.e. the media item has multiple data sources) the I/O
    device will only read the data for the first key that is matched.

    The caller takes ownership of the returned object, which will have its parent set to the
    specified \a parent.
*/
QIODevice *MediaItem::newIODevice(QObject *parent, const QList<MediaItem::ValueKey> &dataKeys) const
{
    QList<MediaItem::ValueKey> keysToSearch = dataKeys;
    if (keysToSearch.isEmpty()) {
        keysToSearch << FileDescriptor << ContentData << Url;
    }

    for (const MediaItem::ValueKey &dataKey : keysToSearch) {
        if (!hasValue(dataKey)) {
            continue;
        }
        switch (int(dataKey)) {
        case MediaItem::FileDescriptor:
        {
            const QVariant value = this->value(MediaItem::FileDescriptor);
            const int fd = value.type() == QVariant::Int
                    ? value.toInt()
                    : value.value<QDBusUnixFileDescriptor>().fileDescriptor();
            if (fd <= 0) {
                qWarning() << Q_FUNC_INFO << "unable to read fileDescriptor value:" << value;
            }
            QFile *file = new QFile(parent);
            if (!file->open(fd, QFile::ReadOnly)) {
                qWarning() << Q_FUNC_INFO << "unable to open fd!" << fd;
                return nullptr;
            }
            return file;
        }
        case MediaItem::ContentData:
        {
            QBuffer *buffer = new QBuffer(parent);
            buffer->setData(value(MediaItem::ContentData).toByteArray());
            if (!buffer->open(QFile::ReadOnly)) {
                qWarning() << Q_FUNC_INFO << "unable to open content buffer!";
                return nullptr;
            }
            return buffer;
        }
        case MediaItem::Url:
        {
            const QUrl url = value(MediaItem::Url).toUrl();
            QFile *file = new QFile(url.isLocalFile() ? url.toLocalFile() : url.toString(), parent);
            if (!file->open(QFile::ReadOnly)) {
                qWarning() << Q_FUNC_INFO << "unable to open file!" << file->fileName();
                return nullptr;
            }
            return file;
        }
        }
    }

    return nullptr;
}

/*!
    Returns a temporary file path based on the item's resource name.

    The directory path is guaranteed to be unique.
*/
QString MediaItem::temporaryFilePath() const
{
    static const QDir privilegedDir(
            QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
            + QLatin1String("/.local/share/system/privileged/Transfers/"));

    const QString resourceName = value(MediaItem::ResourceName).toString();
    if (resourceName.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "failed, no ResourceName set!";
        return QString();
    }

    QString filePath;
    if (privilegedDir.mkpath(".")) {
        QTemporaryDir tempDir(privilegedDir.absoluteFilePath(QStringLiteral("%1_XXXXXX").arg(qAppName())));
        tempDir.setAutoRemove(false);
        if (tempDir.isValid()) {
            filePath = tempDir.path() + QDir::separator() + resourceName;
        }
    }
    if (filePath.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Unable to generate file path:" << filePath;
    }
    return filePath;
}
