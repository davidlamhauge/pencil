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

    qDebug() << c->pos() << " CurrRect " << mCurrentRect << " VIEW " << c->view;

    c->translate(mCurrentRect.center());
    c->scale(static_cast<qreal>(viewRect.width()) / static_cast<qreal>(mCurrentRect.width()) );

    qDebug() << c->pos() << " CurrRect " << mCurrentRect << " VIEW " << c->view << "\n";

    c->setCamRect(mCurrentRect);

    c->modification();

}

// is only called when &Editor::currentFrameChanged fires...
void LayerCamera::updateCamRects(int frame)
{
    Camera* c = getCameraAtFrame(frame);
    if (c == nullptr || !c->getCamRect().isValid()) { return; }

    mCurrentRect = c->getCamRect();
//    qDebug() << "UPDATE " << c->pos() << " " << c->getCamRect() << " "  << c->view;
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

QRect LayerCamera::getViewRect()
{
    return viewRect;
}

QSize LayerCamera::getViewSize()
{
    return viewRect.size();
}

void LayerCamera::ifObjectLoaded(int currentFrame)
{
    for (int i = firstKeyFramePosition(); i <= getMaxKeyFramePosition(); i++)
    {
        if (keyExists(i))
        {
            Camera* tmpCam = getCameraAtFrame(i);
            int w = static_cast<int>(viewRect.width() * tmpCam->view.m11());
            int h = static_cast<int>(viewRect.height() * tmpCam->view.m11());
            QPoint topLeft = QPoint(static_cast<int>(tmpCam->view.dx() - w / 2), static_cast<int>(tmpCam->view.dy() - h/2));
            QRect rect = QRect(topLeft, QSize(w, h));
            tmpCam->setCamRect(rect);
//            qDebug() << "I FOR-LOOP" << tmpCam->pos() << " rect: " << rect << " VIEW: " << tmpCam->view;
        }
    }

    // set mCurrentRect
    Camera* c = getCameraAtFrame(currentFrame);
    if (c == nullptr) { return; }

    mCurrentRect = QRect(QPoint(c->view.dx(), c->view.dy()), QSize(viewRect.width() * c->view.m11(), viewRect.height() * c->view.m11()));
//    qDebug() << " FORBI FOR-LOOP" << c->pos() << " rect: " << mCurrentRect << " VIEW: " << c->view;
    c->modification();
}

// used only when loading scene from xml file
void LayerCamera::loadImageAtFrame(int frameNumber, qreal dx, qreal dy, qreal rotate, qreal scale)
{
    if (keyExists(frameNumber))
    {
        removeKeyFrame(frameNumber);
    }

    int w = static_cast<int>(viewRect.width() / scale);
    int h = static_cast<int>(viewRect.height() / scale);
    QPoint topLeft = QPoint(static_cast<int>(dx - w / 2), static_cast<int>(dy - h / 2));
    mCurrentRect = QRect(topLeft, QSize(w, h));

    Camera* camera = new Camera(QPointF(dx, dy), rotate, scale);
    camera->setPos(frameNumber);
//    qDebug() << "mcurrentRect after LOAD " << mCurrentRect;

    camera->setCamRect(mCurrentRect);
    camera->translate(QPointF(mCurrentRect.center()));
    camera->scale(scale);
    loadKey(camera);
}

Status LayerCamera::saveKeyFrameFile(KeyFrame*, QString)
{
    return Status::OK;
}

KeyFrame* LayerCamera::createKeyFrame(int position, Object*)
{
    Camera* c = new Camera;
    c->setPos(position);
    linearInterpolateTransform(c);

    int w = static_cast<int>(viewRect.width() / c->scaling());
    int h = static_cast<int>(viewRect.height() / c->scaling());
    QPoint topLeft = QPoint(static_cast<int>(c->view.dx() - w / 2), static_cast<int>(c->view.dy() - h / 2));
    mCurrentRect = QRect(topLeft, QSize(w, h));
    c->setCamRect(mCurrentRect);

//    qDebug() << "CREATE " << c->pos() << " " << c->getCamRect() << " "  << c->view;
    return  c;
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

    layerElem.setAttribute("width", viewRect.width());
    layerElem.setAttribute("height", viewRect.height());

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

                loadImageAtFrame(frame, dx, dy, rotate, scale);
            }
        }
        imageTag = imageTag.nextSibling();
    }}
