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

#ifndef UT_MEDIATRANSFERINTERFACE_H
#define UT_MEDIATRANSFERINTERFACE_H

#include <QObject>
#include "mediatransferinterface.h"
class TestInterface: public MediaTransferInterface
{
    Q_OBJECT
public:

    QString displayName() const;
    QUrl serviceIcon() const;
    bool cancelEnabled() const;
    bool restartEnabled() const;

public slots:
    void start();
    void cancel();

public:
    friend class ut_mediatransferinterface;
};


// Ut class
class ut_mediatransferinterface: public QObject
{
    Q_OBJECT
public:

private slots:
    void init();
    void cleanup();
    void testSetMediaItem();
    void testProgress();
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    void testStatus_data();
    void testStatus();
#endif

private:
    TestInterface *tf;
};

#endif // UT_MEDIATRANSFERINTERFACE_H
