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

#include "imageoperation.h"
#include <QuillMetadata>

#include <QFileInfo>
#include <QtDebug>
#include <QImageReader>
#include <QSize>
#include <QtCore/qmath.h>
/*!
    \class ImageOperation
    \brief The ImageOperation class is a helper class to manipulate images.

    This class is meant to be used by share plugins. It can be used for:
    \list
        \li Removing image metadata
        \li Scaling image
        \li Create a temp files from the image paths
    \endlist
*/

/*!
    Creates a file path from the \a sourceFilePath to the \a path location.
    If the path is not given it uses system default temp path e.g. '/var/tmp'.
    This function uses sourceFilePath as a template to generate a temp file name.

    NOTE: This function doesn't create the file, it only generates the file path to
    the temporary file.

    Temporary file will be e.g:

    Source file: "/home/nemo/Pictures/img_001.jpg"
    Temporary file: "/var/tmp/img_001_0.jpg"
 */
QString ImageOperation::uniqueFilePath(const QString &sourceFilePath, const QString &path)
{
    if (sourceFilePath.isEmpty() || !QFile::exists(sourceFilePath)) {
        qWarning() << Q_FUNC_INFO << sourceFilePath << "Doesn't exist or then the path is empty!";
        return QString();
    }

    if (path.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "'path' argument is empty!";
        return QString();
    }

    QFileInfo fileInfo(sourceFilePath);

    // Construct target temp file path first:
    QDir dir(path);
    QStringList prevFiles = dir.entryList(QStringList() << fileInfo.baseName() + QLatin1String("*"), QDir::Files);
    int count = prevFiles.count();

    // Create temp file with increasing index in a file name e.g.
    // /var/temp/img_001_0.jpg, /var/temp/img_001_1.jpg
    // In a case there already is a file with the same filename
    QString filePath = dir.absolutePath() + QDir::separator();
    QString fileName =  fileInfo.baseName() +
                        QLatin1String("_") +
                        QString::number(count) +
                        QLatin1String(".") +
                        fileInfo.suffix();

    // This makes sure that we don't generate a file name which already exists. E.g. there are files:
    // img_001_0, img_001_1, img_001_2 and img_001 gets deleted. Then this code would generate a
    // filename img_001_2 which already exists
    while(prevFiles.contains(fileName)) {
        ++count;
        fileName =  fileInfo.baseName() +
                    QLatin1String("_") +
                    QString::number(count) +
                    QLatin1String(".") +
                    fileInfo.suffix();
    }

    return filePath + fileName;
}

/*!
   Helper method to remove metadata from jpeg files. Only author and location related
   metadata will be removed. \a sourceFile is the path to the original file.

   Returns a path to the copy of the image with metadata removed.
 */
QString ImageOperation::removeImageMetadata(const QString &sourceFile)
{

    if (!QuillMetadata::canRead(sourceFile)) {
        qWarning() << Q_FUNC_INFO << "Can't read the source: " << sourceFile;
        return QString();
    }

    QString targetFile = uniqueFilePath(sourceFile);

    // Copy image content first
    if (!QFile::copy(sourceFile, targetFile)) {
        qWarning() << Q_FUNC_INFO << "Failed to copy content!";
        return QString();
    }

    // Get metadata and remove it
    QuillMetadata md(sourceFile);
    if(!md.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid metadata";
        return QString();
    }

    // Remove bunch of metadata entries
    md.removeEntry(QuillMetadata::Tag_Creator);
    md.removeEntry(QuillMetadata::Tag_Subject);
    md.removeEntry(QuillMetadata::Tag_Title);
    md.removeEntry(QuillMetadata::Tag_City);
    md.removeEntry(QuillMetadata::Tag_Country);
    md.removeEntry(QuillMetadata::Tag_Location);
    md.removeEntry(QuillMetadata::Tag_Description);
    md.removeEntry(QuillMetadata::Tag_Regions);
    md.removeEntry(QuillMetadata::Tag_Timestamp);
    md.removeEntries(QuillMetadata::TagGroup_GPS);

    // Write modified metadata to the target file
    if (!md.write(targetFile)) {
        qWarning() << Q_FUNC_INFO << "Failed to clear metadata!";
        return QString();
    }
    // Return new file with removed metadata
    return targetFile;
}

/*!
    Scale image \a sourceFile using \a scaleFactor. The scaled image is stored to the \a targetFile or
    if \a targetFile is not given, then a temporary file is created for saving.

    The \a scaleFactor argument must be greater than 0 and less than 1. This function returns path to the
    scaled image. Note that if user doesn't specify \a targetFile the scaled image is stored under temp
    directory. Nothing guarantees that created file will remain in that diretory forewer so the caller is
    reponsible of copying file for more permanent storing.

    Returns a path to the scaled image.

    It is also recommended that if the caller doesn't use the scaled file, which is stored to the temp
    directory later, the caller should remove the file.
 */
QString ImageOperation::scaleImage(const QString &sourceFile, qreal scaleFactor, const QString &targetFile)
{
    if ( scaleFactor <= 0.0  || 1.0 <= scaleFactor) {
        qWarning() << Q_FUNC_INFO << "Argument scaleFactor needs to be 0 < scale factor < 1";
        return QString();
    }

    if (!QFile::exists(sourceFile)) {
        qWarning() << Q_FUNC_INFO << sourceFile << "doesn't exist!";
        return QString();
    }

    QString tmpFile = targetFile;
    if (tmpFile.isEmpty()) {
        tmpFile = uniqueFilePath(sourceFile);
    }

    // Using just basic QImage scale here. We can easily replace this implementation later, if we notice
    // performance bottlenecks here.
    QImageReader ir(sourceFile);
    if (!ir.canRead()) {
        qWarning() << Q_FUNC_INFO << "Couldn't read source image!";
        return QString();
    }

    QSize imageSize(ir.size());
    imageSize = imageSize.scaled(imageSize * scaleFactor, Qt::KeepAspectRatio);
    ir.setScaledSize(imageSize);
    QImage image = ir.read();

    int angle;
    bool mirrored;
    imageOrientation(sourceFile, &angle, &mirrored);

    if (mirrored) {
        image = image.mirrored(true, false);
    }

    if (angle != 0) {
        QTransform transform;
        transform.rotate(angle);
        image = image.transformed(transform);
    }

    if (!image.save(tmpFile)) {
        qWarning() << Q_FUNC_INFO
                   << "Failed to save scaled image to temp file!"
                   << tmpFile;
        return QString();
    }

    return tmpFile;
}

/*!
    Scale image from a \a sourceFile to the \a targetSize. If user gives \a targetFile argument, it is used for
    saving the scaled image to that location.

    NOTE: It's hard to predict what will be the actual file size after scaling because it depends on image data.
          Therefore it's good to remember that \a targetSize is used by this algorithm, but the final file size
          may varie a lot.

    Returns a path to the scaled image.
 */
QString ImageOperation::scaleImageToSize(const QString &sourceFile, quint64 targetSize, const QString &targetFile)
{
    if (targetSize == 0) {
        qWarning() << Q_FUNC_INFO << "Target size is 0. Can't scale image to 0 size!";
        return QString();
    }

    if (!QFile::exists(sourceFile)) {
        qWarning() << Q_FUNC_INFO << sourceFile << "doesn't exist!";
        return QString();
    }

    QFileInfo f(sourceFile);
    quint64 originalSize = f.size();
    if (originalSize <= targetSize) {
        return QString();
    }

    QString tmpFile = targetFile;
    if (tmpFile.isEmpty()) {
        tmpFile = uniqueFilePath(sourceFile);
    }

    QImageReader ir(sourceFile);
    if (!ir.canRead()) {
        qWarning() << Q_FUNC_INFO << "Can't read the original image!";
        return QString();
    }

    // NOTE: We really don't know the size on the disk after scaling and saving
    //
    // So this is a home made algorithm to downscale image based on give target size using the following
    // logic:
    //
    // 1) First we figure out magic number (a) from the original image size (s) and width (w) and height(h).
    //    Magic number is basically a combination of the image depth (bits per pixel) and compression:
    //    a =  s / (w * h)
    //
    // 2) We want the image to be the same aspect ratio (r) than the original image.
    //    r = w / h
    //
    // 3) Calculate the new width based on the following formula, where s' is the target size
    //    w * h * a = s'            =>
    //    w * w / r * a = s'        =>
    //    w * w = (s' * r) / a      =>
    //    w = sqrt( (s' * r) / a )

    qint32  w = ir.size().width();              // Width
    qint32  h = ir.size().height();             // Height
    qreal   r = w / (h * 1.0);                  // Aspect ratio
    qreal   a = originalSize / (w * h * 1.0);   // The magic number, which combines depth and compression

    qint32 newWidth = qSqrt((targetSize * r) / a);
    qint32 newHeight = newWidth / r;

    QSize imageSize(ir.size());
    imageSize = imageSize.scaled(newWidth, newHeight, Qt::KeepAspectRatio);
    ir.setScaledSize(imageSize);
    QImage image = ir.read();

    if (image.isNull()) {
        qWarning() << Q_FUNC_INFO
                   << "NULL image";
        return QString();
    }
    // Make sure orientation is right.
    int angle;
    bool mirrored;
    imageOrientation(sourceFile, &angle, &mirrored);

    if (mirrored) {
        image = image.mirrored(true, false);
    }

    if (angle != 0) {
        QTransform transform;
        transform.rotate(angle);
        image = image.transformed(transform);
    }

    if (!image.save(tmpFile)) {
        qWarning() << Q_FUNC_INFO
                   << "Failed to save scaled image to temp file!"
                   << tmpFile;
        return QString();
    }

    return tmpFile;
}

void ImageOperation::imageOrientation(const QString &sourceFile, int *angle, bool *mirror)
{
    *angle = 0;
    *mirror = false;

    if(!QuillMetadata::canRead(sourceFile)) {
        qWarning() << Q_FUNC_INFO << "Can't read metadata";
        return;
    }
    QuillMetadata md(sourceFile);
    if (!md.hasExif()) {
        qWarning() << "Metadata invalid";
        return;
    }

    int exifOrientation = md.entry(QuillMetadata::Tag_Orientation).toInt();
    switch (exifOrientation) {
    case 1: *angle = 0  ; *mirror = false; break;
    case 2: *angle = 0  ; *mirror = true ; break;
    case 3: *angle = 180; *mirror = false; break;
    case 4: *angle = 180; *mirror = true ; break;
    case 5: *angle = 90 ; *mirror = true ; break;
    case 6: *angle = 90 ; *mirror = false; break;
    case 7: *angle = 270; *mirror = true ; break;
    case 8: *angle = 270; *mirror = false; break;
    default: break;
    }
}
