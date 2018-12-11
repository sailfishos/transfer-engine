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

#include "transferdbrecord.h"
#include "metatypedeclarations.h"

#include <QtDBus>

/*!
    \class TransferDBRecord
    \brief The TransferDBRecord class is a wrapper class for TransferEngine DBus message.

    \ingroup transfer-engine-lib

    This class wraps transfer method related information and is used to pass that information
    over the DBus to the any client who is interested in about it.

    A single instance of TransferDBRecord contains information of a one transfer record in TransferEngine
    database. Usually clients don't need to fill any data to this class, instead they can request a list
    of TransferDBRecords from the TransferEngine and access its data via value() method.
 */

/*!
    \enum TransferDBRecord::TransferDBRecordField

    An enum for accessing TrasnferDBRecord values using value() method

    \value TransferID Id of the transfer
    \value TransferType Type of the transfer (Upload, Download, Sync
    \value Progress Progress of the transfer
    \value URL Url of the media related to the transfer
    \value Status The status of the transfer
    \value PluginID The id of the plugin which is handling the transfer
    \value Timestamp The timestamp for the transfer
    \value DisplayName The display name for the transfer
    \value ResourceName The name of the resource
    \value MimeType MimeType information of the media being transfered
    \value FileSize File size of the media being transfered
    \value ServiceIcon Icon url for a service related to the transfer
    \value ApplicationIcon Application icon url which has created e.g. sync or download transfer
    \value ThumbnailIcon Thumbnail url
    \value CancelSupported Boolean to indicate if cancel is supported
    \value RestartSupported Boolean to indicate if cancel is supported
*/

TransferDBRecord::TransferDBRecord()
{
}

/*!
    Assigns \a other to this transfer db record and returns a reference to this transfer db record.
 */
TransferDBRecord &TransferDBRecord::operator=(const TransferDBRecord &other)
{
    transfer_id     = other.transfer_id;
    transfer_type   = other.transfer_type;
    status          = other.status;
    size            = other.size;
    progress        = other.progress;
    plugin_id       = other.plugin_id;
    url             = other.url;
    timestamp       = other.timestamp;
    display_name    = other.display_name;
    resource_name   = other.resource_name;
    mime_type       = other.mime_type;
    service_icon    = other.service_icon;
    application_icon= other.application_icon;
    thumbnail_icon  = other.thumbnail_icon;
    cancel_supported = other.cancel_supported;
    restart_supported = other.restart_supported;
    return *this;
}

/*!
    Constructs a copy of \a other.
 */
TransferDBRecord::TransferDBRecord(const TransferDBRecord &other):
    transfer_id(other.transfer_id),
    transfer_type(other.transfer_type),
    status(other.status),
    size(other.size),
    progress(other.progress),
    plugin_id(other.plugin_id),
    url(other.url),
    timestamp(other.timestamp),
    display_name(other.display_name),
    resource_name(other.resource_name),
    mime_type(other.mime_type),
    service_icon(other.service_icon),
    application_icon(other.application_icon),
    thumbnail_icon(other.thumbnail_icon),
    cancel_supported(other.cancel_supported),
    restart_supported(other.restart_supported)
{
}

TransferDBRecord::~TransferDBRecord()
{
}

/*!
    Writes the given \a record to specified \a argument.
*/
QDBusArgument &operator<<(QDBusArgument &argument, const TransferDBRecord &record)
{
    argument.beginStructure();

    argument << record.transfer_id
             << record.transfer_type
             << record.status
             << record.size
             << record.progress
             << record.plugin_id
             << record.url
             << record.timestamp
             << record.display_name
             << record.resource_name
             << record.mime_type
             << record.service_icon
             << record.application_icon
             << record.thumbnail_icon
             << record.cancel_supported
             << record.restart_supported;
    argument.endStructure();
    return argument;
}


/*!
    Reads the given \a argument and stores it to the specified \a record.
*/
const QDBusArgument &operator>>(const QDBusArgument &argument, TransferDBRecord &record)
{
    argument.beginStructure();
    argument >> record.transfer_id
             >> record.transfer_type
             >> record.status
             >> record.size
             >> record.progress
             >> record.plugin_id
             >> record.url
             >> record.timestamp
             >> record.display_name
             >> record.resource_name
             >> record.mime_type
             >> record.service_icon
             >> record.application_icon
             >> record.thumbnail_icon
             >> record.cancel_supported
             >> record.restart_supported;
    argument.endStructure();
    return argument;
}

/*!
    Registers TransferDBRecord and QList<TransferDBRecord> as DBus types.
 */
void TransferDBRecord::registerType()
{
    qDBusRegisterMetaType<TransferDBRecord>();
    qDBusRegisterMetaType<QList<TransferDBRecord> >();
}

/*!
    Returns any of the TransferDBRecord values based on the index. As an \a index it's recommended to use
    TransferDBRecord::TransferDBRecordField enum.
*/
QVariant TransferDBRecord::value(int index) const
{
    switch(index) {
    case TransferID:
        return transfer_id;

    case TransferType:
        return transfer_type;

    case Progress:
        return progress;

    case URL:
        return url;

    case Status:
        return status;

    case PluginID:
        return plugin_id;

    case Timestamp:
        return timestamp;

    case DisplayName:
        return display_name;

    case ResourceName:
        return resource_name;

    case MimeType:
        return mime_type;

    case FileSize:
        return size;

    case ServiceIcon:
        return service_icon;

    case ApplicationIcon:
        return application_icon;

    case ThumbnailIcon:
        return thumbnail_icon;

    case CancelSupported:
        return cancel_supported;

    case  RestartSupported:
        return restart_supported;

    default:
        qWarning() << Q_FUNC_INFO << "Unknown index: " << index;
        return QVariant();
    }
}

bool TransferDBRecord::isValid() const
{
    return transfer_id > 0 && transfer_type > 0;
}

bool operator ==(const TransferDBRecord &left, const TransferDBRecord &right)
{
    return left.transfer_id == right.transfer_id &&
           left.transfer_type == right.transfer_type &&
           left.status == right.status &&
           left.progress == right.progress;
}

bool operator !=(const TransferDBRecord &left, const TransferDBRecord &right)
{
    return left.transfer_id != right.transfer_id ||
            left.transfer_type != right.transfer_type ||
            left.status != right.status ||
            left.progress != right.progress;
}

