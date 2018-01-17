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

#ifndef TRANSFERDBRECORD_H
#define TRANSFERDBRECORD_H

#include <QtGlobal>
#include <QDBusArgument>

class TransferDBRecord
{
public:
    enum TransferDBRecordField {
        TransferID = 0,
        TransferType,
        Progress,
        URL,
        Status,
        PluginID,
        Timestamp,
        DisplayName,
        ResourceName,
        MimeType,
        FileSize,
        ServiceIcon,
        ApplicationIcon,
        ThumbnailIcon,
        CancelSupported,
        RestartSupported
    };

    TransferDBRecord();
    TransferDBRecord &operator=(const TransferDBRecord &other);
    TransferDBRecord(const TransferDBRecord &other);
    ~TransferDBRecord();


    friend QDBusArgument &operator<<(QDBusArgument &argument, const TransferDBRecord &record);
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, TransferDBRecord &record);

    static void registerType();

    QVariant value(int index) const;

    bool isValid() const;

    friend bool operator ==(const TransferDBRecord &left, const TransferDBRecord &right);
    friend bool operator !=(const TransferDBRecord &left, const TransferDBRecord &right);


// TODO: Maybe this could use QVariantList internally to ease of pain of keeping thigs up to date
//       when database structure / fields change
    int     transfer_id;
    int     transfer_type;
    int     status;
    qint64  size;
    double  progress;
    QString plugin_id;
    QString url;
    QString timestamp;
    QString display_name;
    QString resource_name;
    QString mime_type;
    QString service_icon;
    QString application_icon;
    QString thumbnail_icon;
    bool    cancel_supported;
    bool    restart_supported;
};

bool operator ==(const TransferDBRecord &left, const TransferDBRecord &right);
bool operator !=(const TransferDBRecord &left, const TransferDBRecord &right);



#endif // DBUSTYPES_H
