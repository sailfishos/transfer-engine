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

#include "transfermethodinfo.h"
#include "metatypedeclarations.h"

/*!
    \class TransferMethodInfo
    \brief The TransferMethodInfo class encapsulate information of a single transfer method.

    \ingroup transfer-engine-lib

    Share plugin must create a list of instances of TransferMethodInfo class to encapsulate
    information about the plugin for example filling information for the Bluetooth sharing plugin:

    \code
        QList<TransferMethodInfo> infoList;
        TransferMethodInfo info;

        QStringList capabilities;
        capabilities << QLatin1String("*");

        info.displayName     = QLatin1String("Bluetooth");
        info.userName        = "";
        info.accountId       = "";
        info.accountIcon     = QLatin1String("image://theme/icon-m-bluetooth");
        info.methodId        = QLatin1String("bluetooth");
        info.shareUIPath     = SHARE_UI_PATH + QLatin1String("/BluetoothShareUI.qml");
        info.capabilitities  = capabilities;
        infoList << info;
    \endcode
*/

/*!
    \enum TransferMethodInfo::TransferMethodInfoField

    This enum can be used for accessing different values using value() method

    \value DisplayName The name that will be visible e.g. Facebook or Bluetooth
    \value UserName User name e.g. mike.myers@gmail.com
    \value MethodId The plugin Id of the share plugin e.g. "bluetooth"
    \value AccountId The Id the account, needed in a case of multiple accounts
    \value AccountIcon The url of the icon representing the account
    \value ShareUIPath The path to the share ui QML plugin. This QML file will be loaded by the share UI
    \value Capabilities A list of supported mimetypes
*/

/*!
    Creates an instance of TransferMethodInfo.
 */
TransferMethodInfo::TransferMethodInfo()
{
}

/*!
    Assigns \a other object to this.
*/
TransferMethodInfo &TransferMethodInfo::operator=(const TransferMethodInfo &other)
{
    displayName     = other.displayName;
    userName        = other.userName;
    methodId        = other.methodId;
    shareUIPath     = other.shareUIPath;
    capabilitities  = other.capabilitities;
    accountId       = other.accountId;
    accountIcon     = other.accountIcon;
    hints           = other.hints;
    return *this;
}

/*!
    Copies \a other to this instance.
*/
TransferMethodInfo::TransferMethodInfo(const TransferMethodInfo &other):
    displayName(other.displayName),
    userName(other.userName),
    methodId(other.methodId),
    shareUIPath(other.shareUIPath),
    capabilitities(other.capabilitities),
    accountId(other.accountId),
    accountIcon(other.accountIcon),
    hints(other.hints)
{

}

/*!
    Destroys TransferMethodInfo instance.
*/
TransferMethodInfo::~TransferMethodInfo()
{

}

/*!
    Writes data from \a info to \a argument.
*/
QDBusArgument &operator<<(QDBusArgument &argument, const TransferMethodInfo &info)
{
    argument.beginStructure();
    argument << info.displayName
             << info.userName
             << info.methodId
             << info.shareUIPath
             << info.capabilitities
             << info.accountId
             << info.accountIcon
             << info.hints;

    argument.endStructure();
    return argument;
}

/*!
    Reads data from \a argument to \a info.
*/
const QDBusArgument &operator>>(const QDBusArgument &argument, TransferMethodInfo &info)
{
    argument.beginStructure();
    argument >> info.displayName
             >> info.userName
             >> info.methodId
             >> info.shareUIPath
             >> info.capabilitities
             >> info.accountId
             >> info.accountIcon
             >> info.hints;

    argument.endStructure();
    return argument;
}

/*!
    Registers TransferMethodInfo and QList<TransferMethodInfo> as DBus types.
*/
void TransferMethodInfo::registerType()
{
    qDBusRegisterMetaType<TransferMethodInfo>();
    qDBusRegisterMetaType<QList<TransferMethodInfo> >();
}

/*!
 Returns the value using the \a index. Actually the index is enum TransferMethodInfoField.
 */
QVariant TransferMethodInfo::value(int index) const
{
    switch(index) {
    case DisplayName:
        return displayName;
    case UserName:
        return userName;
    case MethodId:
        return methodId;
    case ShareUIPath:
        return shareUIPath;
    case Capabilities:
        return capabilitities;
    case AccountId:
        return accountId;
    case AccountIcon:
        return accountIcon;
   default:
        return QVariant();
    }
}

QDBusArgument &operator<<(QDBusArgument &argument, const TransferMethodInfoDeprecated &info)
{
    argument.beginStructure();
    argument << info.displayName
             << info.userName
             << info.methodId
             << info.shareUIPath
             << info.capabilitities
             << info.accountId;
    argument.endStructure();
    return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, TransferMethodInfoDeprecated &info)
{
    argument.beginStructure();
    argument >> info.displayName
             >> info.userName
             >> info.methodId
             >> info.shareUIPath
             >> info.capabilitities
             >> info.accountId;

    argument.endStructure();
    return argument;
}

void TransferMethodInfoDeprecated::registerType()
{
    qDBusRegisterMetaType<TransferMethodInfoDeprecated>();
    qDBusRegisterMetaType<QList<TransferMethodInfoDeprecated> >();
}
