/*

Pencil - Traditional Animation Software
Copyright (C) 2012-2018 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include "actioncommands.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QFileDialog>

#include "pencildef.h"
#include "editor.h"
#include "object.h"
#include "viewmanager.h"
#include "layermanager.h"
#include "soundmanager.h"
#include "playbackmanager.h"
#include "colormanager.h"
#include "preferencemanager.h"
#include "selectionmanager.h"
#include "util.h"
#include "app_util.h"

#include "layercamera.h"
#include "layersound.h"
#include "layerbitmap.h"
#include "layervector.h"
#include "bitmapimage.h"
#include "vectorimage.h"
#include "soundclip.h"
#include "camera.h"

#include "movieexporter.h"
#include "filedialogex.h"
#include "exportmoviedialog.h"
#include "exportimagedialog.h"
#include "aboutdialog.h"
#include "doubleprogressdialog.h"
#include "copymultiplekeyframesdialog.h"
#include "checkupdatesdialog.h"

ActionCommands::ActionCommands(QWidget* parent) : QObject(parent)
{
    mParent = parent;
}

ActionCommands::~ActionCommands() {}

Status ActionCommands::importSound()
{
    Layer* layer = mEditor->layers()->currentLayer();
    Q_ASSERT(layer);
    NULLReturn(layer, Status::FAIL);

    if (layer->type() != Layer::SOUND)
    {
        QMessageBox msg;
        msg.setText(tr("No sound layer exists as a destination for your import. Create a new sound layer?"));
        msg.addButton(tr("Create sound layer"), QMessageBox::AcceptRole);
        msg.addButton(tr("Don't create layer"), QMessageBox::RejectRole);

        int buttonClicked = msg.exec();
        if (buttonClicked != QMessageBox::AcceptRole)
        {
            return Status::SAFE;
        }

        // Create new sound layer.
        bool ok = false;
        QString strLayerName = QInputDialog::getText(mParent, tr("Layer Properties", "Dialog title on creating a sound layer"),
                                                     tr("Layer name:"), QLineEdit::Normal,
                                                     tr("Sound Layer", "Default name on creating a sound layer"), &ok);
        if (ok && !strLayerName.isEmpty())
        {
            Layer* newLayer = mEditor->layers()->createSoundLayer(strLayerName);
            mEditor->layers()->setCurrentLayer(newLayer);
        }
        else
        {
            Q_ASSERT(false);
            return Status::FAIL;
        }
    }

    layer = mEditor->layers()->currentLayer();
    Q_ASSERT(layer->type() == Layer::SOUND);


    int currentFrame = mEditor->currentFrame();

    SoundClip* key = static_cast<SoundClip*>(mEditor->addNewKey());

    if (key == nullptr)
    {
        // Probably tried to modify a hidden layer or something like that
        // Let Editor handle the warnings
        return Status::SAFE;
    }

    FileDialog fileDialog(mParent);
    QString strSoundFile = fileDialog.openFile(FileType::SOUND);

    if (strSoundFile.isEmpty())
    {
        return Status::SAFE;
    }

    Status st = mEditor->sound()->loadSound(key, strSoundFile);

    if (!st.ok())
    {
        layer->removeKeyFrame(currentFrame);
    }

    return st;
}

Status ActionCommands::exportGif()
{
    // exporting gif
    return exportMovie(true);
}

Status ActionCommands::exportMovie(bool isGif)
{
    ExportMovieDialog* dialog = nullptr;
    if (isGif) {
        dialog = new ExportMovieDialog(mParent, ImportExportDialog::Export, FileType::GIF);
    } else {
        dialog = new ExportMovieDialog(mParent);
    }
    OnScopeExit(dialog->deleteLater());

    dialog->init();
    
    std::vector< std::pair<QString, QSize> > camerasInfo;
    auto cameraLayers = mEditor->object()->getLayersByType< LayerCamera >();
    for (LayerCamera* i : cameraLayers)
    {
        camerasInfo.push_back(std::make_pair(i->name(), i->getViewSize()));
    }

    auto currLayer = mEditor->layers()->currentLayer();
    if (currLayer->type() == Layer::CAMERA)
    {
        QString strName = currLayer->name();
        auto it = std::find_if(camerasInfo.begin(), camerasInfo.end(),
            [strName](std::pair<QString, QSize> p)
        {
            return p.first == strName;
        });

        Q_ASSERT(it != camerasInfo.end());

        std::swap(camerasInfo[0], *it);
    }

    dialog->setCamerasInfo(camerasInfo);

    int lengthWithSounds = mEditor->layers()->animationLength(true);
    int length = mEditor->layers()->animationLength(false);

    dialog->setDefaultRange(1, length, lengthWithSounds);
    dialog->exec();
    
    if (dialog->result() == QDialog::Rejected)
    {
        return Status::SAFE;
    }
    QString strMoviePath = dialog->getFilePath();

    ExportMovieDesc desc;
    desc.strFileName = strMoviePath;
    desc.startFrame = dialog->getStartFrame();
    desc.endFrame = dialog->getEndFrame();
    desc.fps = mEditor->playback()->fps();
    desc.exportSize = dialog->getExportSize();
    desc.strCameraName = dialog->getSelectedCameraName();
    desc.loop = dialog->getLoop();
    desc.alpha = dialog->getTransparency();

    DoubleProgressDialog progressDlg;
    progressDlg.setWindowModality(Qt::WindowModal);
    progressDlg.setWindowTitle(tr("Exporting movie"));
    Qt::WindowFlags eFlags = Qt::Dialog | Qt::WindowTitleHint;
    progressDlg.setWindowFlags(eFlags);
    progressDlg.show();

    MovieExporter ex;

    connect(&progressDlg, &DoubleProgressDialog::canceled, [&ex]
    {
        ex.cancel();
    });

    // The start points and length for the current minor operation segment on the major progress bar
    float minorStart, minorLength;

    Status st = ex.run(mEditor->object(), desc,
        [&progressDlg, &minorStart, &minorLength](float f, float final)
        {
            progressDlg.major->setValue(f);

            minorStart = f;
            minorLength = qMax(0.f, final - minorStart);

            QApplication::processEvents();
        },
        [&progressDlg, &minorStart, &minorLength](float f) {
            progressDlg.minor->setValue(f);

            progressDlg.major->setValue(minorStart + f * minorLength);

            QApplication::processEvents();
        },
        [&progressDlg](QString s) {
            progressDlg.setStatus(s);
            QApplication::processEvents();
        }
    );

    if (st.ok() && QFile::exists(strMoviePath))
    {
        if (isGif) {
            auto btn = QMessageBox::question(mParent, "Pencil2D",
                                             tr("Finished. Open file location?"));

            if (btn == QMessageBox::Yes)
            {
                QString path = dialog->getAbsolutePath();
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            }
            return Status::OK;
        }
        auto btn = QMessageBox::question(mParent, "Pencil2D",
                                         tr("Finished. Open movie now?", "When movie export done."));
        if (btn == QMessageBox::Yes)
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(strMoviePath));
        }
    }
    return Status::OK;
}

Status ActionCommands::exportImageSequence()
{
    auto dialog = new ExportImageDialog(mParent, FileType::IMAGE_SEQUENCE);
    OnScopeExit(dialog->deleteLater());
    
    dialog->init();

    std::vector< std::pair<QString, QSize> > camerasInfo;
    auto cameraLayers = mEditor->object()->getLayersByType< LayerCamera >();
    for (LayerCamera* i : cameraLayers)
    {
        camerasInfo.push_back(std::make_pair(i->name(), i->getViewSize()));
    }

    auto currLayer = mEditor->layers()->currentLayer();
    if (currLayer->type() == Layer::CAMERA)
    {
        QString strName = currLayer->name();
        auto it = std::find_if(camerasInfo.begin(), camerasInfo.end(),
            [strName](std::pair<QString, QSize> p)
        {
            return p.first == strName;
        });

        Q_ASSERT(it != camerasInfo.end());
        std::swap(camerasInfo[0], *it);
    }
    dialog->setCamerasInfo(camerasInfo);

    int lengthWithSounds = mEditor->layers()->animationLength(true);
    int length = mEditor->layers()->animationLength(false);

    dialog->setDefaultRange(1, length, lengthWithSounds);

    dialog->exec();

    if (dialog->result() == QDialog::Rejected)
    {
        return Status::SAFE;
    }

    QString strFilePath = dialog->getFilePath();
    QSize exportSize = dialog->getExportSize();
    QString exportFormat = dialog->getExportFormat();
    bool exportKeyframesOnly = dialog->getExportKeyframesOnly();
    bool useTranparency = dialog->getTransparency();
    int startFrame = dialog->getStartFrame();
    int endFrame  = dialog->getEndFrame();

    QString sCameraLayerName = dialog->getCameraLayerName();
    LayerCamera* cameraLayer = static_cast<LayerCamera*>(mEditor->layers()->findLayerByName(sCameraLayerName, Layer::CAMERA));

    // Show a progress dialog, as this can take a while if you have lots of frames.
    QProgressDialog progress(tr("Exporting image sequence..."), tr("Abort"), 0, 100, mParent);
    hideQuestionMark(progress);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    mEditor->object()->exportFrames(startFrame, endFrame,
                                    cameraLayer,
                                    exportSize,
                                    strFilePath,
                                    exportFormat,
                                    useTranparency,
                                    exportKeyframesOnly,
                                    mEditor->layers()->currentLayer()->name(),
                                    true,
                                    &progress,
                                    100);

    progress.close();

    return Status::OK;
}

Status ActionCommands::exportImage()
{
    // Options
    auto dialog = new ExportImageDialog(mParent, FileType::IMAGE);
    OnScopeExit(dialog->deleteLater());

    dialog->init();

    std::vector< std::pair<QString, QSize> > camerasInfo;
    auto cameraLayers = mEditor->object()->getLayersByType< LayerCamera >();
    for (LayerCamera* i : cameraLayers)
    {
        camerasInfo.push_back(std::make_pair(i->name(), i->getViewSize()));
    }

    auto currLayer = mEditor->layers()->currentLayer();
    if (currLayer->type() == Layer::CAMERA)
    {
        QString strName = currLayer->name();
        auto it = std::find_if(camerasInfo.begin(), camerasInfo.end(),
            [strName](std::pair<QString, QSize> p)
        {
            return p.first == strName;
        });

        Q_ASSERT(it != camerasInfo.end());
        std::swap(camerasInfo[0], *it);
    }
    dialog->setCamerasInfo(camerasInfo);

    dialog->exec();

    if (dialog->result() == QDialog::Rejected)
    {
        return Status::SAFE;
    }

    QString filePath = dialog->getFilePath();
    QSize exportSize = dialog->getExportSize();
    QString exportFormat = dialog->getExportFormat();
    bool useTranparency = dialog->getTransparency();

    // Export
    QString sCameraLayerName = dialog->getCameraLayerName();
    LayerCamera* cameraLayer = static_cast<LayerCamera*>(mEditor->layers()->findLayerByName(sCameraLayerName, Layer::CAMERA));

    QTransform view = cameraLayer->getViewAtFrame(mEditor->currentFrame());

    bool bOK = mEditor->object()->exportIm(mEditor->currentFrame(),
                                           view,
                                           cameraLayer->getViewSize(),
                                           exportSize,
                                           filePath,
                                           exportFormat,
                                           true,
                                           useTranparency);

    if (!bOK)
    {
        QMessageBox::warning(mParent,
                             tr("Warning"),
                             tr("Unable to export image."),
                             QMessageBox::Ok);
        return Status::FAIL;
    }
    return Status::OK;
}

void ActionCommands::flipSelectionX()
{
    bool flipVertical = false;
    mEditor->flipSelection(flipVertical);
}

void ActionCommands::flipSelectionY()
{
    bool flipVertical = true;
    mEditor->flipSelection(flipVertical);
}

void ActionCommands::selectAll()
{
    mEditor->selectAll();
}

void ActionCommands::deselectAll()
{
    mEditor->deselectAll();
}

void ActionCommands::ZoomIn()
{
    mEditor->view()->scaleUp();
}

void ActionCommands::ZoomOut()
{
    mEditor->view()->scaleDown();
}

void ActionCommands::rotateClockwise()
{
    float currentRotation = mEditor->view()->rotation();
    mEditor->view()->rotate(currentRotation + 15.f);
}

void ActionCommands::rotateCounterClockwise()
{
    float currentRotation = mEditor->view()->rotation();
    mEditor->view()->rotate(currentRotation - 15.f);
}

void ActionCommands::toggleMirror()
{
    bool flipX = mEditor->view()->isFlipHorizontal();
    mEditor->view()->flipHorizontal(!flipX);
}

void ActionCommands::toggleMirrorV()
{
    bool flipY = mEditor->view()->isFlipVertical();
    mEditor->view()->flipVertical(!flipY);
}

void ActionCommands::showGrid(bool bShow)
{
    auto prefs = mEditor->preference();
    if (bShow)
        prefs->turnOn(SETTING::GRID);
    else
        prefs->turnOff(SETTING::GRID);
}

void ActionCommands::PlayStop()
{
    PlaybackManager* playback = mEditor->playback();
    if (playback->isPlaying())
    {
        playback->stop();
    }
    else
    {
        playback->play();
    }
}

void ActionCommands::GotoNextFrame()
{
    mEditor->scrubForward();
}

void ActionCommands::GotoPrevFrame()
{
    mEditor->scrubBackward();
}

void ActionCommands::GotoNextKeyFrame()
{
    mEditor->scrubNextKeyFrame();
}

void ActionCommands::GotoPrevKeyFrame()
{
    mEditor->scrubPreviousKeyFrame();
}

Status ActionCommands::addNewKey()
{
    KeyFrame* key = mEditor->addNewKey();

    SoundClip* clip = dynamic_cast<SoundClip*>(key);
    if (clip)
    {
        FileDialog fileDialog(mParent);
        QString strSoundFile = fileDialog.openFile(FileType::SOUND);

        if (strSoundFile.isEmpty())
        {
            mEditor->layers()->currentLayer()->removeKeyFrame(clip->pos());
            return Status::SAFE;
        }
        Status st = mEditor->sound()->loadSound(clip, strSoundFile);
        if (!st.ok())
        {
            mEditor->layers()->currentLayer()->removeKeyFrame(clip->pos());
            return Status::ERROR_LOAD_SOUND_FILE;
        }
    }

    Camera* cam = dynamic_cast<Camera*>(key);
    if (cam)
    {
        mEditor->view()->updateViewTransforms();
    }

    return Status::OK;
}

void ActionCommands::removeKey()
{
    mEditor->removeKey();

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer->keyFrameCount() == 0)
    {
        layer->addNewKeyFrameAt(1);
    }
}

void ActionCommands::duplicateKey()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) return;
    if (!layer->visible())
    {
        mEditor->showLayerNotVisibleWarning();
        return;
    }

    KeyFrame* key = layer->getKeyFrameAt(mEditor->currentFrame());
    if (key == nullptr) return;

    KeyFrame* dupKey = key->clone();

    int nextEmptyFrame = mEditor->currentFrame() + 1;
    while (layer->keyExistsWhichCovers(nextEmptyFrame))
    {
        nextEmptyFrame += 1;
    }

    layer->addKeyFrame(nextEmptyFrame, dupKey);
    mEditor->scrubTo(nextEmptyFrame);

    if (layer->type() == Layer::SOUND)
    {
        mEditor->sound()->processSound(dynamic_cast<SoundClip*>(dupKey));
    }
    else
    {
        dupKey->setFileName(""); // don't share filename
        dupKey->modification();
    }

    mEditor->layers()->notifyAnimationLengthChanged();
}

void ActionCommands::manipulateFrames()
{
    int startLoop = 1, stopLoop = 2;          // default values
    if (mEditor->playback()->isRangedPlaybackOn())
    {                                   // defined range values if available
        startLoop = mEditor->playback()->getRangedStartFrame();
        stopLoop = mEditor->playback()->getRangedEndFrame();
    }
    LayerManager* layerMgr = mEditor->layers();
    Layer::LAYER_TYPE layerType = layerMgr->currentLayer()->type();
    if (layerType != Layer::BITMAP && layerType != Layer::VECTOR)
    {
        Q_UNUSED(QMessageBox::information(nullptr, tr("Action not available"),
                                          tr("Can only be used on Bitmap and Vector layers."),
                                          QMessageBox::Ok));
        return;
    }
    CopyMultiplekeyframesDialog* cd = new CopyMultiplekeyframesDialog(layerMgr, startLoop, stopLoop, nullptr);
    cd->exec();

    if (cd->result() != QDialog::Accepted) { return; }

    if (!cd->getValidity())
    {
        Q_UNUSED(QMessageBox::critical(nullptr, tr("Action not valid!"),
                                       tr("Failed validation check:\n- Exceeds 9999 Frames OR\n- Range not valid"),
                                       QMessageBox::Ok));
        return;
    }

    // If Validation is OK
    startLoop = cd->getFirstFrame();
    stopLoop = cd->getLastFrame();
    Layer *fromLayer = layerMgr->findLayerByName(cd->getFromLayer());
    Q_ASSERT(fromLayer != nullptr);
    Layer *toLayer = nullptr;
    int scrubOrg = mEditor->currentFrame();
    int numLoops, startFrame;
    switch (cd->getCurrentTab()) {
    case 0: // Copy
        startFrame = cd->getCopyStartFrame();
        toLayer = layerMgr->findLayerByName(cd->getCopyToLayer());
        Q_ASSERT(toLayer != nullptr);
        numLoops = cd->getNumLoops();
        fromLayer->copyFrames(startLoop, stopLoop, numLoops, startFrame, fromLayer, toLayer);
        break;
    case 1: // Move
        startFrame = cd->getMoveStartFrame();
        toLayer = layerMgr->findLayerByName(cd->getMoveToLayer());
        Q_ASSERT(toLayer != nullptr);
        fromLayer->moveFrames(startLoop, stopLoop,startFrame, fromLayer, toLayer);
        break;
    case 2: // Reverse
        startFrame = cd->getReverseStartFrame();
        fromLayer->reverseFrames(startLoop, stopLoop, startFrame, fromLayer);
        break;
    case 3: // Delete
        fromLayer->deleteFrames(startLoop, stopLoop, fromLayer);
        break;
    default:
        Q_ASSERT(false);
        break;
    }
    mEditor->scrubTo(scrubOrg);
    mEditor->layers()->notifyLayerChanged(toLayer);
    mEditor->layers()->notifyAnimationLengthChanged();
}

void ActionCommands::moveFrameForward()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer)
    {
        if (layer->moveKeyFrameForward(mEditor->currentFrame()))
        {
            mEditor->scrubForward();
        }
    }

    mEditor->layers()->notifyAnimationLengthChanged();
}

void ActionCommands::moveFrameBackward()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer)
    {
        if (layer->moveKeyFrameBackward(mEditor->currentFrame()))
        {
            mEditor->scrubBackward();
        }
    }
}

Status ActionCommands::addNewBitmapLayer()
{
    bool ok;
    QString text = QInputDialog::getText(nullptr, tr("Layer Properties"),
                                         tr("Layer name:"), QLineEdit::Normal,
                                         mEditor->layers()->nameSuggestLayer(tr("Bitmap Layer")), &ok);
    if (ok && !text.isEmpty())
    {
        mEditor->layers()->createBitmapLayer(text);
    }
    return Status::OK;
}

Status ActionCommands::addNewVectorLayer()
{
    bool ok;
    QString text = QInputDialog::getText(nullptr, tr("Layer Properties"),
                                         tr("Layer name:"), QLineEdit::Normal,
                                         mEditor->layers()->nameSuggestLayer(tr("Vector Layer")), &ok);
    if (ok && !text.isEmpty())
    {
        mEditor->layers()->createVectorLayer(text);
    }
    return Status::OK;
}

Status ActionCommands::addNewCameraLayer()
{
    bool ok;
    QString text = QInputDialog::getText(nullptr, tr("Layer Properties"),
                                         tr("Layer name:"), QLineEdit::Normal,
                                         mEditor->layers()->nameSuggestLayer(tr("Camera Layer")), &ok);
    if (ok && !text.isEmpty())
    {
        mEditor->layers()->createCameraLayer(text);
    }
    return Status::OK;
}

Status ActionCommands::addNewSoundLayer()
{
    bool ok = false;
    QString strLayerName = QInputDialog::getText(nullptr, tr("Layer Properties"),
                                                 tr("Layer name:"), QLineEdit::Normal,
                                                 mEditor->layers()->nameSuggestLayer(tr("Sound Layer")), &ok);
    if (ok && !strLayerName.isEmpty())
    {
        Layer* layer = mEditor->layers()->createSoundLayer(strLayerName);
        mEditor->layers()->setCurrentLayer(layer);
   }
    return Status::OK;
}

Status ActionCommands::deleteCurrentLayer()
{
    LayerManager* layerMgr = mEditor->layers();
    QString strLayerName = layerMgr->currentLayer()->name();

    int ret = QMessageBox::warning(mParent,
                                   tr("Delete Layer", "Windows title of Delete current layer pop-up."),
                                   tr("Are you sure you want to delete layer: ") + strLayerName + " ?",
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);
    if (ret == QMessageBox::Ok)
    {
        Status st = layerMgr->deleteLayer(mEditor->currentLayerIndex());
        if (st == Status::ERROR_NEED_AT_LEAST_ONE_CAMERA_LAYER)
        {
            QMessageBox::information(mParent, "",
                                     tr("Please keep at least one camera layer in project", "text when failed to delete camera layer"));
        }
    }
    return Status::OK;
}

void ActionCommands::changeKeyframeLineColor()
{
    if (mEditor->layers()->currentLayer()->type() == Layer::BITMAP &&
            mEditor->layers()->currentLayer()->keyExists(mEditor->currentFrame()))
    {
        QRgb color = mEditor->color()->frontColor().rgb();
        LayerBitmap* layer = static_cast<LayerBitmap*>(mEditor->layers()->currentLayer());
        layer->getBitmapImageAtFrame(mEditor->currentFrame())->fillNonAlphaPixels(color);
        mEditor->updateFrame(mEditor->currentFrame());
    }
}

void ActionCommands::changeallKeyframeLineColor()
{
    if (mEditor->layers()->currentLayer()->type() == Layer::BITMAP)
    {
        QRgb color = mEditor->color()->frontColor().rgb();
        LayerBitmap* layer = static_cast<LayerBitmap*>(mEditor->layers()->currentLayer());
        for (int i = layer->firstKeyFramePosition(); i <= layer->getMaxKeyFramePosition(); i++)
        {
            if (layer->keyExists(i))
                layer->getBitmapImageAtFrame(i)->fillNonAlphaPixels(color);
        }
        mEditor->updateFrame(mEditor->currentFrame());
    }
}

void ActionCommands::help()
{
    QString url = "http://www.pencil2d.org/doc/";
    QDesktopServices::openUrl(QUrl(url));
}

void ActionCommands::quickGuide()
{
    QString sDocPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString sCopyDest = QDir(sDocPath).filePath("pencil2d_quick_guide.pdf");

    QFile quickGuideFile(":/app/pencil2d_quick_guide.pdf");
    quickGuideFile.copy(sCopyDest);

    QDesktopServices::openUrl(QUrl::fromLocalFile(sCopyDest));
}

void ActionCommands::website()
{
    QString url = "https://www.pencil2d.org/";
    QDesktopServices::openUrl(QUrl(url));
}

void ActionCommands::forum()
{
    QString url = "https://discuss.pencil2d.org/";
    QDesktopServices::openUrl(QUrl(url));
}

void ActionCommands::discord()
{
    QString url = "https://discord.gg/8FxdV2g";
    QDesktopServices::openUrl(QUrl(url));
}

void ActionCommands::reportbug()
{
    QString url = "https://github.com/pencil2d/pencil/issues";
    QDesktopServices::openUrl(QUrl(url));
}

void ActionCommands::checkForUpdates()
{
    CheckUpdatesDialog dialog;
    dialog.startChecking();
    dialog.exec();
}

void ActionCommands::about()
{
    AboutDialog* aboutBox = new AboutDialog(mParent);
    aboutBox->setAttribute(Qt::WA_DeleteOnClose);
    aboutBox->init();
    aboutBox->exec();
}
