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

#ifndef METATYPEDECLARATIONS_H
#define METATYPEDECLARATIONS_H

#include "transferdbrecord.h"
#include "transfermethodinfo.h"
#include <QList>

Q_DECLARE_METATYPE(TransferDBRecord)
Q_DECLARE_METATYPE(QList<TransferDBRecord>)
Q_DECLARE_METATYPE(TransferMethodInfoDeprecated)
Q_DECLARE_METATYPE(QList<TransferMethodInfoDeprecated>)
Q_DECLARE_METATYPE(TransferMethodInfo)
Q_DECLARE_METATYPE(QList<TransferMethodInfo>)


#endif // METATYPEDECLARATIONS_H
