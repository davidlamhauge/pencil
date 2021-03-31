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
#include "buckettool.h"

#include <QPixmap>
#include <QPainter>
#include <QtMath>
#include <QSettings>
#include "pointerevent.h"

#include "layer.h"
#include "layervector.h"
#include "layerbitmap.h"
#include "layermanager.h"
#include "colormanager.h"
#include "strokemanager.h"
#include "viewmanager.h"
#include "vectorimage.h"
#include "editor.h"
#include "scribblearea.h"


BucketTool::BucketTool(QObject* parent) : StrokeTool(parent)
{
}

ToolType BucketTool::type()
{
    return BUCKET;
}

void BucketTool::loadSettings()
{
    mPropertyEnabled[TOLERANCE] = true;
    mPropertyEnabled[WIDTH] = true;
    mPropertyEnabled[BLEEDFILL] = true;
    mPropertyEnabled[EXPANDFILL] = true;

    QSettings settings(PENCIL2D, PENCIL2D);

    properties.width = settings.value("fillThickness", 4.0).toDouble();
    properties.feather = 10;
    properties.stabilizerLevel = StabilizationLevel::NONE;
    properties.useAA = DISABLED;
    properties.tolerance = settings.value("tolerance", 32.0).toDouble();
    properties.useBleedFill = settings.value("bleedFillOn", false).toBool();
    properties.useExpandFill = settings.value("expandFillOn", false).toBool();
}

void BucketTool::resetToDefault()
{
    setWidth(4.0);
    setTolerance(32.0);
    setUseBleedFill(false);
    setUseExpandFill(false);
}

QCursor BucketTool::cursor()
{
    if (mEditor->preference()->isOn(SETTING::TOOL_CURSOR))
    {
        QPixmap pixmap(":icons/bucketTool.png");
        QPainter painter(&pixmap);
        painter.end();

        return QCursor(pixmap, 4, 20);
    }
    else
    {
        return QCursor(QPixmap(":icons/cross.png"), 10, 10);
    }
}

/**
 * @brief BrushTool::setWidth
 * @param width
 * set fill thickness
 */
void BucketTool::setWidth(const qreal width)
{
    // Set current property
    properties.width = width;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("fillThickness", width);
    settings.sync();
}

void BucketTool::setUseBleedFill(const bool bleedFill)
{
    // Set current property
    properties.useBleedFill = bleedFill;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("bleedFillOn", bleedFill);
    settings.sync();
}

void BucketTool::setBleedFillLayer(const QString name)
{
    properties.bleedFillLayer = name;
}

void BucketTool::setUseExpandFill(const bool expandFill)
{
    // Set current property
    properties.useExpandFill = expandFill;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("expandFillOn", expandFill);
    settings.sync();
}

void BucketTool::setExpandFillSize(const int size)
{
    // Set current property
    properties.expandFillSize = size;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("expandFillSize", size);
    settings.sync();
}

void BucketTool::setTolerance(const int tolerance)
{
    // Set current property
    properties.tolerance = tolerance;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("tolerance", tolerance);
    settings.sync();
}

void BucketTool::pointerPressEvent(PointerEvent* event)
{
    startStroke(event->inputType());
}

void BucketTool::pointerMoveEvent(PointerEvent* event)
{
    if (event->buttons() & Qt::LeftButton && event->inputType() == mCurrentInputType)
    {
        Layer* layer = mEditor->layers()->currentLayer();
        if (layer->type() == Layer::VECTOR)
        {
            drawStroke();
        }
    }
}

void BucketTool::pointerReleaseEvent(PointerEvent* event)
{
    if (event->inputType() != mCurrentInputType) return;

    Layer* layer = editor()->layers()->currentLayer();
    if (layer == nullptr) { return; }

    if (event->button() == Qt::LeftButton)
    {
        mEditor->backup(typeName());

        switch (layer->type())
        {
        case Layer::BITMAP: paintBitmap(layer); break;
        case Layer::VECTOR: paintVector(layer); break;
        default:
            break;
        }
    }
    endStroke();
}

bool BucketTool::startAdjusting(Qt::KeyboardModifiers modifiers, qreal argStep)
{
    mQuickSizingProperties.clear();
    if (mEditor->layers()->currentLayer()->type() == Layer::VECTOR)
    {
        mQuickSizingProperties.insert(Qt::ShiftModifier, WIDTH);
    }
    else
    {
        mQuickSizingProperties.insert(Qt::ControlModifier, TOLERANCE);
    }
    return BaseTool::startAdjusting(modifiers, argStep);
}

void BucketTool::paintBitmap(Layer* layer)
{
    Layer* targetLayer = layer; // by default
    int layerNumber = editor()->layers()->currentLayerIndex(); // by default

    BitmapImage* targetImage = static_cast<LayerBitmap*>(targetLayer)->getLastBitmapImageAtFrame(editor()->currentFrame(), 0);
    if (targetImage == nullptr) { return; } // Can happen if the first frame is deleted while drawing

    QPoint point = QPoint(qFloor(getLastPoint().x()), qFloor(getLastPoint().y()));
    QRect cameraRect = mScribbleArea->getCameraRect().toRect();
    BitmapImage::floodFill(targetImage,
                           cameraRect,
                           point,
                           qPremultiply(mEditor->color()->frontColor().rgba()),
                           properties.tolerance);

    mScribbleArea->setModified(layerNumber, mEditor->currentFrame());
}

void BucketTool::paintVector(Layer* layer)
{
    mScribbleArea->clearBitmapBuffer();

    VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
    if (vectorImage == nullptr) { return; } // Can happen if the first frame is deleted while drawing

    if (!vectorImage->isPathFilled())
    {
        vectorImage->fillSelectedPath(mEditor->color()->frontColorNumber());
    }

    vectorImage->applyWidthToSelection(properties.width);
    vectorImage->applyColorToSelectedCurve(mEditor->color()->frontColorNumber());
    vectorImage->applyColorToSelectedArea(mEditor->color()->frontColorNumber());

    applyChanges();

    mScribbleArea->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
}

void BucketTool::applyChanges()
{
    mScribbleArea->applyTransformedSelection();
}

void BucketTool::drawStroke()
{
    StrokeTool::drawStroke();

    if (properties.stabilizerLevel != strokeManager()->getStabilizerLevel())
    {
        strokeManager()->setStabilizerLevel(properties.stabilizerLevel);
    }

    QList<QPointF> p = strokeManager()->interpolateStroke();

    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::VECTOR)
    {
        mCurrentWidth = 30;
        int rad = qRound((mCurrentWidth / 2 + 2) * mEditor->view()->scaling());

        QColor pathColor = qPremultiply(mEditor->color()->frontColor().rgba());
        //pathColor.setAlpha(255);

        QPen pen(pathColor,
                 mCurrentWidth * mEditor->view()->scaling(),
                 Qt::NoPen,
                 Qt::RoundCap,
                 Qt::RoundJoin);

        if (p.size() == 4)
        {
            QPainterPath path(p[0]);
            path.cubicTo(p[1], p[2], p[3]);
            mScribbleArea->drawPath(path, pen, Qt::NoBrush, QPainter::CompositionMode_Source);
            mScribbleArea->refreshVector(path.boundingRect().toRect(), rad);
        }
    }
}
