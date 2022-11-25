/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include "layerbg.h"

#include <QDir>
#include <QFile>
#include "keyframe.h"
#include "bitmapimage.h"

LayerBG::LayerBG(Object *object) : Layer(object, Layer::BG_LAYER)
{
    setName(tr("BG Layer"));
}

LayerBG::~LayerBG()
{
}

QDomElement LayerBG::createDomElement(QDomDocument &doc) const
{
    QDomElement layerElem = createBaseDomElement(doc);

    foreachKeyFrame([&](KeyFrame* pKeyFrame)
    {
        BitmapImage* pImg = static_cast<BitmapImage*>(pKeyFrame);

        QDomElement imageTag = doc.createElement("image");
        imageTag.setAttribute("frame", pKeyFrame->pos());
        imageTag.setAttribute("src", fileName(pKeyFrame));
        imageTag.setAttribute("topLeftX", pImg->topLeft().x());
        imageTag.setAttribute("topLeftY", pImg->topLeft().y());
        imageTag.setAttribute("opacity", pImg->getOpacity());
        // TODO add startframe, direction, endframe, pixels
        layerElem.appendChild(imageTag);

        Q_ASSERT(QFileInfo(pKeyFrame->fileName()).fileName() == fileName(pKeyFrame));
    });

    return layerElem;
}

void LayerBG::loadDomElement(const QDomElement &element, QString dataDirPath, std::function<void ()> progressStep)
{
    this->loadBaseDomElement(element);

    QDomNode imageTag = element.firstChild();
    while (!imageTag.isNull())
    {
        QDomElement imageElement = imageTag.toElement();
        if (!imageElement.isNull())
        {
            if (imageElement.tagName() == "image")
            {
                QString path = dataDirPath + "/" + imageElement.attribute("src"); // the file is supposed to be in the data directory
                QFileInfo fi(path);
                if (!fi.exists()) path = imageElement.attribute("src");
                int position = imageElement.attribute("frame").toInt();
                int x = imageElement.attribute("topLeftX").toInt();
                int y = imageElement.attribute("topLeftY").toInt();
                qreal opacity = 1.0;
                if (imageElement.hasAttribute("opacity")) {
                    opacity = imageElement.attribute("opacity").toDouble();
                }
                // TODO add startframe, direction, endframe, pixels
                loadImageAtFrame(path, QPoint(x, y), position, opacity);

                progressStep();
            }
        }
        imageTag = imageTag.nextSibling();
    }


}

Status LayerBG::presave(const QString &sDataFolder)
{
    QDir dataFolder(sDataFolder);
    // Handles keys that have been moved but not modified
    std::vector<BitmapImage*> movedOnlyBitmaps;
    foreachKeyFrame([&movedOnlyBitmaps,&dataFolder,this](KeyFrame* key)
    {
        auto bitmap = static_cast<BitmapImage*>(key);
        // (b->fileName() != fileName(b) && !modified => the keyframe has been moved, but users didn't draw on it.
        if (!bitmap->fileName().isEmpty()
            && !bitmap->isModified()
            && bitmap->fileName() != filePath(bitmap, dataFolder))
        {
            movedOnlyBitmaps.push_back(bitmap);
        }
    });

    for (BitmapImage* b : movedOnlyBitmaps)
    {
        // Move to temporary locations first to avoid overwritting anything we shouldn't be
        // Ex: Frame A moves from 1 -> 2, Frame B moves from 2 -> 3. Make sure A does not overwrite B
        QString tmpPath = dataFolder.filePath(QString::asprintf("t_%03d.%03d.png", id(), b->pos()));
        if (QFileInfo(b->fileName()).dir() != dataFolder) {
            // Copy instead of move if the data folder itself has changed
            QFile::copy(b->fileName(), tmpPath);
        }
        else {
            QFile::rename(b->fileName(), tmpPath);
        }
        b->setFileName(tmpPath);
    }

    for (BitmapImage* b : movedOnlyBitmaps)
    {
        QString dest = filePath(b, dataFolder);
        QFile::remove(dest);

        QFile::rename(b->fileName(), dest);
        b->setFileName(dest);
    }

    return Status::OK;}

Status LayerBG::saveKeyFrameFile(KeyFrame* keyframe, QString path)
{
    QString strFilePath = filePath(keyframe, QDir(path));

    BitmapImage* bitmapImage = static_cast<BitmapImage*>(keyframe);

    bool needSave = needSaveFrame(keyframe, strFilePath);
    if (!needSave)
    {
        return Status::SAFE;
    }

    bitmapImage->setFileName(strFilePath);

    Status st = bitmapImage->writeFile(strFilePath);
    if (!st.ok())
    {
        bitmapImage->setFileName("");

        DebugDetails dd;
        dd << "LayerBitmap::saveKeyFrame";
        dd << QString("  KeyFrame.pos() = %1").arg(keyframe->pos());
        dd << QString("  strFilePath = %1").arg(strFilePath);
        dd << QString("BitmapImage could not be saved");
        dd.collect(st.details());
        return Status(Status::FAIL, dd);
    }

    bitmapImage->setModified(false);
    return Status::OK;}

KeyFrame *LayerBG::createKeyFrame(int position, Object *)
{
    BitmapImage* b = new BitmapImage;
    b->setPos(position);
    b->enableAutoCrop(true);
    return b;
}

void LayerBG::loadImageAtFrame(QString path, QPoint topLeft, int frameNumber, qreal opacity)
{
    BitmapImage* pKeyFrame = new BitmapImage(topLeft, path);
    pKeyFrame->enableAutoCrop(true);
    pKeyFrame->setPos(frameNumber);
    pKeyFrame->setOpacity(opacity);
    loadKey(pKeyFrame);
}

QString LayerBG::filePath(KeyFrame *key, const QDir &dataFolder) const
{
    return dataFolder.filePath(fileName(key));
}

QString LayerBG::fileName(KeyFrame *key) const
{
    return QString::asprintf("%03d.%03d.png", id(), key->pos());
}

bool LayerBG::needSaveFrame(KeyFrame *key, const QString &savePath)
{
    if (key->isModified()) // keyframe was modified
        return true;
    if (QFile::exists(savePath) == false) // hasn't been saved before
        return true;
    if (key->fileName().isEmpty())
        return true;
    return false;
}

