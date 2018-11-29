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

#ifndef TRANSFERENGINECLIENT_H
#define TRANSFERENGINECLIENT_H

#include <QObject>
#include <QUrl>
#include "transfertypes.h"

class CallbackInterfacePrivate;

class CallbackInterface {
public:
    CallbackInterface();
    CallbackInterface(const QString &server, const QString &path, const QString &interface,
                      const QString &cancelMethod, const QString &restartMethod);
    ~CallbackInterface();

private:
    CallbackInterfacePrivate *d_ptr;
    Q_DECLARE_PRIVATE(CallbackInterface)
    friend class TransferEngineClient;
};

class TransferEngineClientPrivate;
class TransferEngineClient : public QObject
{
    Q_OBJECT
public:
    enum Status {
        TransferFinished = TransferEngineData::TransferFinished,
        TransferCanceled = TransferEngineData::TransferCanceled,
        TransferInterrupted = TransferEngineData::TransferInterrupted
    };

    explicit TransferEngineClient(QObject *parent = 0);
    ~TransferEngineClient();

    int createDownloadEvent(const QString &displayName,
                            const QUrl &applicationIcon,
                            const QUrl &serviceIcon,
                            const QUrl &url,
                            const QString &mimeType,
                            qlonglong expectedFileSize,
                            const CallbackInterface &callback = CallbackInterface());


    int createSyncEvent(const QString &displayName,
                        const QUrl &applicationIcon,
                        const QUrl &serviceIcon,
                        const CallbackInterface &callback = CallbackInterface());

    void startTransfer(int transferId);
    void updateTransferProgress(int transferId, qreal progress);
    void finishTransfer(int transferId, Status status, const QString &reason = QString());

private:
    void cbCancelTransfer(int transferId);
    void cbRestartTransfer(int transferId);
    void clearTransfer(int transferId);
    void clearTransfers();
    void enableNotifications(bool enable);
    bool notificationsEnabled() const;

private:
    TransferEngineClientPrivate *d_ptr;
    Q_DECLARE_PRIVATE(TransferEngineClient)

    friend class DeclarativeTransferInterface;
};

#endif // TRANSFERENGINECLIENT_H
