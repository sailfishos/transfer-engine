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

#ifndef TRANSFERENGINE_H
#define TRANSFERENGINE_H

#include <QObject>
#include <QMap>
#include <QVariantList>

#include "mediatransferinterface.h"
#include "transferdbrecord.h"
#include "transfermethodinfo.h"

class MediaTransferInterface;
class TransferEnginePrivate;
class TransferEngine : public QObject
{
    Q_OBJECT
public:

    explicit TransferEngine(QObject *parent = 0);
    ~TransferEngine();

public Q_SLOTS:
    int uploadMediaItem( const QString &source,
                         const QString &serviceId,
                         const QString &mimeType,
                         bool metadataStripped,
                         const QVariantMap &userData);

    int uploadMediaItemContent(const QVariantMap &content,
                               const QString &serviceId,
                               const QVariantMap &userData);

    int createDownload(const QString &displayName,
                       const QString &applicationIcon,
                       const QString &serviceIcon,
                       const QString &filePath,
                       const QString &mimeType,
                       qlonglong expectedFileSize,
                       const QStringList &callback,
                       const QString &cancelMethod,
                       const QString &restartMethod);

    int createSync(const QString &displayName,
                   const QString &applicationIcon,
                   const QString &serviceIcon,
                   const QStringList &callback,
                   const QString &cancelMethod,
                   const QString &restartMethod);

    void startTransfer(int transferId);

    void restartTransfer(int transferId);

    void finishTransfer(int transferId, int status, const QString &reason);

    void updateTransferProgress(int transferId, double progress);

    QList<TransferDBRecord> transfers();

    QList<TransferDBRecord> activeTransfers();

    QList <TransferMethodInfoDeprecated> transferMethods();
    QList <TransferMethodInfo> transferMethods2();

    void clearTransfers();

    void clearTransfer(int transferId);

    void cancelTransfer(int transferId);

    void enableNotifications(bool enable);

    bool notificationsEnabled();

Q_SIGNALS:
    void progressChanged(int transferId, double progress);

    void statusChanged(int transferId, int status);

    void transferMethodListChanged();

    void transfersChanged();

    void activeTransfersChanged();

private:
    TransferEnginePrivate *d_ptr;
    Q_DECLARE_PRIVATE(TransferEngine)
};


#endif // TRANSFERENGINE_H
