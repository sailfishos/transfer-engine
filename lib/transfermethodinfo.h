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

#ifndef TRANSFERMETHODINFO_H
#define TRANSFERMETHODINFO_H

#include <QtGlobal>
#include <QtDBus/QtDBus>

class TransferMethodInfoDeprecated
{
public:
    friend QDBusArgument &operator<<(QDBusArgument &argument, const TransferMethodInfoDeprecated &record);
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, TransferMethodInfoDeprecated &record);

    static void registerType();

    QString displayName;
    QString userName;
    QString methodId;
    QString shareUIPath;
    QStringList capabilitities;
    quint32 accountId;
};

class TransferMethodInfo
{
public:

    // Used internally only
    enum TransferMethodInfoField {
        DisplayName,     // e.g. Facebook
        UserName,        // e.g. mike.myers@gmail.com
        MethodId,        // id of the plugin
        AccountId,       // id the account, needed in a case of multiple accounts
        AccountIcon,     // account icon source url
        ShareUIPath,     // path to the share ui QML plugin
        Capabilities     // list of supported mimetypes
    };

    TransferMethodInfo();
    TransferMethodInfo &operator=(const TransferMethodInfo &other);
    TransferMethodInfo(const TransferMethodInfo &other);
    ~TransferMethodInfo();

    friend QDBusArgument &operator<<(QDBusArgument &argument, const TransferMethodInfo &record);
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, TransferMethodInfo &record);

    static void registerType();

    QVariant value(int index) const;

    QString displayName;
    QString userName;
    QString methodId;
    QString shareUIPath;
    QStringList capabilitities;
    quint32 accountId;
    QString accountIcon;
    QVariantHash hints;

};

#endif // TRANSFERMETHODINFO_H
