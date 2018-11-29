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

#ifndef TRANSFERTYPES_H
#define TRANSFERTYPES_H

namespace TransferEngineData
{
    enum TransferStatus {
        Unknown,
        NotStarted,
        TransferStarted,
        TransferCanceled,
        TransferFinished,
        TransferInterrupted
    };

    enum TransferType {
        Undefined,
        Upload,
        Download,
        Sync
    };
}
#endif // TRANSFERTYPES_H
