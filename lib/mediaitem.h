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

#ifndef MEDIAITEM_H
#define MEDIAITEM_H
#include <QObject>
#include <QUrl>

// Originally this was meant to be exposed to QML side, but not anymore.
class MediaItemPrivate;

class MediaItem: public QObject
{
public:
    enum ValueKey {
        TransferType,
        Timestamp,
        Status,
        DisplayName,
        Url,
        ContentData,
        ResourceName,
        MimeType,
        FileSize,
        PluginId,
        MetadataStripped,
        ScalePercent,
        Title,
        Description,
        ServiceIcon,
        ApplicationIcon,
        ThumbnailIcon,
        AccountId,
        UserData,

        // Callback methods comes from the API, not from the plugins
        Callback,
        CancelCBMethod,
        RestartCBMethod,

        // These come from plugins.
        CancelSupported,
        RestartSupported

    };

    explicit MediaItem(QObject *parent = 0);
    ~MediaItem();

    QVariant value(ValueKey key) const;

protected:
    void setValue(ValueKey key, const QVariant &value);


private:
    MediaItemPrivate *d_ptr = nullptr;
    Q_DECLARE_PRIVATE(MediaItem)
    friend class TransferEngine;
    friend class TransferEnginePrivate;
    friend class DbManager;
    friend class ut_mediatransferinterface;
};

#endif // MEDIAITEM_H
