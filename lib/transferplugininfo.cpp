/*
 * Copyright (c) 2019 Open Mobile Platform LLC.
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

#include "transferplugininfo.h"

QVariantMap TransferPluginInfo::metaData() const
{
    return property("_nemo_transferplugininfo_capabilities").toMap();
}

void TransferPluginInfo::setMetaData(const QVariantMap &metaData)
{
    setProperty("_nemo_transferplugininfo_capabilities", metaData);
}

void TransferPluginInfo::registerType()
{
    qDBusRegisterMetaType<QList<QVariantMap> >();
}
