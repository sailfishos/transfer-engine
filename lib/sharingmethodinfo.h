/*
 * Copyright (c) 2013 - 2019 Jolla Ltd.
 * Copyright (c) 2021 Open Mobile Platform LLC.
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

#ifndef SHARINGMETHODINFO_H
#define SHARINGMETHODINFO_H

#include <QtGlobal>
#include <QString>
#include <QVariant>

class SharingMethodInfoPrivate;
class SharingMethodInfo
{
public:
    // Used internally only
    enum SharingMethodInfoField {
        DisplayName,           // e.g. Facebook
        Subtitle,              // clarifies display name, e.g. mike.myers@gmail.com
        MethodId,              // id of the plugin
        MethodIcon,            // method icon source url
        AccountId,             // id the account, needed in a case of multiple accounts
        ShareUIPath,           // path to the share ui QML plugin
        Capabilities,          // list of supported mimetypes
        SupportsMultipleFiles  // supports sharing multiple files
    };

    SharingMethodInfo();
    SharingMethodInfo &operator=(const SharingMethodInfo &other);
    SharingMethodInfo(const SharingMethodInfo &other);
    ~SharingMethodInfo();

    QVariant value(int field) const;

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    QString subtitle() const;
    void setSubtitle(const QString &subtitle);

    QString methodId() const;
    void setMethodId(const QString &methodId);

    QString methodIcon() const;
    void setMethodIcon(const QString &methodId);

    quint32 accountId() const;
    void setAccountId(quint32 accountId);

    QString shareUIPath() const;
    void setShareUIPath(const QString &shareUIPath);

    QStringList capabilities() const;
    void setCapabilities(const QStringList &capabilities);

    void setSupportsMultipleFiles(bool supportsMultipleFiles);
    bool supportsMultipleFiles() const;

private:
    SharingMethodInfoPrivate *d_ptr = nullptr;
    Q_DECLARE_PRIVATE(SharingMethodInfo);
};

#endif // SHARINGMETHODINFO_H
