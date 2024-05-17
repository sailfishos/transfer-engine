/*
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

#ifndef SHARINGPLUGININTERFACEV2_H
#define SHARINGPLUGININTERFACEV2_H

#include <QtPlugin>
#include "sharingplugininfov2.h"

class SharingPluginInterfaceV2
{
public:
    virtual SharingPluginInfoV2 *infoObject() = 0;

    virtual QString pluginId() const = 0;
};

Q_DECLARE_INTERFACE(SharingPluginInterfaceV2, "org.sailfishos.SharingPluginInterfaceV2/1.0")
#endif // SHARINGPLUGININTERFACEV2_H
