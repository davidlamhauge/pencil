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
#ifndef LAYERBG_H
#define LAYERBG_H

#include "layer.h"

class BitmapImage;
class QDir;

enum class LayerState
{
    STANDARD,   // Focused on making a parallax-effect
    REPEAT,     // Focused on forever panning backgrounds etc.
};

class LayerBG : public Layer
{
    Q_OBJECT

public:
    LayerBG(Object* object);
    ~LayerBG() override;

    QDomElement createDomElement(QDomDocument& doc) const override;
    void loadDomElement(const QDomElement& element, QString dataDirPath, ProgressCallback progressStep) override;
    Status presave(const QString& sDataFolder) override;

    LayerState getLayerState() { return mState; }
    void setLayerState(LayerState state) { mState = state; }

protected:
    Status saveKeyFrameFile(KeyFrame*, QString strPath) override;
    KeyFrame* createKeyFrame(int position, Object*) override;

private:
    void loadImageAtFrame(QString strFilePath, QPoint topLeft, int frameNumber, int startFrame,
                          int repeatLength, int endFrame, int direction, int pixels, qreal opacity);
    QString filePath(KeyFrame* key, const QDir& dataFolder) const;
    QString fileName(KeyFrame* key) const;
    bool needSaveFrame(KeyFrame* key, const QString& savePath);

    LayerState mState = LayerState::STANDARD;

};

#endif // LAYERBG_H
