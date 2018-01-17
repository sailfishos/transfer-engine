/****************************************************************************************
**
** Copyright (C) 2014-2016 Jolla Ltd.
** Contact: Marko Mattila <marko.mattila@jolla.com>
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

#include "declarativetransfermodel.h"
#include "synchronizelists_p.h"
#include "transferdbrecord.h"

#include <QQmlEngine>
#include <qqml.h>
#include <qqmlinfo.h>

#include <QSqlQuery>
#include <QSqlError>
#include <QThreadStorage>

#define DB_PATH ".local/nemo-transferengine"
#define DB_NAME "transferdb.sqlite"

template <> bool compareIdentity<TransferDBRecord>(
        const TransferDBRecord &item, const TransferDBRecord &reference)
{
    return item.transfer_id == reference.transfer_id;
}

class TransferDatabase : public QSqlDatabase
{
public:
    TransferDatabase() : QSqlDatabase(QLatin1String("QSQLITE")) {}
};

TransferModel::TransferModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_rows(new QVector<TransferDBRecord>())
    , m_rowsIndex(0)
    , m_asyncIndex(0)
    , m_transfersInProgress(0)
    , m_status(Null)
    , m_asyncStatus(Null)
    , m_asyncPending(false)
    , m_asyncRunning(false)
    , m_notified(false)
    , m_complete(false)
    , m_rowsChanges(false)
{
    setAutoDelete(false);
}

TransferModel::~TransferModel()
{
    QMutexLocker locker(&m_mutex);

    if (m_asyncRunning) {
        m_condition.wait(&m_mutex);
    }

    delete m_rows;
}

TransferModel::Status TransferModel::status() const
{
    return m_status;
}

void TransferModel::refresh()
{
    if (!m_complete || m_roles.isEmpty()) {
        return;
    }

    QMutexLocker locker(&m_mutex);

    if (!m_asyncRunning) {
        m_asyncRunning = true;
        QThreadPool::globalInstance()->start(static_cast<QRunnable *>(this));
    }
    m_status = Querying;
    m_asyncStatus = Querying;
    m_asyncPending = true;

    locker.unlock();

    if (m_status != Querying) {
        m_status = Querying;
        emit statusChanged();
    }
}

QJSValue TransferModel::get(int index) const
{
    QJSValue properties;
    QQmlEngine *engine = qmlEngine(this);

    if (engine && index >= 0 && index < m_rows->count()) {
        properties = engine->newObject();
        const TransferDBRecord &row = m_rows->at(index);

        foreach (const int &key, m_roles.keys()) {
            const QString name = m_roles.value(key);
            if (!name.isEmpty() && key >= 0) {
                properties.setProperty(name, engine->toScriptValue(row.value(key)));
            }
        }
    }
    return properties;
}

void TransferModel::clearTransfers()
{
    if (m_client) {
        m_client->clearTransfers();
    }
}

void TransferModel::clearTransfer(int transferId)
{
    if (m_client) {
        m_client->clearTransfer(transferId);
    }
}

QHash<int, QByteArray> TransferModel::roleNames() const
{
    return m_roles;
}

int TransferModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? m_rows->count() : 0;
}

QModelIndex TransferModel::index(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && row >= 0 && row < m_rows->count() && column == 0
            ? createIndex(row, column)
            : QModelIndex();
}

QVariant TransferModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role < 0) {
        return QVariant();
    } else if (role < m_roles.count()) {
        return m_rows->at(index.row()).value(role);
    } else {
        return QVariant();
    }
}

void TransferModel::classBegin()
{
}

void TransferModel::componentComplete()
{
    m_complete = true;

    m_roles[TransferDBRecord::TransferID]         = "transferId";
    m_roles[TransferDBRecord::TransferType]       = "transferType";
    m_roles[TransferDBRecord::Progress]           = "progress";
    m_roles[TransferDBRecord::URL]                = "url";
    m_roles[TransferDBRecord::Status]             = "status";
    m_roles[TransferDBRecord::PluginID]           = "pluginId";
    m_roles[TransferDBRecord::Timestamp]          = "timestamp";
    m_roles[TransferDBRecord::DisplayName]        = "displayName";
    m_roles[TransferDBRecord::ResourceName]       = "resourceName";
    m_roles[TransferDBRecord::MimeType]           = "mimeType";
    m_roles[TransferDBRecord::FileSize]           = "fileSize";
    m_roles[TransferDBRecord::ServiceIcon]        = "serviceIcon";
    m_roles[TransferDBRecord::ApplicationIcon]    = "applicationIcon";
    m_roles[TransferDBRecord::ThumbnailIcon]      = "thumbnailIcon";
    m_roles[TransferDBRecord::CancelSupported]    = "cancelEnabled";
    m_roles[TransferDBRecord::RestartSupported]   = "restartEnabled";

    refresh();

    m_client = new TransferEngineInterface("org.nemo.transferengine",
                                           "/org/nemo/transferengine",
                                           QDBusConnection::sessionBus(),
                                           this);

    connect(m_client,   SIGNAL(progressChanged(int,double)),
            this,       SLOT(refresh()));
    connect(m_client,   SIGNAL(transfersChanged()),
            this,       SLOT(refresh()));
    connect(m_client,   SIGNAL(statusChanged(int,int)),
            this,       SLOT(refresh()));
}

void TransferModel::insertRange(
        int index, int count, const QVector<TransferDBRecord> &source, int sourceIndex)
{
    if (m_rowsChanges) {
        beginInsertRows(QModelIndex(), index, index + count - 1);

        for (int i = 0; i < count; ++i) {
            m_rows->insert(index + i, source.at(sourceIndex + i));
        }

        endInsertRows();
        emit countChanged();
    }
}

void TransferModel::updateRange(
        int index, int count, const QVector<TransferDBRecord> &source, int sourceIndex)
{
    for (int i = 0; i < count; ++i) {
        const int begin = i;
        while (i < count
               && m_rows->at(i).transfer_id == source.at(sourceIndex + i).transfer_id
               && (m_rows->at(i).progress != source.at(sourceIndex + i).progress
               ||  m_rows->at(i).status != source.at(sourceIndex + i).status)) {

            TransferDBRecord &destinationRow = (*m_rows)[index + i];
            const TransferDBRecord &sourceRow = source.at(sourceIndex + i);

            destinationRow = sourceRow;

            ++i;
        }

        if (begin != i) {
            emit dataChanged(createIndex(index + begin, 0), createIndex(index + i - 1, 0));
        }
    }
}

void TransferModel::removeRange(int index, int count)
{
    if (m_rowsChanges) {
        beginRemoveRows(QModelIndex(), index, index + count - 1);

        m_rows->remove(index, count);

        endRemoveRows();
        emit countChanged();
    }
}

bool TransferModel::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        QMutexLocker locker(&m_mutex);

        const Status previousStatus = m_status;
        const QString errorString = m_asyncErrorString;
        m_status = m_asyncStatus;
        m_notified = false;

        const QVector<TransferDBRecord> rows = m_asyncRows;

        locker.unlock();

        m_rowsChanges = true;
        synchronizeList(this, *m_rows, m_rowsIndex, rows, m_asyncIndex);

        if (m_status >= Finished) {
            completeSynchronizeList(this, *m_rows, m_rowsIndex, rows, m_asyncIndex);
        }
        m_rowsChanges = false;

        if (m_asyncTransfersInProgress != m_transfersInProgress) {
            m_transfersInProgress = m_asyncTransfersInProgress;
            emit transfersInProgressChanged();
        }

        if (previousStatus != m_status) {
            if (m_status == Error) {
                qmlInfo(this) << errorString;
            }
            emit statusChanged();
        }

        return true;
    } else {
        return QAbstractListModel::event(event);
    }
}

int TransferModel::transfersInProgress() const
{
    return m_transfersInProgress;
}

void TransferModel::run()
{
    QMutexLocker locker(&m_mutex);

    for (;;) {
        if (m_asyncStatus == Querying) {
            m_asyncPending = false;
            if (m_status == Null) {
                m_asyncStatus = Null;
                continue;
            }

            locker.unlock();

            QVector<TransferDBRecord> rows;
            QString errorString;
            int activeTransfers = 0;
            const bool ok = executeQuery(&rows, &activeTransfers, &errorString);

            locker.relock();

            m_asyncRows = rows;
            m_asyncTransfersInProgress = activeTransfers;

            if (!m_asyncPending) {
                m_asyncErrorString = errorString;
                m_asyncStatus = ok ? Finished : Error;
            }
        } else {
            m_asyncRunning = false;
            if (!m_notified) {
                m_notified = true;
                QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
            }
            m_condition.wakeOne();
            return;
        }
    }
}


bool TransferModel::executeQuery(QVector<TransferDBRecord> *rows, int *activeTransfers, QString *errorString)
{
    // Query items from the database
    QSqlDatabase db = database();
    if (!db.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid database!";
        return false;
    }

    QSqlQuery query(db);
    query.setForwardOnly(true);

    if (!query.exec(QString(QStringLiteral("SELECT * FROM transfers ORDER BY transfer_id DESC")))) {
        qWarning() << Q_FUNC_INFO;
        qWarning() << "Failed to create tmp table";
        qWarning() << query.lastQuery();
        qWarning() << query.lastError().text();
        *errorString = query.lastError().text();
        return false;
    }

    *activeTransfers = 0;
    while (query.next()) {
        int i = 0;
        TransferDBRecord record;
        record.transfer_id          = query.value(i++).toInt();
        record.transfer_type        = query.value(i++).toInt();
        record.timestamp            = query.value(i++).toString();
        record.status               = query.value(i++).toInt();
        record.progress             = query.value(i++).toDouble();
        record.display_name         = query.value(i++).toString();
        record.application_icon     = query.value(i++).toString();
        record.thumbnail_icon       = query.value(i++).toString();
        record.service_icon         = query.value(i++).toString();
        record.url                  = query.value(i++).toString();
        record.resource_name        = query.value(i++).toString();
        record.mime_type            = query.value(i++).toString();
        record.size                 = query.value(i++).toInt();
        record.plugin_id            = query.value(i++).toString();
        i++; // account id
        i++; // strip metadata
        i++; // scale percent
        record.cancel_supported     = query.value(i++).toBool();
        record.restart_supported    = query.value(i++).toBool();

        if (record.status == TransferModel::TransferStarted) {
            ++(*activeTransfers);
        }

        rows->append(record);
    }

    QMutexLocker locker(&m_mutex);
    m_asyncRows = *rows;
    return true;
}


QSqlDatabase TransferModel::database()
{
    static QThreadStorage<QSqlDatabase> thread_database;

    if (!thread_database.hasLocalData()) {
        const QString absDbPath = QDir::homePath() + QDir::separator()
                                + DB_PATH + QDir::separator()
                                + DB_NAME;

	TransferDatabase database;
        database.setDatabaseName(absDbPath);
        database.setConnectOptions(QLatin1String("QSQLITE_OPEN_READONLY")); // sanity check
        thread_database.setLocalData(database);
    }

    QSqlDatabase &database = thread_database.localData();
    if (!database.isOpen() && !database.open()) {
        qWarning() << "Failed to open transfer engine database";
        qWarning() << database.lastError();
        return QSqlDatabase();
    }

    return database;
}
