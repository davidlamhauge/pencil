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
#ifndef LAYERCAMERA_H
#define LAYERCAMERA_H

#include <QRect>
#include "layer.h"
#include "movemode.h"

class Camera;

class LayerCamera : public Layer
{
    Q_OBJECT

public:
    LayerCamera(Object* object);
    ~LayerCamera() override;

    void loadImageAtFrame(int frame, qreal dx, qreal dy, qreal rotate, qreal scale);

    QDomElement createDomElement(QDomDocument& doc) const override;
    void loadDomElement(const QDomElement& element, QString dataDirPath, ProgressCallback progressStep) override;

    Camera* getCameraAtFrame(int frameNumber);
    Camera* getLastCameraAtFrame(int frameNumber, int increment);
    QTransform getViewAtFrame(int frameNumber);
    MoveMode getMoveModeForCamera(QPointF point, qreal tolerance);
    void transformCameraView(int frame, MoveMode mode, QPointF point);
    void updateCamRects(int frame);

    qreal mAspectRatio = 0.75; // Aspect ratio for 800x600
    QRect getViewRect();
    QRect getCurrentRect() { return mCurrentRect; }
    QSize getViewSize();
    void setOffsetPoint(QPointF offset) { mOffsetPoint = offset; }
    QPointF getOffsetPoint() { return mOffsetPoint; }

    void ifObjectLoaded(int currentFrame);
    void setViewRect(QRect newViewRect);

signals:
    void resolutionChanged();

protected:
    Status saveKeyFrameFile(KeyFrame*, QString path) override;
    KeyFrame* createKeyFrame(int position, Object*) override;

private:
    void linearInterpolateTransform(Camera*);

    QPointF mOffsetPoint = QPointF();

    int mFieldW = 800;
    int mFieldH = 600;
    QRect viewRect;         // camera output size, as in Settings
    QRect mCurrentRect;     // current rect on canvas. Can be greater, equal to or less than viewRect
};

#endif
