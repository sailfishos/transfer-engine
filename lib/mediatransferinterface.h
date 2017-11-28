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

#ifndef MEDIATRANSFERINTERFACE_H
#define MEDIATRANSFERINTERFACE_H
#include <QObject>
#include <QUrl>
#include "transfertypes.h"


class MediaItem;
class MediaTransferInterfacePrivate;

class MediaTransferInterface: public QObject
{
    Q_OBJECT
public:

    enum TransferStatus {
        NotStarted            = TransferEngineData::NotStarted,
        TransferStarted       = TransferEngineData::TransferStarted,
        TransferCanceled      = TransferEngineData::TransferCanceled,
        TransferFinished      = TransferEngineData::TransferFinished,
        TransferInterrupted   = TransferEngineData::TransferInterrupted
    };


    explicit MediaTransferInterface(QObject *parent = 0);
    virtual ~MediaTransferInterface();

    MediaItem *mediaItem();

    virtual QString displayName() const = 0;
    virtual QUrl serviceIcon() const = 0;
    virtual bool cancelEnabled() const = 0;
    virtual bool restartEnabled() const = 0;

    MediaTransferInterface::TransferStatus status() const;
    qreal progress() const;

protected:
    void setMediaItem(MediaItem *mediaItem);
    void setStatus(MediaTransferInterface::TransferStatus status);
    void setProgress(qreal progress);

public Q_SLOTS:
    virtual void start() = 0;
    virtual void cancel() = 0;

Q_SIGNALS:
    void statusChanged(MediaTransferInterface::TransferStatus status);
    void progressUpdated(qreal progress);

private:
    MediaTransferInterfacePrivate *d_ptr;
    Q_DECLARE_PRIVATE(MediaTransferInterface)
    friend class TransferEngine;
    friend class TransferEnginePrivate;
    friend class DbManager;
};

#endif // MEDIATRANSFERINTERFACE_H
