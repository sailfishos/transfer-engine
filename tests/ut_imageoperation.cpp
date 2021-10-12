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

#include "ut_imageoperation.h"
#include <qtest.h>
#include <QuillMetadata>
#include "imageoperation.h"
#include <QtDebug>
#include <QDir>

QString ut_imageoperation::createTestFile(const QString &fileName, bool writeContent)
{
    QString f = QDir::tempPath() + QDir::separator() + fileName;
    if (writeContent) {
        QFile file(f);
        file.open(QIODevice::WriteOnly);
        const char * msg = "content";
        file.write(msg, qstrlen(msg));
        file.close();
    }
    return f;
}

void ut_imageoperation::testUniqueFilePath()
{
    QStringList tempFiles;
    tempFiles << createTestFile("img_001.jpg");

    // Bad input
    QCOMPARE(ImageOperation::uniqueFilePath(QString()), QString());

    // Non existing file
    QCOMPARE(ImageOperation::uniqueFilePath(QLatin1String("/home/nemo/Pictures/img_001.jpg")), QString());

    // Existing file
    QStringList tmp;
    tmp << ImageOperation::uniqueFilePath(tempFiles.at(0));
    QCOMPARE(tmp.last().endsWith(QLatin1String("_1.jpg")), true);
    tmp.clear();

    // Add two more files
    tempFiles << createTestFile("img_002.jpg")
              << createTestFile("img_004.jpg");

    // Create couple of temp files from the same source file. Note files must exists
    // on a disk
    tmp << ImageOperation::uniqueFilePath(tempFiles.at(2));
    QFile::copy(tempFiles.at(2), tmp.last());
    QCOMPARE(tmp.last().endsWith(QLatin1String("_1.jpg")), true);

    tmp << ImageOperation::uniqueFilePath(tempFiles.at(2));
    QFile::copy(tempFiles.at(2), tmp.last());
    QCOMPARE(tmp.last().endsWith(QLatin1String("_2.jpg")), true);

    // Remove the first file. This should handle the case that we are not getting
    // a new temp file with _2.jpg as a suffix because this should take into account
    // the removed files too.
    QFile::remove(tmp.first());
    QCOMPARE(ImageOperation::uniqueFilePath(tempFiles.at(2)).endsWith("_3.jpg"), true);

    // Remove all the files generated for this test case
    foreach(QString f, tmp) {
        QFile::remove(f);
    }

    foreach(QString f, tempFiles) {
        QFile::remove(f);
    }
}


void ut_imageoperation::testScale()
{

    QImage img("images/testimage.jpg");
    QVERIFY(!img.isNull());

    QString filePath("images/testimage.jpg");

    QString target = ImageOperation::uniqueFilePath(filePath);

    // Invalid sourceFile -> fail
    QCOMPARE(ImageOperation::scaleImage("", 0.5, target), QString());

    // Valid source file, invalid scaleFactor -> fail
    QCOMPARE(ImageOperation::scaleImage(filePath, -1.0, target), QString());
    QCOMPARE(ImageOperation::scaleImage(filePath, 0, target), QString());
    QCOMPARE(ImageOperation::scaleImage(filePath, 1.0, target), QString());

    // Proper source file, proper scale factor, proper target file
    QString result = ImageOperation::scaleImage(filePath, 0.5, target);
    QCOMPARE(result, target);

    int angle;
    bool mirror;
    ImageOperation::imageOrientation(filePath, &angle, &mirror);
    QCOMPARE(angle, 90);
    QCOMPARE(mirror, false);

    QImage resImage(result);
    QTransform x;
    x.rotate(angle);
    resImage = resImage.transformed(x);

    QCOMPARE(resImage.size(), img.size()*0.5);
    QFile::remove(result);

    result = ImageOperation::scaleImage(filePath, 0.1, target);
    resImage.load(result);
    resImage = resImage.transformed(x);
    QCOMPARE(resImage.size(), img.size()*0.1);
    QFile::remove(result);

    result = ImageOperation::scaleImage(filePath, 0.9, target);
    resImage.load(result);
    resImage = resImage.transformed(x);
    QVERIFY(qAbs(resImage.width() - img.width()*0.9) < 2);
    QVERIFY(qAbs(resImage.height() - img.height()*0.9) < 2);
    QFile::remove(result);
}


void ut_imageoperation::testScaleToSize()
{
    QImage img("images/testimage.jpg");
    QVERIFY(!img.isNull());

    QString filePath("images/testimage.jpg");

    QString target = ImageOperation::uniqueFilePath(filePath);
    QVERIFY(!target.isEmpty());

    QFileInfo f("images/testimage.jpg");

    int angle;
    bool mirror;
    ImageOperation::imageOrientation(filePath, &angle, &mirror);
    QCOMPARE(angle, 90);
    QCOMPARE(mirror, false);

    QTransform x;
    x.rotate(angle);
    img = img.transformed(x);

    int targetSize = f.size() * 0.5;

    // Invalid sourceFile -> fail
    QCOMPARE(ImageOperation::scaleImageToSize("", targetSize, target), QString());

    // Valid source file, invalid targetSize -> fail
    QCOMPARE(ImageOperation::scaleImageToSize(filePath, -1, target), QString());
    QCOMPARE(ImageOperation::scaleImageToSize(filePath, 0, target), QString());
    QCOMPARE(ImageOperation::scaleImageToSize(filePath, targetSize*40, target), QString());

    // Proper source file, proper scale factor, proper target file
    QString result = ImageOperation::scaleImageToSize(filePath, targetSize, target);
    QCOMPARE(result, target);

    QImage tImg(result);
    QFileInfo f2(result);
    QVERIFY(f2.size() <= targetSize);
    QVERIFY(tImg.width() < img.width());
    QVERIFY(tImg.height() < img.height());
    QFile::remove(result);
}


void ut_imageoperation::testDropMetadata()
{
    // NOTE: The test image doesn't contain all metadata fields such as
    // title description, but it contains the most essential fields related
    // to location and the author.
    // First make sure that there is metadata
    QCOMPARE(QuillMetadata::canRead("images/testimage.jpg"), true);
    QuillMetadata md("images/testimage.jpg");
    QCOMPARE(md.isValid(), true);

    QVariant creator = md.entry(QuillMetadata::Tag_Creator);
    QVariant city    = md.entry(QuillMetadata::Tag_City);
    QVariant country = md.entry(QuillMetadata::Tag_Country);
    QVariant location= md.entry(QuillMetadata::Tag_Location);
    QVariant lat     = md.entry(QuillMetadata::Tag_GPSLatitude);
    QVariant lon     = md.entry(QuillMetadata::Tag_GPSLongitude);

    // These shound't be null for the original image
    QCOMPARE(creator.isNull(), false);
    QCOMPARE(city.isNull(), false);
    QCOMPARE(country.isNull(), false);
    QCOMPARE(location.isNull(),false);
    QCOMPARE(lat.isNull(), false);
    QCOMPARE(lon.isNull(), false);

    QString path = ImageOperation::removeImageMetadata("images/testimage.jpg");
    QVERIFY(!path.isNull());
    QCOMPARE(QFile::exists(path), true);


    QCOMPARE(QuillMetadata::canRead(path), true);
    QuillMetadata md2(path);
    QCOMPARE(md2.isValid(), true);

    creator = md2.entry(QuillMetadata::Tag_Creator);
    city    = md2.entry(QuillMetadata::Tag_City);
    country = md2.entry(QuillMetadata::Tag_Country);
    location= md2.entry(QuillMetadata::Tag_Location);
    lat     = md2.entry(QuillMetadata::Tag_GPSLatitude);
    lon     = md2.entry(QuillMetadata::Tag_GPSLongitude);

    // After removing metadata these should be fine.
    QCOMPARE(creator.isNull(), true);
    QCOMPARE(city.isNull(), true);
    QCOMPARE(country.isNull(), true);
    QCOMPARE(location.isNull(),true);
    QCOMPARE(lat.isNull(), true);
    QCOMPARE(lon.isNull(), true);

    // Remove the test image
    QFile::remove(path);
}

void ut_imageoperation::testOrientation()
{
    int angle;
    bool mirrored;
    ImageOperation::imageOrientation("images/testimage-0.jpg", &angle, &mirrored);
    QCOMPARE(angle, 0);
    QCOMPARE(mirrored, false);

    ImageOperation::imageOrientation("images/testimage-0-mirrored.jpg", &angle, &mirrored);
    QCOMPARE(angle, 0);
    QCOMPARE(mirrored, true);

    ImageOperation::imageOrientation("images/testimage-90.jpg", &angle, &mirrored);
    QCOMPARE(angle, 90);
    QCOMPARE(mirrored, false);

    ImageOperation::imageOrientation("images/testimage-90-mirrored.jpg", &angle, &mirrored);
    QCOMPARE(angle, 90);
    QCOMPARE(mirrored, true);

    ImageOperation::imageOrientation("images/testimage-180.jpg", &angle, &mirrored);
    QCOMPARE(angle, 180);
    QCOMPARE(mirrored, false);

    ImageOperation::imageOrientation("images/testimage-180-mirrored.jpg", &angle, &mirrored);
    QCOMPARE(angle, 180);
    QCOMPARE(mirrored, true);

    ImageOperation::imageOrientation("images/testimage-270.jpg", &angle, &mirrored);
    QCOMPARE(angle, 270);
    QCOMPARE(mirrored, false);

    ImageOperation::imageOrientation("images/testimage-270-mirrored.jpg", &angle, &mirrored);
    QCOMPARE(angle, 270);
    QCOMPARE(mirrored, true);
}

/*
QTEST_MAIN(ut_imageoperation)

#include "ut_imageoperation.moc"
*/
