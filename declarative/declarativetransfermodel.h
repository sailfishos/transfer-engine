/*
 * Copyright (c) 2014 - 2019 Jolla Ltd.
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

#ifndef DECLARATIVETRANSFERMODEL_HH
#define DECLARATIVETRANSFERMODEL_HH

#include <QAbstractListModel>
#include <QJSValue>
#include <QMutex>
#include <QQmlParserStatus>
#include <QRunnable>
#include <QStringList>
#include <QWaitCondition>
#include <QSqlDatabase>


#include "transferengineinterface.h"
#include "transfertypes.h"


class TransferModel
        : public QAbstractListModel
        , public QQmlParserStatus
        , private QRunnable
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int transfersInProgress READ transfersInProgress NOTIFY transfersInProgressChanged)
    Q_ENUMS(Status)
    Q_ENUMS(TransferStatus)
    Q_ENUMS(TransferType)
    Q_INTERFACES(QQmlParserStatus)
public:
    enum Status {
        Null,
        Querying,
        Finished,
        Error
    };

    enum TransferStatus {
        NotStarted          = TransferEngineData::NotStarted,
        TransferStarted     = TransferEngineData::TransferStarted,
        TransferCanceled    = TransferEngineData::TransferCanceled,
        TransferFinished    = TransferEngineData::TransferFinished,
        TransferInterrupted = TransferEngineData::TransferInterrupted
    };

    enum TransferType {
        Upload      = TransferEngineData::Upload,
        Download    = TransferEngineData::Download,
        Sync        = TransferEngineData::Sync
    };

    TransferModel(QObject *parent = 0);
    ~TransferModel();

    Status status() const;

    Q_INVOKABLE QJSValue get(int index) const;
    Q_INVOKABLE void clearTransfers();
    Q_INVOKABLE void clearTransfer(int transferId);

    QHash<int, QByteArray> roleNames() const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    void classBegin();
    void componentComplete();

    void insertRange(int index, int count, const QVector<TransferDBRecord> &source, int sourceIndex);
    void updateRange(int index, int count, const QVector<TransferDBRecord> &source, int sourceIndex);
    void removeRange(int index, int count);

    bool event(QEvent *event);

    int transfersInProgress() const;

public slots:
    void refresh();

signals:
    void queryChanged();
    void classNamesChanged();
    void statusChanged();
    void countChanged();
    void transfersInProgressChanged();

private:
    void run();
    bool executeQuery(QVector<TransferDBRecord> *rows, int *activeTransfers, QString *errorString);

    QSqlDatabase database();

    QString m_asyncErrorString;
    QVector<TransferDBRecord> m_asyncRows;
    QVector<TransferDBRecord> *m_rows = nullptr;
    QHash<int, QByteArray> m_roles;

    QMutex m_mutex;
    QWaitCondition m_condition;

    int m_rowsIndex = 0;
    int m_asyncIndex = 0;
    int m_asyncTransfersInProgress = 0;
    int m_transfersInProgress = 0;

    Status m_status = Null;
    Status m_asyncStatus = Null;
    bool m_asyncPending = false;
    bool m_asyncRunning = false;
    bool m_notified = false;
    bool m_complete = false;
    bool m_rowsChanges = false;

    TransferEngineInterface *m_client = nullptr;
};

#endif
