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

#include "sharingmethodinfo.h"
#include "metatypedeclarations.h"

class SharingMethodInfoPrivate
{
public:
    QString displayName;
    QString subtitle;
    QString methodId;
    QString methodIcon;
    quint32 accountId = 0;
    QString shareUIPath;
    QStringList capabilities;
    bool supportsMultipleFiles = false;
};

/*!
    \class SharingMethodInfo
    \brief The SharingMethodInfo class encapsulate information of a single transfer method.

    \ingroup transfer-engine-lib

    Share plugin must create a list of instances of SharingMethodInfo class to encapsulate
    information about the plugin for example filling information for the Bluetooth sharing plugin:

    \code
        QList<SharingMethodInfo> infoList;
        SharingMethodInfo info;

        QStringList capabilities;
        capabilities << QLatin1String("*");

        info.setDisplayName(QLatin1String("Bluetooth"));
        info.setMethodIcon(QLatin1String("image://theme/icon-m-bluetooth"));
        info.setMethodId(QLatin1String("bluetooth"));
        info.setShareUIPath(SHARE_UI_PATH + QLatin1String("/BluetoothShareUI.qml"));
        info.setCapabilities(capabilities);
        infoList << info;
    \endcode
*/

/*!
    Creates an instance of SharingMethodInfo.
 */
SharingMethodInfo::SharingMethodInfo()
    : d_ptr(new SharingMethodInfoPrivate)
{
}

/*!
    Assigns \a other object to this.
*/
SharingMethodInfo &SharingMethodInfo::operator=(const SharingMethodInfo &other)
{
    *d_ptr = *other.d_ptr;
    return *this;
}

/*!
    Copies \a other to this instance.
*/
SharingMethodInfo::SharingMethodInfo(const SharingMethodInfo &other):
    d_ptr(new SharingMethodInfoPrivate(*other.d_ptr))
{
}

SharingMethodInfo::~SharingMethodInfo()
{
    delete d_ptr;
}

/*!
    Returns the value using the \a field, which is enum SharingMethodInfoField.

    \internal
 */
QVariant SharingMethodInfo::value(int field) const
{
    switch(field) {
    case DisplayName:
        return displayName();
    case Subtitle:
        return subtitle();
    case MethodId:
        return methodId();
    case MethodIcon:
        return methodIcon();
    case AccountId:
        return accountId();
    case ShareUIPath:
        return shareUIPath();
    case Capabilities:
        return capabilities();
    case SupportsMultipleFiles:
        return supportsMultipleFiles();
    default:
        return QVariant();
    }
}

QString SharingMethodInfo::displayName() const
{
    Q_D(const SharingMethodInfo);
    return d->displayName;
}

/*
   Set the name that will be visible e.g. Facebook or Bluetooth
*/
void SharingMethodInfo::setDisplayName(const QString &displayName)
{
    Q_D(SharingMethodInfo);
    d->displayName = displayName;
}

QString SharingMethodInfo::subtitle() const
{
    Q_D(const SharingMethodInfo);
    return d->subtitle;
}

/*
   Set subtitle shown next to display name, e.g. account name
*/
void SharingMethodInfo::setSubtitle(const QString &subtitle)
{
    Q_D(SharingMethodInfo);
    d->subtitle = subtitle;
}

QString SharingMethodInfo::methodId() const
{
    Q_D(const SharingMethodInfo);
    return d->methodId;
}

/*
   Set the plugin id of the share plugin e.g. "bluetooth"
*/
void SharingMethodInfo::setMethodId(const QString &methodId)
{
    Q_D(SharingMethodInfo);
    d->methodId = methodId;
}

QString SharingMethodInfo::methodIcon() const
{
    Q_D(const SharingMethodInfo);
    return d->methodIcon;
}

/*
   Set the url of the icon representing the sharing method
*/
void SharingMethodInfo::setMethodIcon(const QString &methodIcon)
{
    Q_D(SharingMethodInfo);
    d->methodIcon = methodIcon;
}

quint32 SharingMethodInfo::accountId() const
{
    Q_D(const SharingMethodInfo);
    return d->accountId;
}

/*
   Set the id of the account, needed in a case of multiple accounts
*/
void SharingMethodInfo::setAccountId(quint32 accountId)
{
    Q_D(SharingMethodInfo);
    d->accountId = accountId;
}

QString SharingMethodInfo::shareUIPath() const
{
    Q_D(const SharingMethodInfo);
    return d->shareUIPath;
}

/*
   Set the path to the share ui QML plugin. This QML file will be loaded by the share UI
*/
void SharingMethodInfo::setShareUIPath(const QString &shareUIPath)
{
    Q_D(SharingMethodInfo);
    d->shareUIPath = shareUIPath;
}

QStringList SharingMethodInfo::capabilities() const
{
    Q_D(const SharingMethodInfo);
    return d->capabilities;
}

/*
   Set the list of supported mimetypes
*/
void SharingMethodInfo::setCapabilities(const QStringList &capabilities)
{
    Q_D(SharingMethodInfo);
    d->capabilities = capabilities;
}

bool SharingMethodInfo::supportsMultipleFiles() const
{
    Q_D(const SharingMethodInfo);
    return d->supportsMultipleFiles;
}

/*
   Set whether this method can take multiple files at once
*/
void SharingMethodInfo::setSupportsMultipleFiles(bool supportsMultipleFiles)
{
    Q_D(SharingMethodInfo);
    d->supportsMultipleFiles = supportsMultipleFiles;
}
