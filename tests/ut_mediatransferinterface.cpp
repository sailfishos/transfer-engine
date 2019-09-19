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

#include "ut_mediatransferinterface.h"
#include "mediaitem.h"
#include <QtTest/QTest>
#include <QVariantMap>
#include <QtDebug>
#include <QSignalSpy>


QString TestInterface::displayName() const
{
    return "Test";
}

QUrl TestInterface::serviceIcon() const
{
    return QUrl();
}

bool TestInterface::cancelEnabled() const
{
    return true;
}

bool TestInterface::restartEnabled() const
{
    return true;
}

void TestInterface::start()
{

}

void TestInterface::cancel()
{

}

// ************************




void ut_mediatransferinterface::init()
{
    tf = new TestInterface();
    qRegisterMetaType<MediaTransferInterface::TransferStatus>("MediaTransferInterface::TransferStatus");
}

void ut_mediatransferinterface::cleanup()
{
    delete tf;
    tf = 0;
}


void ut_mediatransferinterface::testSetMediaItem()
{
    QVERIFY(tf != 0);

    MediaItem *mi = new MediaItem;
    QVariantMap map;
    map.insert("key1", 1);
    map.insert("key2", 2);
    mi->setValue(MediaItem::UserData, map);

    tf->setMediaItem(mi);
    QCOMPARE(tf->mediaItem(), mi);
    QCOMPARE(tf->mediaItem()->parent(), tf);

    QVariantMap map2 = tf->mediaItem()->value(MediaItem::UserData).toMap();
    QCOMPARE(map2.value("key1"), map.value("key1"));
    QCOMPARE(map2.value("key2"), map.value("key2"));
}

void ut_mediatransferinterface::testProgress()
{
    QVERIFY(tf != 0);

    QSignalSpy spy(tf, SIGNAL(progressUpdated(qreal)));

    QVERIFY(tf->progress() == 0);
    QCOMPARE(spy.count(), 0);

    tf->setProgress(0.01);
    QVERIFY(tf->progress() > 0);
    // Test that first signal is only emitted after the threshold value (0.05) has
    // been exceeded.
    tf->setProgress(0.02);
    tf->setProgress(0.03);
    tf->setProgress(0.04);
    tf->setProgress(0.05);
    tf->setProgress(0.06);
    QCOMPARE(spy.count(), 1);

    tf->setProgress(0.11);
    QCOMPARE(spy.count(), 2);

    tf->setProgress(0.21);
    QCOMPARE(spy.count(), 3);

    tf->setProgress(0.31);
    QCOMPARE(spy.count(), 4);

    tf->setProgress(0.41);
    QCOMPARE(spy.count(), 5);


    // Let's set status to finished -> one progressUpdated signal should be emitted
    tf->setStatus(MediaTransferInterface::TransferFinished);
    QCOMPARE(spy.count(), 6);
    QVERIFY(tf->progress() == 1);

    // Set  progress to less than 1
    tf->setProgress(0.5);
    QCOMPARE(spy.count(), 7);

    // Cancel the transfer. This is handled in the same if as handing Interrupted transfer
    tf->setStatus(MediaTransferInterface::TransferCanceled);
    QCOMPARE(spy.count(), 8);
    QVERIFY(tf->progress() == 0);
}



#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
void ut_mediatransferinterface::testStatus_data()
{
    QTest::addColumn<MediaTransferInterface::TransferStatus>("status");
    QTest::addColumn<int>("spycount");

    QTest::newRow("notstarted") << MediaTransferInterface::NotStarted << 0;
    QTest::newRow("starter") << MediaTransferInterface::TransferStarted << 1;
    QTest::newRow("canceled") << MediaTransferInterface::TransferCanceled << 1;
    QTest::newRow("interrupted") << MediaTransferInterface::TransferInterrupted << 1;
    QTest::newRow("finished") << MediaTransferInterface::TransferFinished << 1;
}

void ut_mediatransferinterface::testStatus()
{

    QVERIFY(tf != 0);
    QFETCH(MediaTransferInterface::TransferStatus, status);
    QFETCH(int, spycount);

    QSignalSpy spy(tf, SIGNAL(statusChanged(MediaTransferInterface::TransferStatus)));
    tf->setStatus(status);
    QCOMPARE(tf->status(), status);
    QCOMPARE(spy.count(), spycount);
}


Q_DECLARE_METATYPE(MediaTransferInterface::TransferStatus)
#endif
