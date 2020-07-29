/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2018 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include "layercamera.h"
#include "ui_camerapropertiesdialog.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QtDebug>
#include "camera.h"
#include <qsettings.h>
#include "pencildef.h"

CameraPropertiesDialog::CameraPropertiesDialog(QString name, int width, int height) :
    QDialog(),
    ui(new Ui::CameraPropertiesDialog)
{
    ui->setupUi(this);

    ui->nameBox->setText(name);
    ui->widthBox->setValue(width);
    ui->heightBox->setValue(height);
}

CameraPropertiesDialog::~CameraPropertiesDialog()
{
    delete ui;
}

QString CameraPropertiesDialog::getName()
{
    return ui->nameBox->text();
}

void CameraPropertiesDialog::setName(QString name)
{
    ui->nameBox->setText(name);
}

int CameraPropertiesDialog::getWidth()
{
    return ui->widthBox->value();
}

void CameraPropertiesDialog::setWidth(int width)
{
    ui->widthBox->setValue(width);
}

int CameraPropertiesDialog::getHeight()
{
    return ui->heightBox->value();
}

void CameraPropertiesDialog::setHeight(int height)
{
    ui->heightBox->setValue(height);
}

LayerCamera::LayerCamera(Object* object) : Layer(object, Layer::CAMERA)
{
    setName(tr("Camera Layer"));

    QSettings settings(PENCIL2D, PENCIL2D);
    mFieldW = settings.value("FieldW").toInt();
    mFieldH = settings.value("FieldH").toInt();
    if (mFieldW < 2 || mFieldH < 2)
    {
        mFieldW = 800;
        mFieldH = 600;
    }
    mAspectRatio = static_cast<qreal>(mFieldH) / static_cast<qreal>(mFieldW);
    mCurrentRect = viewRect = QRect(QPoint(-mFieldW / 2, -mFieldH / 2), QSize(mFieldW, mFieldH));
    dialog = nullptr;
}

LayerCamera::~LayerCamera()
{
}

Camera* LayerCamera::getCameraAtFrame(int frameNumber)
{
    return static_cast<Camera*>(getKeyFrameAt(frameNumber));
}

Camera* LayerCamera::getLastCameraAtFrame(int frameNumber, int increment)
{
    return static_cast<Camera*>(getLastKeyFrameAtPosition(frameNumber + increment));
}

QTransform LayerCamera::getViewAtFrame(int frameNumber)
{
    if (keyFrameCount() == 0)
    {
        return QTransform();
    }

    Camera* camera1 = static_cast<Camera*>(getLastKeyFrameAtPosition(frameNumber));

    int nextFrame = getNextKeyFramePosition(frameNumber);
    Camera* camera2 = static_cast<Camera*>(getLastKeyFrameAtPosition(nextFrame));

    if (camera1 == nullptr && camera2 == nullptr)
    {
        return QTransform();
    }
    else if (camera1 == nullptr && camera2 != nullptr)
    {
        return camera2->view;
    }
    else if (camera2 == nullptr && camera1 != nullptr)
    {
        return camera1->view;
    }

    if (camera1 == camera2)
    {
        return camera1->view;
    }

    double frame1 = camera1->pos();
    double frame2 = camera2->pos();

    // linear interpolation
    qreal c2 = (frameNumber - frame1) / (frame2 - frame1);
    qreal c1 = 1.0 - c2;

    auto interpolation = [=](double f1, double f2) -> double
    {
        return f1 * c1 + f2 * c2;
    };

    return QTransform(interpolation(camera1->view.m11(), camera2->view.m11()),
                      interpolation(camera1->view.m12(), camera2->view.m12()),
                      interpolation(camera1->view.m21(), camera2->view.m21()),
                      interpolation(camera1->view.m22(), camera2->view.m22()),
                      interpolation(camera1->view.dx(), camera2->view.dx()),
                      interpolation(camera1->view.dy(), camera2->view.dy()));

}

MoveMode LayerCamera::getMoveModeForCamera(QPointF point, qreal tolerance)
{
    if (QLineF(point, mCurrentRect.topLeft()).length() < tolerance)
    {
        return  MoveMode::TOPLEFT;
    }
    else if (QLineF(point, mCurrentRect.topRight()).length() < tolerance)
    {
        return  MoveMode::TOPRIGHT;
    }
    else if (QLineF(point, mCurrentRect.bottomLeft()).length() < tolerance)
    {
        return  MoveMode::BOTTOMLEFT;
    }
    else if (QLineF(point, mCurrentRect.bottomRight()).length() < tolerance)
    {
        return  MoveMode::BOTTOMRIGHT;
    }
    else if (QLineF(point, QPointF(mCurrentRect.right(), mCurrentRect.y() + mCurrentRect.height()/2)).length() < tolerance)
    {
        return  MoveMode::ROTATION;
    }
    else if (mCurrentRect.contains(point.toPoint()))
    {
        return  MoveMode::CENTER;
    }
    return MoveMode::NONE;
}

void LayerCamera::transformCameraView(int frame, MoveMode mode, QPointF point)
{
    switch (mode) {
    case MoveMode::CENTER:
        mCurrentRect.translate((point - mOffsetPoint).toPoint());
        setOffsetPoint(point);
        break;
    case MoveMode::TOPLEFT:
        if (point.x() < mCurrentRect.right() && point.y() < mCurrentRect.bottom())
        {
            mFieldW = mCurrentRect.right() - static_cast<int>(point.x());
            mFieldH = static_cast<int>(mFieldW * mAspectRatio);
            mCurrentRect = QRect(QPoint(mCurrentRect.right() - mFieldW, mCurrentRect.bottom() - mFieldH) , mCurrentRect.bottomRight());
        }
        break;
    case MoveMode::TOPRIGHT:
        if (point.x() > mCurrentRect.left() && point.y() < mCurrentRect.bottom())
        {
            mFieldW = static_cast<int>(point.x()) - mCurrentRect.left();
            mFieldH = static_cast<int>(mFieldW * mAspectRatio);
            mCurrentRect = QRect(QPoint(mCurrentRect.left(), mCurrentRect.bottom() - mFieldH),
                                 QPoint(mCurrentRect.left() + mFieldW, mCurrentRect.bottom()));
        }
        break;
    case MoveMode::BOTTOMLEFT:
        if (point.x() < mCurrentRect.right() && point.y() > mCurrentRect.top())
        {
            mFieldW = mCurrentRect.right() - static_cast<int>(point.x());
            mFieldH = static_cast<int>(mFieldW * mAspectRatio);
            mCurrentRect = QRect(QPoint(point.toPoint().x(), mCurrentRect.top()), QPoint(mCurrentRect.right(), mCurrentRect.top() + mFieldH));
        }
        break;
    case MoveMode::BOTTOMRIGHT:
        if (point.x() > mCurrentRect.left() && point.y() > mCurrentRect.top())
        {
            mFieldW = static_cast<int>(point.x()) - mCurrentRect.left();
            mFieldH = static_cast<int>(mFieldW * mAspectRatio);
            mCurrentRect = QRect(mCurrentRect.topLeft(), QSize(mFieldW, mFieldH));
        }
        break;
    default:
        break;
    }

    Camera* c = getCameraAtFrame(frame);
    if (c == nullptr) { return; }

//    qDebug() << "før S T: " << c->scaling() << " " << mCurrentRect.center();
    /*
    c->translate(point - mOffsetPoint.toPoint());
    c->scale(static_cast<qreal>(mCurrentRect.width()) / static_cast<qreal>(viewRect.width()));
*/
//    qDebug() << "UPDATE0 frame: " << c->pos() << " point: " << c->translation() << " scale: " << c->scaling() << " rotate: " << c->rotation();
/*
    c->setTranslate(mCurrentRect.center());
    c->setScale(static_cast<qreal>(mCurrentRect.width()) / static_cast<qreal>(viewRect.width()));
*/

//    qDebug() << "UPDATE1 frame: " << c->pos() << " point: " << c->translation() << " scale: " << c->scaling() << " rotate: " << c->rotation();
//    qDebug() << "UPDATE2 frame: " << c->pos() << " point: " << mCurrentRect.center() << " scale: " << c->scaling() << " rotate: " << c->rotation();
    qreal scaling = static_cast<qreal>(mCurrentRect.width()) / static_cast<qreal>(viewRect.width());
    c->view.setMatrix(scaling,
                      0.,
                      0.,
                      0.,
                      scaling,
                      0.,
                      mCurrentRect.center().x(),
                      mCurrentRect.center().y(),
                      0.);

//    qDebug() << "eft S T: " << c->scaling() << " " << mCurrentRect.center();
    c->setCamRect(mCurrentRect);

    c->modification();

}

// is only called when &Editor::currentFrameChanged fires...
void LayerCamera::updateCamRects(int frame)
{
    Camera* c = getCameraAtFrame(frame);
    if (c == nullptr || !c->getCamRect().isValid()) { return; }

    mCurrentRect = c->getCamRect();
    qDebug() << "Ved scrub: " << c->pos() << " " << mCurrentRect << " " << c->view; // VIRKER!
}

void LayerCamera::linearInterpolateTransform(Camera* cam)
{
    if (keyFrameCount() == 0)
        return;

    int frameNumber = cam->pos();
    Camera* camera1 = static_cast<Camera*>(getLastKeyFrameAtPosition(frameNumber - 1));

    int nextFrame = getNextKeyFramePosition(frameNumber);
    Camera* camera2 = static_cast<Camera*>(getLastKeyFrameAtPosition(nextFrame));

    if (camera1 == nullptr && camera2 == nullptr)
    {
        return; // do nothing
    }
    else if (camera1 == nullptr && camera2 != nullptr)
    {
        return cam->assign(*camera2);
    }
    else if (camera2 == nullptr && camera1 != nullptr)
    {
        return cam->assign(*camera1);
    }

    if (camera1 == camera2)
    {
        return cam->assign(*camera1);
    }

    double frame1 = camera1->pos();
    double frame2 = camera2->pos();

    // linear interpolation
    double c2 = (frameNumber - frame1) / (frame2 - frame1);

    auto lerp = [](double f1, double f2, double ratio) -> double
    {
        return f1 * (1.0 - ratio) + f2 * ratio;
    };

    double dx = lerp(camera1->translation().x(), camera2->translation().x(), c2);
    double dy = lerp(camera1->translation().y(), camera2->translation().y(), c2);
    double r = lerp(camera1->rotation(), camera2->rotation(), c2);
    double s = lerp(camera1->scaling(), camera2->scaling(), c2);

    cam->translate(dx, dy);
    cam->rotate(r);
    cam->scale(s);
}

void LayerCamera::getCameraPathFrames(Camera* cam)
{

}

QRect LayerCamera::getViewRect()
{
    return viewRect;
}

QSize LayerCamera::getViewSize()
{
    return viewRect.size();
}

void LayerCamera::loadImageAtFrame(int frameNumber, qreal dx, qreal dy, qreal rotate, qreal scale)
{
    if (keyExists(frameNumber))
    {
        removeKeyFrame(frameNumber);
    }
    int w = viewRect.width() * scale;
    int h = viewRect.height() * scale;
    QPoint topLeft = QPoint(dx - w / 2, dy - h/2);
    mCurrentRect = QRect(topLeft, QSize(w, h));
    qDebug() << "after LOAD " << mCurrentRect;
    Camera* camera = new Camera(QPointF(dx, dy), rotate, scale);
    camera->setPos(frameNumber);

    camera->setCamRect(mCurrentRect);
    camera->setTranslate(QPointF(mCurrentRect.center()));
    camera->setScale(scale);
    loadKey(camera);
}

Status LayerCamera::saveKeyFrameFile(KeyFrame*, QString)
{
    return Status::OK;
}

KeyFrame* LayerCamera::createKeyFrame(int position, Object*)
{
//    if (!keyExists(position))
//    {
    Camera* old = static_cast<Camera*>(getKeyFrameAt(getPreviousKeyFramePosition(position - 1)));
    bool cont = old == nullptr ? false: true;
    Camera* c = new Camera;
    c->setPos(position);
    linearInterpolateTransform(c);
    if (cont && getPreviousKeyFramePosition(position) != position)
    {
        c->view.setMatrix(old->view.m11(),
                          old->view.m12(),
                          old->view.m13(),
                          old->view.m21(),
                          old->view.m22(),
                          old->view.m23(),
                          old->view.m31(),
                          old->view.m32(),
                          old->view.m33());
        /*
        c->setTranslate(old->getCamRect().center());
        c->setScale(old->scaling());
        c->setRotate(old->rotation());
        */
        qDebug() << "UPDATE old frame: " << old->view;
        qDebug() << "UPDATE  c  frame: " << c->view;
    }
//    qDebug() << "CREATE frame: " << c->pos() << " point: " << c->translation() << " scale: " << c->scaling() << " rotate: " << c->rotation();
    return  c;
    /*    }

    Camera* old = static_cast<Camera*>(getLastKeyFrameAtPosition(position - 1));
    if (old == nullptr) { return new Camera; }

    Camera* c = new Camera;
    c->setPos(position);
    c->translate(old->translation());
    c->scale(old->scaling());
    c->rotate(old->rotation());
    return  c; */
}

void LayerCamera::editProperties()
{
    if (dialog == nullptr)
    {
        dialog = new CameraPropertiesDialog(name(), viewRect.width(), viewRect.height());
    }
    dialog->setName(name());
    dialog->setWidth(viewRect.width());
    dialog->setHeight(viewRect.height());
    int result = dialog->exec();
    if (result == QDialog::Accepted)
    {
        setName(dialog->getName());
        QSettings settings(PENCIL2D, PENCIL2D);
        settings.setValue(SETTING_FIELD_W, dialog->getWidth());
        settings.setValue(SETTING_FIELD_H, dialog->getHeight());
        mFieldW = dialog->getWidth();
        mFieldH = dialog->getHeight();
        mAspectRatio = static_cast<qreal>(mFieldH) / static_cast<qreal>(mFieldW);
        mCurrentRect = viewRect = QRect(-dialog->getWidth() / 2, -dialog->getHeight() / 2, dialog->getWidth(), dialog->getHeight());

        emit resolutionChanged();
    }
}

QDomElement LayerCamera::createDomElement(QDomDocument& doc)
{
    QDomElement layerElem = this->createBaseDomElement(doc);

    layerElem.setAttribute("width", viewRect.width()); // TODO should this be viewRect or mCurrentRect?
    layerElem.setAttribute("height", viewRect.height());

    /*
    layerElem.setAttribute("width", mCurrentRect.width()); // TODO should this be viewRect or mCurrentRect?
    layerElem.setAttribute("height", mCurrentRect.height());
  */
    foreachKeyFrame([&](KeyFrame* pKeyFrame)
                    {
                        Camera* camera = static_cast<Camera*>(pKeyFrame);
                        QDomElement keyTag = doc.createElement("camera");
                        keyTag.setAttribute("frame", camera->pos());

                        keyTag.setAttribute("r", camera->rotation());
                        keyTag.setAttribute("s", camera->scaling());
                        keyTag.setAttribute("dx", camera->translation().x());
                        keyTag.setAttribute("dy", camera->translation().y());
                        qDebug() << "SAVE T S: " << camera->pos() << " " << camera->translation() << " " << camera->scaling();
                        layerElem.appendChild(keyTag);
                    });

    return layerElem;
}

void LayerCamera::loadDomElement(const QDomElement& element, QString dataDirPath, ProgressCallback progressStep)
{
    Q_UNUSED(dataDirPath)
    Q_UNUSED(progressStep)

    this->loadBaseDomElement(element);

    int width = element.attribute("width").toInt();
    int height = element.attribute("height").toInt();
    viewRect = QRect(-width / 2, -height / 2, width, height);

    QDomNode imageTag = element.firstChild();
    while (!imageTag.isNull())
    {
        QDomElement imageElement = imageTag.toElement();
        if (!imageElement.isNull())
        {
            if (imageElement.tagName() == "camera")
            {
                int frame = imageElement.attribute("frame").toInt();

                qreal rotate = imageElement.attribute("r", "0").toDouble();
                qreal scale = imageElement.attribute("s", "1").toDouble();
                qreal dx = imageElement.attribute("dx", "0").toDouble();
                qreal dy = imageElement.attribute("dy", "0").toDouble();
                qDebug() << "LOAD T S: " << frame << " " << QPointF(dx, dy) << " " << scale;

                loadImageAtFrame(frame, dx, dy, rotate, scale);
            }
        }
        imageTag = imageTag.nextSibling();
    }
}
