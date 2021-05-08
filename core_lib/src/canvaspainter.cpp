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

#include "canvaspainter.h"

#include <QtMath>
#include "object.h"
#include "layerbitmap.h"
#include "layervector.h"
#include "bitmapimage.h"
#include "layercamera.h"
#include "vectorimage.h"
#include "util.h"
#include "camera.h"


CanvasPainter::CanvasPainter()
{
}

CanvasPainter::~CanvasPainter()
{
}

void CanvasPainter::setCanvas(QPixmap* canvas)
{
    Q_ASSERT(canvas);
    mCanvas = canvas;
}

void CanvasPainter::setViewTransform(const QTransform view, const QTransform viewInverse)
{
    if (mViewTransform != view || mViewInverse != viewInverse) {
        mViewTransform = view;
        mViewInverse = viewInverse;
    }
}

void CanvasPainter::setTransformedSelection(QRect selection, QTransform transform)
{
    // Make sure that the selection is not empty
    if (selection.width() > 0 && selection.height() > 0)
    {
        mSelection = selection;
        mSelectionTransform = transform;
        mRenderTransform = true;
    }
    else
    {
        // Otherwise we shouldn't be in transformation mode
        ignoreTransformedSelection();
    }
}

void CanvasPainter::ignoreTransformedSelection()
{
    mRenderTransform = false;
}

void CanvasPainter::paintCached()
{
    QPixmap tempPixmap(mCanvas->size());
    tempPixmap.fill(Qt::transparent);
    mCanvas->fill(Qt::transparent);
    QPainter tempPainter;
    QPainter painter;
    initializePainter(tempPainter, tempPixmap);
    initializePainter(painter, *mCanvas);

    if (!mPreLayersCache)
    {
        renderPreLayers(painter);
        mPreLayersCache.reset(new QPixmap(*mCanvas));
    }
    else
    {
        painter.setWorldMatrixEnabled(false);
        painter.drawPixmap(0, 0, *(mPreLayersCache.get()));
        painter.setWorldMatrixEnabled(true);
    }

    renderCurLayer(painter);

    if (!mPostLayersCache)
    {
        renderPostLayers(tempPainter);
        mPostLayersCache.reset(new QPixmap(tempPixmap));
        painter.setWorldMatrixEnabled(false);
        painter.drawPixmap(0, 0, tempPixmap);
        painter.setWorldMatrixEnabled(true);
    }
    else
    {
        painter.setWorldMatrixEnabled(false);
        painter.drawPixmap(0, 0, *(mPostLayersCache.get()));
        painter.setWorldMatrixEnabled(true);
    }
}

void CanvasPainter::resetLayerCache()
{
    mPreLayersCache.reset();
    mPostLayersCache.reset();
}

void CanvasPainter::initializePainter(QPainter& painter, QPixmap& pixmap)
{
    painter.begin(&pixmap);
    painter.setWorldMatrixEnabled(true);
    painter.setWorldTransform(mViewTransform);
}

void CanvasPainter::renderPreLayers(QPixmap* pixmap)
{
    QPainter painter;
    initializePainter(painter, *pixmap);
    renderPreLayers(painter);
}

void CanvasPainter::renderPreLayers(QPainter& painter)
{
    if (mOptions.eLayerVisibility != LayerVisibility::CURRENTONLY || mObject->getLayer(mCurrentLayerIndex)->type() == Layer::CAMERA)
    {
        paintCurrentFrame(painter, 0, mCurrentLayerIndex - 1);
    }

    paintOnionSkin(painter);
    painter.setOpacity(1.0);
}

void CanvasPainter::renderCurLayer(QPixmap* pixmap)
{
    QPainter painter;
    initializePainter(painter, *pixmap);
    renderCurLayer(painter);
}

void CanvasPainter::renderCurLayer(QPainter& painter)
{
    paintCurrentFrame(painter, mCurrentLayerIndex, mCurrentLayerIndex);
}

void CanvasPainter::renderPostLayers(QPixmap* pixmap)
{
    QPainter painter;
    initializePainter(painter, *pixmap);
    renderPostLayers(painter);
}

void CanvasPainter::renderPostLayers(QPainter& painter)
{
    if (mOptions.eLayerVisibility != LayerVisibility::CURRENTONLY || mObject->getLayer(mCurrentLayerIndex)->type() == Layer::CAMERA)
    {
        paintCurrentFrame(painter, mCurrentLayerIndex + 1, mObject->getLayerCount() - 1);
    }

    // post effects
    if (mOptions.bAxis)
    {
        paintAxis(painter);
    }
}

void CanvasPainter::setPaintSettings(const Object* object, int currentLayer, int frame, QRect rect, BitmapImage* buffer)
{
    Q_UNUSED(rect);
    Q_ASSERT(object);
    mObject = object;

    CANVASPAINTER_LOG("Set CurrentLayerIndex = %d", currentLayer);
    mCurrentLayerIndex = currentLayer;
    mFrameNumber = frame;
    mBuffer = buffer;
}

void CanvasPainter::paint()
{
    QPainter painter;
    initializePainter(painter, *mCanvas);

    renderPreLayers(painter);
    renderCurLayer(painter);
    renderPostLayers(painter);
}

void CanvasPainter::paintBackground()
{
    mCanvas->fill(Qt::transparent);
}

void CanvasPainter::paintOnionSkin(QPainter& painter)
{
    if (!mOptions.onionWhilePlayback && mOptions.isPlaying) { return; }

    Layer* layer = mObject->getLayer(mCurrentLayerIndex);

    if (layer->visible() == false)
        return;

    if (layer->keyFrameCount() == 0)
        return;

    qreal minOpacity = static_cast<qreal>(mOptions.fOnionSkinMinOpacity / 100);
    qreal maxOpacity = static_cast<qreal>(mOptions.fOnionSkinMaxOpacity / 100);

    if (mOptions.bPrevOnionSkin && mFrameNumber > 1)
    {
        // Paint onion skin before current frame.
        qreal prevOpacityIncrement = (maxOpacity - minOpacity) / mOptions.nPrevOnionSkinCount;
        qreal opacity = maxOpacity;

        int onionFrameNumber = mFrameNumber;
        if (mOptions.bIsOnionAbsolute)
        {
            onionFrameNumber = layer->getPreviousFrameNumber(onionFrameNumber + 1, true);
        }
        onionFrameNumber = layer->getPreviousFrameNumber(onionFrameNumber, mOptions.bIsOnionAbsolute);

        int onionPosition = 0;

        while (onionPosition < mOptions.nPrevOnionSkinCount && onionFrameNumber > 0)
        {
            painter.setOpacity(opacity);

            switch (layer->type())
            {
            case Layer::BITMAP: { paintBitmapFrame(painter, layer, onionFrameNumber, mOptions.bColorizePrevOnion, false, false); break; }
            case Layer::VECTOR: { paintVectorFrame(painter, layer, onionFrameNumber, mOptions.bColorizePrevOnion, false, false); break; }
            default: break;
            }
            opacity = opacity - prevOpacityIncrement;

            onionFrameNumber = layer->getPreviousFrameNumber(onionFrameNumber, mOptions.bIsOnionAbsolute);
            onionPosition++;
        }
    }

    if (mOptions.bNextOnionSkin)
    {
        // Paint onion skin after current frame.
        qreal nextOpacityIncrement = (maxOpacity - minOpacity) / mOptions.nNextOnionSkinCount;
        qreal opacity = maxOpacity;

        int onionFrameNumber = layer->getNextFrameNumber(mFrameNumber, mOptions.bIsOnionAbsolute);
        int onionPosition = 0;

        while (onionPosition < mOptions.nNextOnionSkinCount && onionFrameNumber > 0)
        {
            painter.setOpacity(opacity);

            switch (layer->type())
            {
            case Layer::BITMAP: { paintBitmapFrame(painter, layer, onionFrameNumber, mOptions.bColorizeNextOnion, false, false); break; }
            case Layer::VECTOR: { paintVectorFrame(painter, layer, onionFrameNumber, mOptions.bColorizeNextOnion, false, false); break; }
            default: break;
            }
            opacity = opacity - nextOpacityIncrement;

            onionFrameNumber = layer->getNextFrameNumber(onionFrameNumber, mOptions.bIsOnionAbsolute);
            onionPosition++;
        }
    }
}

void CanvasPainter::paintBitmapFrame(QPainter& painter,
                                     Layer* layer,
                                     int nFrame,
                                     bool colorize,
                                     bool useLastKeyFrame,
                                     bool isCurrentFrame)
{
#ifdef _DEBUG
    LayerBitmap* bitmapLayer = dynamic_cast<LayerBitmap*>(layer);
    Q_ASSERT(bitmapLayer);
#else
    LayerBitmap* bitmapLayer = static_cast<LayerBitmap*>(layer);
#endif

    CANVASPAINTER_LOG("    Paint Bitmap Frame = %d, UseLastKeyFrame = %d", nFrame, useLastKeyFrame);
    BitmapImage* paintedImage = nullptr;
    if (useLastKeyFrame)
    {
        paintedImage = bitmapLayer->getLastBitmapImageAtFrame(nFrame, 0);
        CANVASPAINTER_LOG("      Actual frame = %d", paintedImage->pos());
    }
    else
    {
        paintedImage = bitmapLayer->getBitmapImageAtFrame(nFrame);
    }

    if (paintedImage == nullptr) { return; }
    paintedImage->loadFile(); // Critical! force the BitmapImage to load the image
    CANVASPAINTER_LOG("        Paint Image Size: %dx%d", paintedImage->image()->width(), paintedImage->image()->height());

    const bool frameIsEmpty = (paintedImage == nullptr || paintedImage->bounds().isEmpty());
    const bool isDrawing = isCurrentFrame && mBuffer && !mBuffer->bounds().isEmpty();
    if (frameIsEmpty && !isDrawing)
    {
        CANVASPAINTER_LOG("        Early return frame %d, %d", frameIsEmpty, isDrawing);
        return;
    }

    BitmapImage paintToImage;
    paintToImage.paste(paintedImage);

    painter.setOpacity(paintedImage->getOpacity() - (1.0-painter.opacity()));
    if (isCurrentFrame)
    {
        paintToImage.paste(mBuffer, mOptions.cmBufferBlendMode);
    }

    if (colorize)
    {
        QBrush colorBrush = QBrush(Qt::transparent); //no color for the current frame

        if (nFrame < mFrameNumber)
        {
            colorBrush = QBrush(Qt::red);
        }
        else if (nFrame > mFrameNumber)
        {
            colorBrush = QBrush(Qt::blue);
        }

        paintToImage.drawRect(paintedImage->bounds(),
                              Qt::NoPen,
                              colorBrush,
                              QPainter::CompositionMode_SourceIn,
                              false);
    }

    // If the current frame on the current layer has a transformation, we apply it.
    bool shouldPaintTransform = mRenderTransform && nFrame == mFrameNumber && layer == mObject->getLayer(mCurrentLayerIndex);
    if (shouldPaintTransform)
    {
        paintToImage.clear(mSelection);
    }

    painter.setWorldMatrixEnabled(true);
    prescale(&paintToImage);
    paintToImage.paintImage(painter, mScaledBitmap, mScaledBitmap.rect(), paintToImage.bounds());

    if (shouldPaintTransform)
    {
        paintTransformedSelection(painter);
    }
//    static int cc = 0;
//    QString path = QString("C:/Temp/pencil2d/canvas-%1-%2-%3.png")
//        .arg(cc++, 3, 10, QChar('0'))
//        .arg(layer->name())
//        .arg(mFrameNumber);
//    Q_ASSERT(mCanvas->save(path));

}

void CanvasPainter::prescale(BitmapImage* bitmapImage)
{
    QImage origImage = bitmapImage->image()->copy();

    // copy content of our unmodified qimage
    // to our (not yet) scaled bitmap
    mScaledBitmap = origImage.copy();

    if (mOptions.scaling >= 1.0f)
    {
        // TODO: Qt doesn't handle huge upscaled qimages well...
        // possible solution, myPaintLib canvas renderer splits its canvas up in chunks.
    }
    else
    {
        // map to correct matrix
        QRect mappedOrigImage = mViewTransform.mapRect(bitmapImage->bounds());
        mScaledBitmap = mScaledBitmap.scaled(mappedOrigImage.size(),
                                             Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
}

void CanvasPainter::paintVectorFrame(QPainter& painter,
                                     Layer* layer,
                                     int nFrame,
                                     bool colorize,
                                     bool useLastKeyFrame,
                                     bool isCurrentFrame)
{
#ifdef _DEBUG
    LayerVector* vectorLayer = dynamic_cast<LayerVector*>(layer);
    Q_ASSERT(vectorLayer);
#else
    LayerVector* vectorLayer = static_cast<LayerVector*>(layer);
#endif

    CANVASPAINTER_LOG("Paint Onion skin vector, Frame = %d", nFrame);
    VectorImage* vectorImage = nullptr;
    if (useLastKeyFrame)
    {
        vectorImage = vectorLayer->getLastVectorImageAtFrame(nFrame, 0);
    }
    else
    {
        vectorImage = vectorLayer->getVectorImageAtFrame(nFrame);
    }
    if (vectorImage == nullptr)
    {
        return;
    }

    QImage* strokeImage = new QImage(mCanvas->size(), QImage::Format_ARGB32_Premultiplied);
    vectorImage->outputImage(strokeImage, mViewTransform, mOptions.bOutlines, mOptions.bThinLines, mOptions.bAntiAlias);

    // Go through a Bitmap image to paint the onion skin colour
    BitmapImage rasterizedVectorImage;
    rasterizedVectorImage.setImage(strokeImage);

    if (colorize)
    {
        QBrush colorBrush = QBrush(Qt::transparent); //no color for the current frame

        if (nFrame < mFrameNumber)
        {
            colorBrush = QBrush(Qt::red);
        }
        else if (nFrame > mFrameNumber)
        {
            colorBrush = QBrush(Qt::blue);
        }
        rasterizedVectorImage.drawRect(strokeImage->rect(),
                                 Qt::NoPen, colorBrush,
                                 QPainter::CompositionMode_SourceIn, false);
    }

    // Don't transform the image here as we used the viewTransform in the image output
    painter.setWorldMatrixEnabled(false);

    painter.setOpacity(vectorImage->getOpacity() - (1.0-painter.opacity()));
    if (isCurrentFrame)
    {
        // Paste buffer onto image to see stroke in realtime
        rasterizedVectorImage.paste(mBuffer, mOptions.cmBufferBlendMode);
    }

    // Paint buffer pasted on top of vector image:
    // fixes polyline not being rendered properly
    rasterizedVectorImage.paintImage(painter);
}

void CanvasPainter::paintTransformedSelection(QPainter& painter) const
{
    // Make sure there is something selected
    if (mSelection.width() == 0 || mSelection.height() == 0)
        return;

    Layer* layer = mObject->getLayer(mCurrentLayerIndex);

    if (layer->type() == Layer::BITMAP)
    {
        // Get the transformed image
        BitmapImage* bitmapImage = static_cast<LayerBitmap*>(layer)->getLastBitmapImageAtFrame(mFrameNumber, 0);
        if (bitmapImage == nullptr) { return; };
        BitmapImage transformedImage = bitmapImage->transformed(mSelection, mSelectionTransform, mOptions.bAntiAlias);

        // Paint the transformation output
        painter.setWorldMatrixEnabled(true);
        transformedImage.paintImage(painter);
    }
}

/** Paints layers within the specified range for the current frame.
 *
 *  @param painter The painter to paint to
 *  @param startLayer The first layer to paint (inclusive)
 *  @param endLayer The last layer to paint (inclusive)
 */
void CanvasPainter::paintCurrentFrame(QPainter& painter, int startLayer, int endLayer)
{
    painter.setOpacity(1.0);


    bool isCameraLayer = mObject->getLayer(mCurrentLayerIndex)->type() == Layer::CAMERA;

    for (int i = startLayer; i <= endLayer; ++i)
    {
        Layer* layer = mObject->getLayer(i);

        if (layer->visible() == false)
            continue;

        if (mOptions.eLayerVisibility == LayerVisibility::RELATED && !isCameraLayer)
        {
            painter.setOpacity(calculateRelativeOpacityForLayer(i));
        }

        CANVASPAINTER_LOG("  Render Layer[%d] %s", i, layer->name());
        switch (layer->type())
        {
        case Layer::BITMAP: { paintBitmapFrame(painter, layer, mFrameNumber, false, true, i == mCurrentLayerIndex); break; }
        case Layer::VECTOR: { paintVectorFrame(painter, layer, mFrameNumber, false, true, i == mCurrentLayerIndex); break; }
        default: break;
        }
    }
}

qreal CanvasPainter::calculateRelativeOpacityForLayer(int layerIndex) const
{
    int layerOffset = mCurrentLayerIndex - layerIndex;
    int absoluteOffset = qAbs(layerOffset);
    qreal newOpacity = 1.0;
    if (absoluteOffset != 0)
    {
        newOpacity = qPow(static_cast<qreal>(mOptions.fLayerVisibilityThreshold), absoluteOffset);
    }
    return newOpacity;
}

void CanvasPainter::paintAxis(QPainter& painter) const
{
    painter.setPen(Qt::green);
    painter.drawLine(QLineF(0, -500, 0, 500));

    painter.setPen(Qt::red);
    painter.drawLine(QLineF(-500, 0, 500, 0));
}

int round100(double f, int gridSize)
{
    return static_cast<int>(f) / gridSize * gridSize;
}

void CanvasPainter::paintGrid(QPainter& painter) const
{
    int gridSizeW = mOptions.nGridSizeW;
    int gridSizeH = mOptions.nGridSizeH;

    QRectF rect = painter.viewport();
    QRectF boundingRect = mViewTransform.inverted().mapRect(rect);

    int left = round100(boundingRect.left(), gridSizeW) - gridSizeW;
    int right = round100(boundingRect.right(), gridSizeW) + gridSizeW;
    int top = round100(boundingRect.top(), gridSizeH) - gridSizeH;
    int bottom = round100(boundingRect.bottom(), gridSizeH) + gridSizeH;

    QPen pen(Qt::lightGray);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setWorldMatrixEnabled(true);
    painter.setBrush(Qt::NoBrush);
    QPainter::RenderHints previous_renderhints = painter.renderHints();
    painter.setRenderHint(QPainter::Antialiasing, false);
    // draw vertical grid lines
    for (int x = left; x < right; x += gridSizeW)
    {
        painter.drawLine(x, top, x, bottom);
    }

    // draw horizontal grid lines
    for (int y = top; y < bottom; y += gridSizeH)
    {
        painter.drawLine(left, y, right, y);
    }
    painter.setRenderHints(previous_renderhints);
}

void CanvasPainter::paintOverlayCenter(QPainter& painter, QTransform cameraTransform, QRect cameraRect) const
{
    painter.save();
    painter.setCompositionMode(QPainter::RasterOp_NotSourceAndNotDestination);

    QPen pen(Qt::DashLine);
    qreal space = 10;
    QVector<qreal> dashes;
    dashes << 10 << space << 10 << space << 10 << space;
    pen.setDashPattern(dashes);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setWorldMatrixEnabled(true);
    painter.setBrush(Qt::NoBrush);
    painter.setRenderHint(QPainter::Antialiasing, false);

    QPolygon poly = cameraTransform.inverted().mapToPolygon(cameraRect);
    QPoint centerTop = QLineF(poly.at(0), poly.at(1)).pointAt(0.5).toPoint();
    QPoint centerBottom = QLineF(poly.at(2), poly.at(3)).pointAt(0.5).toPoint();
    QPoint centerLeft = QLineF(poly.at(0), poly.at(3)).pointAt(0.5).toPoint();
    QPoint centerRight = QLineF(poly.at(1), poly.at(2)).pointAt(0.5).toPoint();
    painter.drawLine(QLineF(centerTop, centerBottom).pointAt(0.4).toPoint(),
                     QLineF(centerTop, centerBottom).pointAt(0.6).toPoint());
    painter.drawLine(QLineF(centerLeft, centerRight).pointAt(0.4).toPoint(),
                     QLineF(centerLeft, centerRight).pointAt(0.6).toPoint());

    painter.restore();
}

void CanvasPainter::paintOverlayThirds(QPainter& painter, QTransform cameraTransform, QRect cameraRect) const
{
    painter.save();
    painter.setCompositionMode(QPainter::RasterOp_NotSourceAndNotDestination);

    QPen pen(Qt::DashLine);
    qreal space = 10;
    QVector<qreal> dashes;
    dashes << 10 << space << 10 << space << 10 << space;
    pen.setDashPattern(dashes);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setWorldMatrixEnabled(true);
    painter.setBrush(Qt::NoBrush);
    QPainter::RenderHints previous_renderhints = painter.renderHints();
    painter.setRenderHint(QPainter::Antialiasing, false);

    QPolygon poly = cameraTransform.inverted().mapToPolygon(cameraRect);
    QLineF topLine(poly.at(0), poly.at(1));
    QLineF bottomLine(poly.at(3), poly.at(2));
    QLineF leftLine(poly.at(0), poly.at(3));
    QLineF rightLine(poly.at(1), poly.at(2));
    painter.drawLine(topLine.pointAt(0.333).toPoint(), bottomLine.pointAt(0.333));
    painter.drawLine(topLine.pointAt(0.667).toPoint(), bottomLine.pointAt(0.667));
    painter.drawLine(leftLine.pointAt(0.333).toPoint(), rightLine.pointAt(0.333));
    painter.drawLine(leftLine.pointAt(0.667).toPoint(), rightLine.pointAt(0.667));

    painter.setRenderHints(previous_renderhints);
    painter.restore();
}

void CanvasPainter::paintOverlayGolden(QPainter& painter, QTransform cameraTransform, QRect cameraRect) const
{
    QRect rect = cameraRect;
    if (mObject->getLayer(mCurrentLayerIndex)->type() != Layer::CAMERA)
        rect = cameraTransform.inverted().mapRect(cameraRect);

    painter.save();
    painter.setCompositionMode(QPainter::RasterOp_NotSourceAndNotDestination);

    QPen pen(Qt::DashLine);
    qreal space = 10;
    QVector<qreal> dashes;
    dashes << 10 << space << 10 << space << 10 << space;
    pen.setDashPattern(dashes);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setWorldMatrixEnabled(true);
    painter.setBrush(Qt::NoBrush);
    QPainter::RenderHints previous_renderhints = painter.renderHints();
    painter.setRenderHint(QPainter::Antialiasing, false);

    QPolygon poly = cameraTransform.inverted().mapToPolygon(cameraRect);
    QLineF topLine(poly.at(0), poly.at(1));
    QLineF bottomLine(poly.at(3), poly.at(2));
    QLineF leftLine(poly.at(0), poly.at(3));
    QLineF rightLine(poly.at(1), poly.at(2));
    painter.drawLine(topLine.pointAt(0.382).toPoint(), bottomLine.pointAt(0.382));
    painter.drawLine(topLine.pointAt(0.618).toPoint(), bottomLine.pointAt(0.618));
    painter.drawLine(leftLine.pointAt(0.382).toPoint(), rightLine.pointAt(0.382));
    painter.drawLine(leftLine.pointAt(0.618).toPoint(), rightLine.pointAt(0.618));

    painter.setRenderHints(previous_renderhints);
    painter.restore();
}

void CanvasPainter::paintOverlaySafeAreas(QPainter& painter, QTransform cameraTransform, QRect cameraRect) const
{
    QRect rect = cameraRect;
    if (mObject->getLayer(mCurrentLayerIndex)->type() != Layer::CAMERA)
        rect = cameraTransform.inverted().mapRect(cameraRect);

    painter.save();
    painter.setCompositionMode(QPainter::RasterOp_NotSourceAndNotDestination);
    QPen pen(Qt::DashLine);
    qreal space = 10;
    QVector<qreal> dashes;
    dashes << 10 << space << 10 << space << 10 << space;
    pen.setDashPattern(dashes);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setWorldMatrixEnabled(true);
    painter.setBrush(Qt::NoBrush);
    QPainter::RenderHints previous_renderhints = painter.renderHints();
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QPolygon poly = cameraTransform.inverted().mapToPolygon(cameraRect);
    QLineF topLeftCrossLine(poly.at(0), poly.at(2));
    QLineF bottomLeftCrossLine(poly.at(3), poly.at(1));

    if (mOptions.bActionSafe)
    {
        int action = mOptions.nActionSafe;
        painter.drawLine(topLeftCrossLine.pointAt((action / 2.0) / 100.0).toPoint(),
                         bottomLeftCrossLine.pointAt((100 - (action / 2.0)) / 100).toPoint());
        painter.drawLine(topLeftCrossLine.pointAt((action / 2.0) / 100.0).toPoint(),
                         bottomLeftCrossLine.pointAt((action / 2.0) / 100).toPoint());
        painter.drawLine(topLeftCrossLine.pointAt((100 - (action / 2.0)) / 100.0).toPoint(),
                         bottomLeftCrossLine.pointAt((100 - (action / 2.0)) / 100).toPoint());
        painter.drawLine(topLeftCrossLine.pointAt((100 - (action / 2.0)) / 100.0).toPoint(),
                         bottomLeftCrossLine.pointAt((action / 2.0) / 100).toPoint());

        if (mOptions.bShowSafeAreaHelperText)
        {
            painter.drawText(topLeftCrossLine.pointAt((action / 2.0) / 100.0).toPoint(), tr("Safe Action area %1 %").arg(action));
        }
    }
    if (mOptions.bTitleSafe)
    {
        int title = mOptions.nTitleSafe;
        painter.drawLine(topLeftCrossLine.pointAt((title / 2.0) / 100.0).toPoint(),
                         bottomLeftCrossLine.pointAt((100 - (title / 2.0)) / 100).toPoint());
        painter.drawLine(topLeftCrossLine.pointAt((title / 2.0) / 100.0).toPoint(),
                         bottomLeftCrossLine.pointAt((title / 2.0) / 100).toPoint());
        painter.drawLine(topLeftCrossLine.pointAt((100 - (title / 2.0)) / 100.0).toPoint(),
                         bottomLeftCrossLine.pointAt((100 - (title / 2.0)) / 100).toPoint());
        painter.drawLine(topLeftCrossLine.pointAt((100 - (title / 2.0)) / 100.0).toPoint(),
                         bottomLeftCrossLine.pointAt((title / 2.0) / 100).toPoint());

        if (mOptions.bShowSafeAreaHelperText)
        {
            painter.drawText(bottomLeftCrossLine.pointAt((title / 2.0) / 100), tr("Safe Title area %1 %").arg(title));
        }
    }

    painter.setRenderHints(previous_renderhints);
    painter.restore();
}

void CanvasPainter::renderGrid(QPainter& painter) const
{
    if (mOptions.bGrid)
    {
        painter.save();
        painter.setWorldTransform(mViewTransform);
        paintGrid(painter);
        painter.restore();
    }
}

void CanvasPainter::renderOverlays(QPainter& painter) const
{
    QTransform cameraTransform;
    QRect cameraRect;
    // Find the first visible camera layers
    for (int i = 0; i < mObject->getLayerCount(); ++i)
    {
        Layer* layer = mObject->getLayer(i);
        if (layer->type() == Layer::CAMERA && layer->visible())
        {
            LayerCamera* cameraLayer = static_cast<LayerCamera*>(layer);
            cameraTransform = cameraLayer->getViewAtFrame(mFrameNumber);
            cameraRect = cameraLayer->getViewRect();
            break;
        }
    }
    if (mOptions.bCenter)
    {
        painter.save();
        painter.setWorldTransform(mViewTransform);
        paintOverlayCenter(painter, cameraTransform, cameraRect);
        painter.restore();
    }
    if (mOptions.bThirds)
    {
        painter.save();
        painter.setWorldTransform(mViewTransform);
        paintOverlayThirds(painter, cameraTransform, cameraRect);
        painter.restore();
    }
    if (mOptions.bGoldenRatio)
    {
        painter.save();
        painter.setWorldTransform(mViewTransform);
        paintOverlayGolden(painter, cameraTransform, cameraRect);
        painter.restore();
    }
    if (mOptions.bSafeArea)
    {
        painter.save();
        painter.setWorldTransform(mViewTransform);
        paintOverlaySafeAreas(painter, cameraTransform, cameraRect);
        painter.restore();
    }

}

QRect CanvasPainter::getCameraRect()
{
    return mCameraRect;
}
