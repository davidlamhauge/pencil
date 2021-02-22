/*

Pencil2D - Traditional Animation Software
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#ifndef CAMERA_H
#define CAMERA_H

#include <QTransform>
#include "keyframe.h"


class Camera : public KeyFrame
{
public:
    explicit Camera();
    explicit Camera(QPointF translation, qreal rotation, qreal scaling);
    explicit Camera(const Camera&);
    ~Camera() override;

    Camera* clone() override;

    QTransform getView();
    void reset();
    void updateViewTransform();
    void assign(const Camera& rhs);

    void translate(qreal dx, qreal dy);
    void translate(const QPointF);
    QPointF translation() { return mTranslate; }

    void rotate(qreal degree);
    qreal rotation() { return mRotate; }

    void scale(qreal scaleValue);
    void scaleWithOffset(qreal scaleValue, QPointF offset); // for zooming at the mouse position
    qreal scaling() { return mScale; }

    QTransform view;

    bool operator==(const Camera& rhs) const;

    void setDistance(qreal dist) { mDistance = dist; }
    qreal getDistance() { return mDistance; }

    void setAperture(qreal ap) { mAperture = ap; }
    qreal getAperture() { return mAperture; }

    void setFocalLength(qreal fln) { mFocalLength = fln; }
    qreal getFocalLength() { return mFocalLength; }

private:
    QPointF mTranslate;
    qreal mRotate = 0.;
    qreal mScale = 1.;

    // settings for distance, aperture and focal length
    qreal mDistance = 10.f;     // Range: 0.1-10000 m, 10 m as standard for camera and all new layers
    qreal mAperture = 8.f;      // Can be 2.8-4-5.6-8-11-22. CAN NOT CHANGE in scene/one pclx-file!
    qreal mFocalLength = 50.f;  // Range: 30-300 mm, 50 mm as standard

    bool mNeedUpdateView = true;
};

#endif // CAMERA_H
