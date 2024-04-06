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

#include "dbmanager.h"
#include "transfertypes.h"
#include "mediaitem.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QtDebug>
#include <QDateTime>
#include <QFile>
#include <QDir>

#define DB_PATH ".local/nemo-transferengine"
#define DB_NAME "transferdb.sqlite"

// Table for metadata
#define DROP_METADATA   "DROP TABLE metadata;"
#define TABLE_METADATA  "CREATE TABLE metadata  (metadata_id INTEGER PRIMARY KEY AUTOINCREMENT,\n" \
                        "title TEXT,\n" \
                        "description TEXT,\n" \
                        "transfer_id INTEGER NOT NULL,\n" \
                        "FOREIGN KEY(transfer_id) REFERENCES transfers(transfer_id) ON DELETE CASCADE\n" \
                        ");\n"

// Table for callbacks. In practice there are dbus interfaces
#define DROP_CALLBACK   "DROP TABLE callback;"
#define TABLE_CALLBACK  "CREATE TABLE callback	(callback_id INTEGER PRIMARY KEY AUTOINCREMENT,\n" \
                        "service TEXT,\n" \
                        "path TEXT,\n" \
                        "interface TEXT,\n"\
                        "cancel_method TEXT,\n"\
                        "restart_method TEXT,\n"\
                        "transfer_id INTEGER NOT NULL,\n"\
                        "FOREIGN KEY(transfer_id) REFERENCES transfers(transfer_id) ON DELETE CASCADE\n"\
                        ");\n"

// Table for all the transfers
#define DROP_TRANSFERS  "DROP TABLE transfers;"
#define TABLE_TRANSFERS "CREATE TABLE transfers (transfer_id INTEGER PRIMARY KEY AUTOINCREMENT,\n" \
                        "transfer_type INTEGER,\n" \
                        "timestamp TEXT,\n" \
                        "status INTEGER,\n" \
                        "progress REAL,\n" \
                        "display_name TEXT,\n" \
                        "application_icon TEXT,\n"\
                        "thumbnail_icon TEXT,\n"\
                        "service_icon TEXT,\n" \
                        "url TEXT,\n" \
                        "resource_name TEXT,\n" \
                        "mime_type TEXT,\n" \
                        "file_size INTEGER,\n" \
                        "plugin_id TEXT,\n" \
                        "account_id TEXT,\n"\
                        "strip_metadata INTEGER,\n" \
                        "scale_percent REAL,\n" \
                        "cancel_supported INTEGER,\n" \
                        "restart_supported INTEGER,\n" \
                        "notification_id INTEGER\n" \
                        "transient INTEGER\n" \
                        ");\n"

// Cascade trigger i.e. when transfer is removed and it has metadata or callbacks, this
// trigger make sure that they are also removed
#define DROP_TRIGGER    "DROP TRIGGER delete_cascade;"
#define TRIGGER         "CREATE TRIGGER delete_cascade\n" \
                        "BEFORE DELETE ON transfers\n" \
                        "FOR EACH ROW BEGIN\n" \
                        "    DELETE FROM metadata WHERE transfer_id = OLD.transfer_id;\n" \
                        "    DELETE FROM callback WHERE transfer_id = OLD.transfer_id;\n" \
                        "END;\n"

// Update the following version if database schema changes.
#define USER_VERSION 3
#define PRAGMA_USER_VERSION   QString("PRAGMA user_version=%1").arg(USER_VERSION)

class DbManagerPrivate {
public:

    QString currentDateTime()
    {
        QDateTime dt = QDateTime::currentDateTimeUtc();
        return dt.toString(Qt::ISODate);
    }

    bool createDatabaseSchema()
    {
        bool ok = true;
        QSqlQuery query;
        if (!query.exec(TABLE_METADATA)) {
            qWarning() << "DbManagerPrivate::createDatabase: create metadata table: "
                       << query.lastError().text() << ":" << query.lastError().databaseText();
            ok = false;
        }
        if (!query.exec(TABLE_CALLBACK)) {
            qWarning() << "DbManagerPrivate::createDatabase: create callback table: "
                       << query.lastError().text() << ":" << query.lastError().databaseText();
            ok = false;
        }
        if (!query.exec(TABLE_TRANSFERS)) {
            qWarning() << "DbManagerPrivate::createDatabase: create transfer table: "
                       << query.lastError().text() << ":" << query.lastError().databaseText();
            ok = false;
        }
        if (!query.exec(TRIGGER)) {
            qWarning() << "DbManagerPrivate::createDatabase: create cascade trigger: "
                       << query.lastError().text() << ":" << query.lastError().databaseText();
            ok = false;
        }
        if (!query.exec(PRAGMA_USER_VERSION)) {
            qWarning() << "DbManagerPrivate::createDatabase: pragma user_version: "
                       << query.lastError().text() << ":" << query.lastError().databaseText();
            ok = false;
        }
        query.finish();
        return ok;
    }

    bool deleteOldTables()
    {
        bool ok = true;
        QSqlQuery query;
        if (!query.exec(DROP_TRIGGER)) {
            qWarning() << Q_FUNC_INFO << "Drop trigger:"
                       << query.lastError().text() << ":" << query.lastError().databaseText();
            ok = false;
        }
        if (!query.exec(DROP_METADATA)) {
            qWarning() << Q_FUNC_INFO << "Drop metadata:"
                       << query.lastError().text() << ":" << query.lastError().databaseText();
            ok = false;
        }
        if (!query.exec(DROP_CALLBACK)) {
            qWarning() << Q_FUNC_INFO << "Drop callback:"
                       << query.lastError().text() << ":" << query.lastError().databaseText();
            ok = false;
        }
        if (!query.exec(DROP_TRANSFERS)) {
            qWarning() << Q_FUNC_INFO << "Drop transfers:"
                       << query.lastError().text() << ":" << query.lastError().databaseText();
            ok = false;
        }

        query.finish();
        return ok;
    }

    int userVersion()
    {
        const QString queryStr = QString("PRAGMA user_version");

        QSqlQuery query;
        if (!query.exec(queryStr)) {
            qWarning() << "DbManager::callback: Failed to execute SQL query for user_version."
                       << query.lastError().text() << ": "
                       << query.lastError().databaseText();
            return -1;
        }
        QSqlRecord rec = query.record();
        if (query.isActive() && query.isSelect()) {
            query.first();
            QString v = query.value(rec.indexOf("user_version")).toString();
            if (v.isEmpty()) {
                return -1;
            }
            return v.toInt();
        }
        return -1;
    }

    QSqlDatabase m_db;
};

/*! \class DbManager
    \brief The DbManager class is a singleton class to read and write transfers database.

    \ingroup transfer-engine

    DbManager class takes care of reading and writing transfer database used by Nemo Transfer
    Engine. It's a singleton class and it can be instantiated using DbManager::instance() method.
*/


/*!
    Returns a singleton instance of this DbManager. Note that caller is NOT responsible of
    deleting the instance. It will be deleted automatically when application stack is cleaned.
 */
DbManager * DbManager::instance()
{
    static DbManager instance;
    return &instance;
}

/*!
    Private constructor.
 */
DbManager::DbManager():
    d_ptr(new DbManagerPrivate)
{
    Q_D(DbManager);
    const QString absDbPath = QDir::homePath() + QDir::separator()
                            + DB_PATH + QDir::separator()
                            + DB_NAME;

    bool dbExists = QFile::exists(absDbPath);

    if (!dbExists) {
        if (!QDir().mkpath(QDir::homePath() + QDir::separator() + DB_PATH)) {
            qWarning() << "DbManager::DbManager: failed to create directory for db!";
            return;
        }
    }

    d->m_db = QSqlDatabase::addDatabase("QSQLITE");
    d->m_db.setDatabaseName(absDbPath);
    d->m_db.setConnectOptions("foreign_keys = ON"); // sanity check
    d->m_db.open();

    // Create database schema if db didn't exist
    if (!dbExists) {
        if (!d->createDatabaseSchema()) {
            qCritical("DbManager::DbManager: Failed to create DB schema. Can't continue!");
        }
    } else {
        // Database exists, check the schema version
        if (d->userVersion() == 1) {
            // For this we get away with DeclarativeTransferModel directly reading database without
            // update because notification_id is the last column
            QSqlQuery query;
            if (query.exec("ALTER TABLE transfers ADD COLUMN notification_id INTEGER")) {
                qWarning() << "Extended transfers table";

                if (!query.exec(PRAGMA_USER_VERSION)) {
                    qWarning() << "DbManager pragma user_version update:"
                               << query.lastError().text() << ":" << query.lastError().databaseText();
                }
            } else {
                qWarning() << "Failed to extend transfers table!"
                           << query.lastError().text() << ":" << query.lastError().databaseText();
            }
        }

        if (d->userVersion() == 2) {
            // For this we get away with DeclarativeTransferModel directly reading database without
            // update because transient is the last column
            QSqlQuery query;
            if (query.exec("ALTER TABLE transfers ADD COLUMN transient INTEGER")) {
                qWarning() << "Extended transfers table for transient column";

                if (!query.exec(PRAGMA_USER_VERSION)) {
                    qWarning() << "DbManager pragma user_version update:"
                               << query.lastError().text() << ":" << query.lastError().databaseText();
                }
            } else {
                qWarning() << "Failed to extend transfers table for transient column!"
                           << query.lastError().text() << ":" << query.lastError().databaseText();
            }
        }

        if (d->userVersion() != USER_VERSION) {
            d->deleteOldTables();
            d->createDatabaseSchema();
        }
    }

    TransferDBRecord::registerType();
}

/*!
    Destroys the DbManager. Clients should not call this, instead the created DbManager instance
    will be destroyed automatically.
 */
DbManager::~DbManager()
{
    Q_D(DbManager);
    if (d->m_db.isOpen()) {
        d->m_db.close();
    }

    delete d_ptr;
    d_ptr = 0;
}

/*!
    Returns a DBus callback interface and method names for the transfer with \a key.
    If there is no callback for the \a key then an empty QStringList is returned.

    In a case there is a DBus callback, then QStringList contains the following items:
    \list
        \li service
        \li path
        \li interface
        \li cancel method name
        \li restart method name
    \endlist
 */
QStringList DbManager::callback(int key) const
{
    QString queryStr = QString("SELECT service, path, interface, cancel_method, restart_method FROM callback WHERE transfer_id='%1';")
            .arg(QString::number(key));

    QStringList result;
    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "DbManager::callback: Failed to execute SQL query. Couldn't get callback!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return result;
    }

    QSqlRecord rec = query.record();
    if (query.isActive() && query.isSelect()) {
        query.first();
        result <<  query.value(rec.indexOf("service")).toString()
                << query.value(rec.indexOf("path")).toString()
                << query.value(rec.indexOf("interface")).toString()
                << query.value(rec.indexOf("cancel_method")).toString()
                << query.value(rec.indexOf("restart_method")).toString();
    }
    return result;
}

/*!
    Creates a metadata entry for the existing transfer with \a key. Metadata can contain only
    \a title and/or \a description.

    Metadata entry will be created to the metadata table. Argument \a key must point to the
    existing entry in transfers table.

    This method returns a key of the created record in metadata table or -1 on failure.

    NOTE: Deleting the record from the  transfer which has a \a key, also deletes related
    metadata entry.
 */
int DbManager::createMetadataEntry(int key, const QString &title, const QString &description)
{
    QSqlQuery query;
    query.prepare("INSERT INTO metadata (title, description, transfer_id)"
                  "VALUES (:title, :description, :transfer_id)");
    query.bindValue(":title",       title);
    query.bindValue(":description", description);
    query.bindValue(":transfer_id", key);

    if (!query.exec()) {
        qWarning() << "DbManager::createMetadataEntry: Failed to execute SQL query. Couldn't create an entry!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return -1;
    }

    // Return the id of the last record
    QVariant rowId = query.lastInsertId();
    query.finish();
    return rowId.isValid()? rowId.toInt() : -1;
}

/*!
    Creates a callback entry to the callback table for the existing transfer with a \a key.

    The callback is a dbus interface so it must contain the following attributes:
    \list
        \li \a service e.g. com.jolla.myapp
        \li \a path e.g. /com/jolla/myapp
        \li \a interface e.g. com.jolla.myapp
        \li \a cancelMethod The name of the cancel method
        \li \a restartMethod The name of the restart method
    \endlist

    This method returns a key of the created callback record in a callback table or -1 on
    failure.

    NOTE: Deleting the record from the  transfer which has a \a key, also deletes related
    callback entry.

 */
int DbManager::createCallbackEntry(int key,
                                   const QString &service,
                                   const QString &path,
                                   const QString &interface,
                                   const QString &cancelMethod,
                                   const QString &restartMethod)
{
    QSqlQuery query;
    query.prepare("INSERT INTO callback (service, path, interface, cancel_method, restart_method, transfer_id)"
                  "VALUES (:service, :path, :interface, :cancel_method, :restart_method, :transfer_id)");
    query.bindValue(":service",         service);
    query.bindValue(":path",            path);
    query.bindValue(":interface",       interface);
    query.bindValue(":cancel_method",   cancelMethod);
    query.bindValue(":restart_method",  restartMethod);
    query.bindValue(":transfer_id",     key);

    if (!query.exec()) {
        qWarning() << "DbManager::createCallbackEntry: Failed to execute SQL query. Couldn't create an entry!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return -1;
    }

    // Return the id of the last record
    QVariant rowId = query.lastInsertId();
    query.finish();
    return rowId.isValid()? rowId.toInt() : -1;
}

/*!
    Creates a transfer entry to the transfers table bsaed on \a mediaItem content.

    MediaItem instance contains all the required information for the single Upload,
    Download or a Sync item. Based on this information, DbManager creates a record
    to the transfers table, but also to the callback and metadata tables if these
    are defined.

    This method returns a key of the created transfer or -1 on failure.
*/
int DbManager::createTransferEntry(const MediaItem *mediaItem)
{
    Q_D(DbManager);
    QSqlQuery query;
    query.prepare("INSERT INTO transfers (transfer_type, timestamp, status, progress, display_name, application_icon, thumbnail_icon, "
                  "  service_icon, url, resource_name, mime_type, file_size, plugin_id, account_id, strip_metadata, scale_percent, "
                  "  cancel_supported, restart_supported, notification_id, transient)"
                  "VALUES (:transfer_type, :timestamp, :status, :progress, :display_name, :application_icon, :thumbnail_icon, "
                  "  :service_icon, :url, :resource_name, :mime_type, :file_size, :plugin_id, :account_id, :strip_metadata, :scale_percent, "
                  "  :cancel_supported, :restart_supported, :notification_id, :transient)");
    query.bindValue(":transfer_type",       mediaItem->value(MediaItem::TransferType));
    query.bindValue(":status",              TransferEngineData::NotStarted);
    query.bindValue(":timestamp",           d->currentDateTime());
    query.bindValue(":progress",            0);
    query.bindValue(":display_name",        mediaItem->value(MediaItem::DisplayName));
    query.bindValue(":application_icon",    mediaItem->value(MediaItem::ApplicationIcon));
    query.bindValue(":thumbnail_icon",      mediaItem->value(MediaItem::ThumbnailIcon));
    query.bindValue(":service_icon",        mediaItem->value(MediaItem::ServiceIcon));
    query.bindValue(":url",                 mediaItem->value(MediaItem::Url));
    query.bindValue(":resource_name",       mediaItem->value(MediaItem::ResourceName));
    query.bindValue(":mime_type",           mediaItem->value(MediaItem::MimeType));
    query.bindValue(":file_size",           mediaItem->value(MediaItem::FileSize));
    query.bindValue(":plugin_id",           mediaItem->value(MediaItem::PluginId));
    query.bindValue(":account_id",          mediaItem->value(MediaItem::AccountId));
    query.bindValue(":strip_metadata",      mediaItem->value(MediaItem::MetadataStripped));
    query.bindValue(":scale_percent",       mediaItem->value(MediaItem::ScalePercent));
    query.bindValue(":cancel_supported",    mediaItem->value(MediaItem::CancelSupported));
    query.bindValue(":restart_supported",   mediaItem->value(MediaItem::RestartSupported));
    query.bindValue(":notification_id",     0);
    query.bindValue(":transient",           mediaItem->value(MediaItem::Transient).toBool() ? 1 : 0);

    if (!query.exec()) {
        qWarning() << "DbManager::createTransferEntry: Failed to execute SQL query. Couldn't create an entry!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return -1;
    }

    // Return the id of the last record
    QVariant rowId = query.lastInsertId();
    query.finish();

    if (!rowId.isValid()) {
        qWarning() << "DbManager::createShareEntry: Invalid row ID!";
        return -1;
    }

    // Create a metadata entry if user has passed any
    const QString title = mediaItem->value(MediaItem::Title).toString();
    const QString desc  = mediaItem->value(MediaItem::Description).toString();
    if (!title.isEmpty() || !desc.isEmpty()) {
        if (createMetadataEntry(rowId.toInt(), title, desc) < 0){
            qWarning() << "DbManager::createTransferEntry: Failed to create metadata entry";
            return -1;
        }
    }

    // Create a callback entry if it's been provided
    const QStringList callback  = mediaItem->value(MediaItem::Callback).toStringList();
    const QString cancelMethod  = mediaItem->value(MediaItem::CancelCBMethod).toString();
    const QString restartMethod = mediaItem->value(MediaItem::RestartCBMethod).toString();

    // One of the methods must exist if the callback has been provided
    if (callback.count() == 3 && (!cancelMethod.isEmpty() || !restartMethod.isEmpty())){
        const int res = createCallbackEntry(rowId.toInt(), callback.at(0), callback.at(1), callback.at(2),
                                            cancelMethod, restartMethod);
        if (res < 0) {
            qWarning() << "DbManager::createTransferEntry: Failed to create callback entry";
            return -1;
        }
    }

    return rowId.toInt();
}

/*!
    Updates transfer \a status of the existing transfer with \a key. Changing the status updates
    the timestamp too.

    This method returns true on success, false on failure.
 */
bool DbManager::updateTransferStatus(int key, TransferEngineData::TransferStatus status)
{
    Q_D(DbManager);
    QString queryStr;
    switch(status) {
    case TransferEngineData::TransferStarted:
        queryStr = QString("UPDATE transfers SET status='%1', progress='0', timestamp='%2' WHERE transfer_id='%3';")
                .arg(QString::number(status))
                .arg(d->currentDateTime())
                .arg(QString::number(key));
        break;

    case TransferEngineData::NotStarted:
    case TransferEngineData::TransferFinished:
    case TransferEngineData::TransferInterrupted:
    case TransferEngineData::TransferCanceled:
        queryStr = QString("UPDATE transfers SET status='%1', timestamp='%2' WHERE transfer_id='%3';")
                .arg(QString::number(status))
                .arg(d->currentDateTime())
                .arg(QString::number(key));
        break;
    case TransferEngineData::Unknown:
        qWarning() << "Unknown transfer status!";
        return false;
    }

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "Failed to execute SQL query. Couldn't update a record!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return false;
    }
    query.finish();
    return true;
}

/*!
    Updates transfer \a progress of the existing transfer with \a key.

    This method returns true on success, false on failure.
 */
bool DbManager::updateProgress(int key, qreal progress)
{
    QString queryStr = QString("UPDATE transfers SET progress='%1' WHERE transfer_id='%2';")
            .arg(QString::number(progress))
            .arg(QString::number(key));

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "Failed to execute SQL query. Couldn't update the progress!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return false;
    }
    query.finish();
    return true;
}

/*!
    Removes an existing transfer with a \a key from the transfers table. If this transfer has
    metadata or callback defined, they will be removed too.

    This method returns true on success, false on failure.
 */
bool DbManager::removeTransfer(int key)
{
    QString queryStr = QString("DELETE FROM transfers WHERE transfer_id='%1' AND (status='%2' OR status='%3' OR status='%4');")
            .arg(key)
            .arg(TransferEngineData::TransferFinished)
            .arg(TransferEngineData::TransferCanceled)
            .arg(TransferEngineData::TransferInterrupted);

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << Q_FUNC_INFO;
        qWarning() << "Failed to execute SQL query: " << query.lastQuery();
        qWarning() << query.lastError().text();
        return false;
    }

    query.finish();
    return true;
}

/*!
    Remove failed transfers with same provider except the one having the \a excludeKey. Argument \a type defines the type
    of the removed failed transfers and can be one of TransferEngineData::Download, TransferEngineData::Upload
    or TransferEngineData::Sync.

    This methods returns true on success or false on failure
 */
bool DbManager::clearFailedTransfers(int excludeKey, TransferEngineData::TransferType type)
{
    // DELETE FROM transfers where transfer_id!=4584 AND status=5 AND  display_name=(SELECT display_name FROM transfers WHERE transfer_id=4584);
    QString queryStr = QString("DELETE FROM transfers WHERE transfer_id!=%1 AND status=%2 AND transfer_type=%3 AND display_name=(SELECT display_name FROM transfers WHERE transfer_id=%1);")
            .arg(excludeKey)
            .arg(TransferEngineData::TransferInterrupted)
            .arg(type);

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << Q_FUNC_INFO;
        qWarning() << "Failed to execute query: " << query.lastQuery();
        qWarning() << query.lastError().text();
        return false;
    }
    query.finish();
    return true;
}

/*!
    Clears all finished, canceled or failed transfers from the database.

    This method returns  true on success, false on failure.
*/
bool DbManager::clearTransfers()
{
    QString queryStr = QString("DELETE FROM transfers WHERE status='%1' OR status='%2' OR status='%3';")
            .arg(TransferEngineData::TransferFinished)
            .arg(TransferEngineData::TransferCanceled)
            .arg(TransferEngineData::TransferInterrupted);

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "Failed to execute SQL query. Couldn't delete the list finished transfers!";
        return false;
    }

    query.finish();
    return true;
}

bool DbManager::clearTransfer(int key)
{
    QSqlQuery query;
    TransferEngineData::TransferStatus status = transferStatus(key);
    switch (status) {
    case TransferEngineData::TransferFinished:
    case TransferEngineData::TransferCanceled:
    case TransferEngineData::TransferInterrupted:
        if (query.exec(QString("DELETE FROM transfers WHERE transfer_id='%1';").arg(QString::number(key)))) {
            return true;
        }
        qWarning() << "Failed to execute SQL query. Couldn't delete transfer" << key;
        return false;
    default:
        qWarning() << "Not clearing transfer" << key << "because its status is" << status;
        return false;
    }
}

int DbManager::transferCount() const
{
    QSqlQuery query;
    if (query.exec(QString("SELECT COUNT(transfer_id) FROM transfers"))) {
        query.next();
        return query.value(0).toInt();
    } else {
        qWarning() << "DbManager::transferCount: Failed to execute SQL query!";
        return -1;
    }
}

int DbManager::activeTransferCount() const
{
    QSqlQuery query;
    if (query.exec(QString("SELECT COUNT(transfer_id) FROM transfers WHERE status='%1'").arg(TransferEngineData::TransferStarted))) {
        query.next();
        return query.value(0).toInt();
    } else {
        qWarning() << "DbManager::activeTransferCount: Failed to execute SQL query!";
        return -1;
    }
}

/*!
    Returns all the transfers from the database. This method doesn't fetch all the fields from all the
    tables, instead it returns what is required to fill fields in TransferDBRecord class.
 */
QList<TransferDBRecord> DbManager::transfers() const
{
    return transfers(TransferEngineData::Unknown);
}

/*!
    Returns all the active transfers from the database. This method doesn't fetch all the fields from all the
    tables, instead it returns what is required to fill fields in TransferDBRecord class.
 */
QList<TransferDBRecord> DbManager::activeTransfers() const
{
    return transfers(TransferEngineData::TransferStarted);
}

/*!
    Returns all the transfers from the database which have the specified status. This method doesn't fetch all the fields from all the
    tables, instead it returns what is required to fill fields in TransferDBRecord class.
 */
QList<TransferDBRecord> DbManager::transfers(TransferEngineData::TransferStatus status) const
{
    // TODO: This should order the result based on timestamp
    QList<TransferDBRecord> records;
    QString queryStr = (status == TransferEngineData::Unknown) ?
        QString("SELECT * FROM transfers WHERE transient=0 ORDER BY transfer_id DESC") :
        QString("SELECT * FROM transfers WHERE transient=0 AND status='%1' ORDER BY transfer_id DESC").arg(status);
    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "DbManager::transfers: Failed to execute SQL query. Couldn't get list of transfers!";
        return records;
    }

    // The record could actually contain eg. QVariantList instead of hardcoded and
    // typed members.
    QSqlRecord rec = query.record();
    while (query.next()) {
        TransferDBRecord record;
        record.transfer_id          = query.value(rec.indexOf("transfer_id")).toInt();
        record.transfer_type        = query.value(rec.indexOf("transfer_type")).toInt();
        record.progress             = query.value(rec.indexOf("progress")).toDouble();
        record.url                  = query.value(rec.indexOf("url")).toString();
        record.status               = query.value(rec.indexOf("status")).toInt();
        record.plugin_id            = query.value(rec.indexOf("plugin_id")).toString();
        record.display_name         = query.value(rec.indexOf("display_name")).toString();
        record.resource_name        = query.value(rec.indexOf("resource_name")).toString();
        record.mime_type            = query.value(rec.indexOf("mime_type")).toString();
        record.timestamp            = query.value(rec.indexOf("timestamp")).toString();
        record.size                 = query.value(rec.indexOf("file_size")).toInt();
        record.application_icon     = query.value(rec.indexOf("application_icon")).toString();
        record.thumbnail_icon       = query.value(rec.indexOf("thumbnail_icon")).toString();
        record.service_icon         = query.value(rec.indexOf("service_icon")).toString();
        record.cancel_supported     = query.value(rec.indexOf("cancel_supported")).toBool();
        record.restart_supported    = query.value(rec.indexOf("restart_supported")).toBool();
        records << record;
    }
    query.finish();
    return records;
}

/*!
    Returns the transfer type e.g. Sync, Download or Upload of the transfer with a \a key.

    If there is no transfer record with key or error occurs, this method returns TransferEngineData::Undefined.
 */
TransferEngineData::TransferType DbManager::transferType(int key) const
{
    QString queryStr = QString("SELECT transfer_type FROM transfers WHERE transfer_id='%1';")
            .arg(QString::number(key));

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "DbManager::transferType: Failed to execute SQL query. Couldn't get transfer type!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return TransferEngineData::Undefined;
    }


    if (query.isActive() && query.isSelect()) {
        query.first();
        return static_cast<TransferEngineData::TransferType>(query.value(0).toInt());
    } else {
        return TransferEngineData::Undefined;
    }
}

/*!
    Returns the transfer status of the transfer with \a key. In a case of error
    the TransferEngineData::Unknown is returned.
 */
TransferEngineData::TransferStatus DbManager::transferStatus(int key) const
{

    QString queryStr = QString("SELECT status FROM transfers WHERE transfer_id='%1';")
            .arg(QString::number(key));

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "DbManager::transferStatus: Failed to execute SQL query. Couldn't get transfer status!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return TransferEngineData::Unknown;
    }


    if (query.isActive() && query.isSelect()) {
        query.first();
        return static_cast<TransferEngineData::TransferStatus>(query.value(0).toInt());
    } else {
        return TransferEngineData::Unknown;
    }
}

/*!
     Returns the transfer progress or -1 if unavailable
 */
qreal DbManager::transferProgress(int key) const
{
    QString queryStr = QString("SELECT progress FROM transfers WHERE transfer_id='%1';").arg(QString::number(key));

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "DbManager::transferStatus: Failed to execute SQL query. Couldn't get the progress!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return -1;
    }

    if (query.isActive() && query.isSelect()) {
        query.first();
        return static_cast<qreal>(query.value(0).toReal());
    } else {
        return -1;
    }
}

int DbManager::notificationId(int key)
{
    QString queryStr = QString("SELECT notification_id FROM transfers WHERE transfer_id='%1';").arg(QString::number(key));

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "DbManager::transferStatus: Failed to execute SQL query. Couldn't get the notification id!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return 0;
    }

    if (query.isActive() && query.isSelect()) {
        query.first();
        return static_cast<qreal>(query.value(0).toReal());
    } else {
        return 0;
    }
}

bool DbManager::setNotificationId(int key, int notificationId)
{
    QString queryStr = QString("UPDATE transfers SET notification_id='%1' WHERE transfer_id='%2';")
            .arg(QString::number(notificationId)).arg(QString::number(key));

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "Failed to execute SQL query. Couldn't update the notification id!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return false;
    }
    query.finish();
    return true;
}

/*!
    Reads the callback method names from the database for the transfer with \a key. The method names are set to the
    output arguments \a cancelMethod and \a restartMethod.

    This method returns true on success, false on failure.
*/
bool DbManager::callbackMethods(int key, QString &cancelMethod, QString &restartMethod) const
{
    QString queryStr = QString("SELECT cancel_method, restart_method FROM callback WHERE transfer_id='%1';")
            .arg(QString::number(key));

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "DbManager::callbackMethods: Failed to execute SQL query. Couldn't get callback methods!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return false;
    }


    if (query.isActive() && query.isSelect()) {
        query.first();
        cancelMethod = query.value(0).toString();
        restartMethod = query.value(1).toString();
        return true;
    } else {
        return false;
    }
}

// Used only for Sharing atm. If this is needed for Sync and download add fetching the callback too.
/*!
    Returns a MediaItem instance from the transfer data with a \a key.
*/
MediaItem * DbManager::mediaItem(int key) const
{
    QString queryStr = QString("SELECT * FROM transfers WHERE transfer_id='%1';")
            .arg(QString::number(key));

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qWarning() << "DbManager::mediaItem: Failed to execute SQL query. Couldn't update the progress!"
                   << query.lastError().text() << ": "
                   << query.lastError().databaseText();
        return 0;
    }

    MediaItem *item = new MediaItem;
    QSqlRecord rec = query.record();
    if (query.next()) {
        item->setValue(MediaItem::Url,             query.value(rec.indexOf("url")));
        item->setValue(MediaItem::MetadataStripped,query.value(rec.indexOf("strip_metadata")));
        item->setValue(MediaItem::ScalePercent,    query.value(rec.indexOf("scale_percent")));
        item->setValue(MediaItem::ResourceName,    query.value(rec.indexOf("resource_name")));
        item->setValue(MediaItem::MimeType,        query.value(rec.indexOf("mime_type")));
        item->setValue(MediaItem::TransferType,    query.value(rec.indexOf("transfer_type")));
        item->setValue(MediaItem::FileSize,        query.value(rec.indexOf("file_size")));
        item->setValue(MediaItem::PluginId,        query.value(rec.indexOf("plugin_id")));
        item->setValue(MediaItem::AccountId,       query.value(rec.indexOf("account_id")));
        item->setValue(MediaItem::DisplayName,     query.value(rec.indexOf("display_name")));
        item->setValue(MediaItem::ServiceIcon,     query.value(rec.indexOf("service_icon")));
        item->setValue(MediaItem::ApplicationIcon, query.value(rec.indexOf("application_icon")));
        item->setValue(MediaItem::ThumbnailIcon,   query.value(rec.indexOf("thumbnail_icon")));
        item->setValue(MediaItem::CancelSupported, query.value(rec.indexOf("cancel_supported")));
        item->setValue(MediaItem::RestartSupported, query.value(rec.indexOf("restart_supported")));
    } else {
        qWarning() << "DbManager::mediaItem: Failed to get media item data from database!";
        delete item;
        return 0;
    }

    query.finish();

    // Get metadata part title, description
    queryStr = QString("SELECT title, description FROM metadata WHERE transfer_id='%1';")
            .arg(QString::number(key));

    if (!query.exec(queryStr)) {
            qWarning() << "DbManager::mediaItem: Failed to execute SQL query. Couldn't update the progress!"
                       << query.lastError().text() << ": "
                       << query.lastError().databaseText();
            delete item;
            return 0;
    }
    rec = query.record();
    // NOTE: There might be that user hasn't set any title or description so let the flow just pass
    //       this point if there isn't anything...
    if (query.next()) {
        item->setValue(MediaItem::Title, query.value(rec.indexOf("title")));
        item->setValue(MediaItem::Description, query.value(rec.indexOf("description")));
    }

    query.finish();
    return item;
}




