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

#ifndef IMAGEOPERATION_H
#define IMAGEOPERATION_H

#include <QString>
#include <QDir>

class ImageOperation
{
public:
    static QString uniqueFilePath(const QString &sourceFilePath, const QString &path = QDir::tempPath());
    static QString removeImageMetadata(const QString &sourceFile);
    static QString scaleImage(const QString &sourceFile, qreal scaleFactor, const QString &targetFile=QString());
    static QString scaleImageToSize(const QString &sourceFile, quint64 targetSize, const QString &targetFile=QString());
    static void imageOrientation(const QString &sourceFile, int *angle, bool *mirror);
};

#endif // IMAGEOPERATION_H
