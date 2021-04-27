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

#include "editor.h"

#include <QApplication>
#include <QClipboard>
#include <QTimer>
#include <QImageReader>
#include <QDropEvent>
#include <QMimeData>
#include <QTemporaryDir>

#include "object.h"
#include "vectorimage.h"
#include "bitmapimage.h"
#include "soundclip.h"
#include "layerbitmap.h"
#include "layervector.h"
#include "layercamera.h"
#include "backupelement.h"

#include "colormanager.h"
#include "filemanager.h"
#include "toolmanager.h"
#include "layermanager.h"
#include "playbackmanager.h"
#include "viewmanager.h"
#include "preferencemanager.h"
#include "soundmanager.h"
#include "selectionmanager.h"

#include "scribblearea.h"
#include "timeline.h"
#include "util.h"


static BitmapImage g_clipboardBitmapImage;
static VectorImage g_clipboardVectorImage;


Editor::Editor(QObject* parent) : QObject(parent)
{
    mBackupIndex = -1;
    clipboardBitmapOk = false;
    clipboardVectorOk = false;
}

Editor::~Editor()
{
    // a lot more probably needs to be cleaned here...
    clearUndoStack();
    clearTemporary();
}

bool Editor::init()
{
    // Initialize managers
    mColorManager = new ColorManager(this);
    mLayerManager = new LayerManager(this);
    mToolManager = new ToolManager(this);
    mPlaybackManager = new PlaybackManager(this);
    mViewManager = new ViewManager(this);
    mPreferenceManager = new PreferenceManager(this);
    mSoundManager = new SoundManager(this);
    mSelectionManager = new SelectionManager(this);

    mAllManagers =
    {
        mColorManager,
        mToolManager,
        mLayerManager,
        mPlaybackManager,
        mViewManager,
        mPreferenceManager,
        mSoundManager,
        mSelectionManager
    };

    for (BaseManager* pManager : mAllManagers)
    {
        pManager->init();
    }

    makeConnections();

    mIsAutosave = mPreferenceManager->isOn(SETTING::AUTO_SAVE);
    mAutosaveNumber = mPreferenceManager->getInt(SETTING::AUTO_SAVE_NUMBER);

    return true;
}

int Editor::currentFrame() const
{
    return mFrame;
}

int Editor::fps()
{
    return mPlaybackManager->fps();
}

void Editor::setFps(int fps)
{
    mPreferenceManager->set(SETTING::FPS, fps);
    emit fpsChanged(fps);
}

void Editor::makeConnections()
{
    connect(mPreferenceManager, &PreferenceManager::optionChanged, this, &Editor::settingUpdated);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &Editor::clipboardChanged);
    // XXX: This is a hack to prevent crashes until #864 is done (see #1412)
    connect(mLayerManager, &LayerManager::layerDeleted, this, &Editor::sanitizeBackupElementsAfterLayerDeletion);
}

void Editor::settingUpdated(SETTING setting)
{
    switch (setting)
    {
    case SETTING::AUTO_SAVE:
        mIsAutosave = mPreferenceManager->isOn(SETTING::AUTO_SAVE);
        break;
    case SETTING::AUTO_SAVE_NUMBER:
        mAutosaveNumber = mPreferenceManager->getInt(SETTING::AUTO_SAVE_NUMBER);
        break;
    case SETTING::ONION_TYPE:
        mScribbleArea->onOnionSkinTypeChanged();
        emit updateTimeLine();
        break;
    case SETTING::FRAME_POOL_SIZE:
        mObject->setActiveFramePoolSize(mPreferenceManager->getInt(SETTING::FRAME_POOL_SIZE));
        break;
    case SETTING::LAYER_VISIBILITY:
        mScribbleArea->setLayerVisibility(static_cast<LayerVisibility>(mPreferenceManager->getInt(SETTING::LAYER_VISIBILITY)));
        emit updateTimeLine();
        break;
    default:
        break;
    }
}

BackupElement* Editor::currentBackup()
{
    if (mBackupIndex >= 0)
    {
        return mBackupList[mBackupIndex];
    }
    return nullptr;
}

void Editor::backup(const QString& undoText)
{
    KeyFrame* frame = nullptr;
    if (mLastModifiedLayer > -1 && mLastModifiedFrame > 0)
    {
        if (layers()->currentLayer()->type() == Layer::SOUND)
        {
            frame = layers()->currentLayer()->getKeyFrameWhichCovers(mLastModifiedFrame);
            if (frame != nullptr)
            {
                backup(mLastModifiedLayer, frame->pos(), undoText);
            }
        }
        else
        {
            backup(mLastModifiedLayer, mLastModifiedFrame, undoText);
        }
    }
    if (mLastModifiedLayer != layers()->currentLayerIndex() || mLastModifiedFrame != currentFrame())
    {
        if (layers()->currentLayer()->type() == Layer::SOUND)
        {
            frame = layers()->currentLayer()->getKeyFrameWhichCovers(currentFrame());

            if (frame != nullptr)
            {
                backup(layers()->currentLayerIndex(), frame->pos(), undoText);
            }
        }
        else
        {
            backup(layers()->currentLayerIndex(), currentFrame(), undoText);
        }
    }
}

void Editor::backup(int backupLayer, int backupFrame, const QString& undoText)
{
    while (mBackupList.size() - 1 > mBackupIndex && !mBackupList.empty())
    {
        delete mBackupList.takeLast();
    }
    while (mBackupList.size() > 19)   // we authorize only 20 levels of cancellation
    {
        delete mBackupList.takeFirst();
        mBackupIndex--;
    }

    Layer* layer = mObject->getLayer(backupLayer);
    if (layer != nullptr)
    {
        if (layer->type() == Layer::BITMAP)
        {
            BitmapImage* bitmapImage = static_cast<BitmapImage*>(layer->getLastKeyFrameAtPosition(backupFrame));
            if (currentFrame() == 1)
            {
                int previous = layer->getPreviousKeyFramePosition(backupFrame);
                bitmapImage = static_cast<BitmapImage*>(layer->getKeyFrameAt(previous));
            }
            if (bitmapImage != nullptr)
            {
                BackupBitmapElement* element = new BackupBitmapElement(bitmapImage);
                element->layer = backupLayer;
                element->frame = bitmapImage->pos();
                element->undoText = undoText;
                element->somethingSelected = select()->somethingSelected();
                element->mySelection = select()->mySelectionRect();
                element->myTransformedSelection = select()->myTransformedSelectionRect();
                element->myTempTransformedSelection = select()->myTempTransformedSelectionRect();
                element->rotationAngle = select()->myRotation();
                mBackupList.append(element);
                mBackupIndex++;
            }
        }
        else if (layer->type() == Layer::VECTOR)
        {
            VectorImage* vectorImage = static_cast<VectorImage*>(layer->getLastKeyFrameAtPosition(mFrame));
            if (vectorImage != nullptr)
            {
                BackupVectorElement* element = new BackupVectorElement(vectorImage);
                element->layer = backupLayer;
                element->frame = vectorImage->pos();
                element->undoText = undoText;
                element->somethingSelected = select()->somethingSelected();
                element->mySelection = select()->mySelectionRect();
                element->myTransformedSelection = select()->myTransformedSelectionRect();
                element->myTempTransformedSelection = select()->myTempTransformedSelectionRect();
                element->rotationAngle = select()->myRotation();
                mBackupList.append(element);
                mBackupIndex++;
            }
        }
        else if (layer->type() == Layer::SOUND)
        {
            int previous = layer->getPreviousKeyFramePosition(backupFrame);
            KeyFrame* key = layer->getLastKeyFrameAtPosition(backupFrame);

            // in case tracks overlap, get previous frame
            if (key == nullptr)
            {
                KeyFrame* previousKey = layer->getKeyFrameAt(previous);
                key = previousKey;
            }
            if (key != nullptr) {
                SoundClip* clip = static_cast<SoundClip*>(key);
                if (clip)
                {
                    BackupSoundElement* element = new BackupSoundElement(clip);
                    element->layer = backupLayer;
                    element->frame = backupFrame;
                    element->undoText = undoText;
                    element->fileName = clip->fileName();
                    element->originalName = clip->soundClipName();
                    mBackupList.append(element);
                    mBackupIndex++;
                }
            }
        }
    }

    updateAutoSaveCounter();

    emit updateBackup();
}

void Editor::sanitizeBackupElementsAfterLayerDeletion(int layerIndex)
{
    for (int i = 0; i < mBackupList.size(); i++)
    {
        BackupElement *backupElement = mBackupList[i];
        BackupBitmapElement *bitmapElement;
        BackupVectorElement *vectorElement;
        BackupSoundElement *soundElement;
        switch (backupElement->type())
        {
        case BackupElement::BITMAP_MODIF:
            bitmapElement = qobject_cast<BackupBitmapElement*>(backupElement);
            Q_ASSERT(bitmapElement);
            if (bitmapElement->layer > layerIndex)
            {
                bitmapElement->layer--;
                continue;
            }
            else if (bitmapElement->layer != layerIndex)
            {
                continue;
            }
            break;
        case BackupElement::VECTOR_MODIF:
            vectorElement = qobject_cast<BackupVectorElement*>(backupElement);
            Q_ASSERT(vectorElement);
            if (vectorElement->layer > layerIndex)
            {
                vectorElement->layer--;
                continue;
            }
            else if (vectorElement->layer != layerIndex)
            {
                continue;
            }
            break;
        case BackupElement::SOUND_MODIF:
            soundElement = qobject_cast<BackupSoundElement*>(backupElement);
            Q_ASSERT(soundElement);
            if (soundElement->layer > layerIndex)
            {
                soundElement->layer--;
                continue;
            }
            else if (soundElement->layer != layerIndex)
            {
                continue;
            }
            break;
        default:
            Q_UNREACHABLE();
        }
        if (i <= mBackupIndex)
        {
            mBackupIndex--;
        }
        delete mBackupList.takeAt(i);
        i--;
    }
}

void Editor::restoreKey()
{
    BackupElement* lastBackupElement = mBackupList[mBackupIndex];

    Layer* layer = nullptr;
    int frame = 0;
    int layerIndex = 0;
    if (lastBackupElement->type() == BackupElement::BITMAP_MODIF)
    {
        BackupBitmapElement* lastBackupBitmapElement = static_cast<BackupBitmapElement*>(lastBackupElement);
        layerIndex = lastBackupBitmapElement->layer;
        frame = lastBackupBitmapElement->frame;
        layer = object()->getLayer(layerIndex);
        addKeyFrame(layerIndex, frame);
        dynamic_cast<LayerBitmap*>(layer)->getBitmapImageAtFrame(frame)->paste(&lastBackupBitmapElement->bitmapImage);
        emit frameModified(frame);
    }
    if (lastBackupElement->type() == BackupElement::VECTOR_MODIF)
    {
        BackupVectorElement* lastBackupVectorElement = static_cast<BackupVectorElement*>(lastBackupElement);
        layerIndex = lastBackupVectorElement->layer;
        frame = lastBackupVectorElement->frame;
        layer = object()->getLayer(layerIndex);
        addKeyFrame(layerIndex, frame);
        dynamic_cast<LayerVector*>(layer)->getVectorImageAtFrame(frame)->paste(lastBackupVectorElement->vectorImage);
        emit frameModified(frame);
    }
    if (lastBackupElement->type() == BackupElement::SOUND_MODIF)
    {
        QString strSoundFile;
        BackupSoundElement* lastBackupSoundElement = static_cast<BackupSoundElement*>(lastBackupElement);
        layerIndex = lastBackupSoundElement->layer;
        frame = lastBackupSoundElement->frame;

        strSoundFile = lastBackupSoundElement->fileName;
        if (strSoundFile.isEmpty()) return;
        KeyFrame* key = addKeyFrame(layerIndex, frame);
        SoundClip* clip = dynamic_cast<SoundClip*>(key);
        if (clip)
        {
            Status st = sound()->loadSound(clip, lastBackupSoundElement->fileName);
            clip->setSoundClipName(lastBackupSoundElement->originalName);
            if (!st.ok())
            {
                removeKey();
                emit layers()->currentLayerChanged(layers()->currentLayerIndex()); // trigger timeline repaint.
            }
        }
    }
}

void Editor::undo()
{
    if (!mBackupList.empty() && mBackupIndex > -1)
    {
        if (mBackupIndex == mBackupList.size() - 1)
        {
            BackupElement* lastBackupElement = mBackupList[mBackupIndex];
            if (lastBackupElement->type() == BackupElement::BITMAP_MODIF)
            {
                BackupBitmapElement* lastBackupBitmapElement = static_cast<BackupBitmapElement*>(lastBackupElement);
                backup(lastBackupBitmapElement->layer, lastBackupBitmapElement->frame, "NoOp");
                mBackupIndex--;
            }
            if (lastBackupElement->type() == BackupElement::VECTOR_MODIF)
            {
                BackupVectorElement* lastBackupVectorElement = static_cast<BackupVectorElement*>(lastBackupElement);
                backup(lastBackupVectorElement->layer, lastBackupVectorElement->frame, "NoOp");
                mBackupIndex--;
            }
            if (lastBackupElement->type() == BackupElement::SOUND_MODIF)
            {
                BackupSoundElement* lastBackupSoundElement = static_cast<BackupSoundElement*>(lastBackupElement);
                backup(lastBackupSoundElement->layer, lastBackupSoundElement->frame, "NoOp");
                mBackupIndex--;
            }
        }

        qDebug() << "Undo" << mBackupIndex;
        mBackupList[mBackupIndex]->restore(this);
        mBackupIndex--;
        mScribbleArea->cancelTransformedSelection();

        Layer* layer = layers()->currentLayer();
        if (layer == nullptr) { return; }

        select()->resetSelectionTransform();
        if (layer->type() == Layer::VECTOR)
        {
            VectorImage *vectorImage = static_cast<VectorImage*>(layer->getKeyFrameAt(mFrame));
            vectorImage->calculateSelectionRect();
            select()->setSelection(vectorImage->getSelectionRect(), false);
        }
        emit updateBackup();
    }
}

void Editor::redo()
{
    if (!mBackupList.empty() && mBackupIndex < mBackupList.size() - 2)
    {
        mBackupIndex++;

        mBackupList[mBackupIndex + 1]->restore(this);
        emit updateBackup();
    }
}

void Editor::clearUndoStack()
{
    mBackupIndex = -1;
    while (!mBackupList.isEmpty())
    {
        delete mBackupList.takeLast();
    }
    mLastModifiedLayer = -1;
    mLastModifiedFrame = -1;
}

void Editor::updateAutoSaveCounter()
{
    if (mIsAutosave == false)
        return;

    mAutosaveCounter++;
    if (mAutosaveCounter >= mAutosaveNumber)
    {
        resetAutoSaveCounter();
        emit needSave();
    }
}

void Editor::resetAutoSaveCounter()
{
    mAutosaveCounter = 0;
}

void Editor::cut()
{
    copy();
    mScribbleArea->deleteSelection();
    deselectAll();
}

void Editor::copy()
{
    Layer* layer = mObject->getLayer(layers()->currentLayerIndex());
    if (layer == nullptr)
    {
        return;
    }

    if (layer->type() == Layer::BITMAP)
    {
        LayerBitmap* layerBitmap = static_cast<LayerBitmap*>(layer);
        BitmapImage* bitmapImage = layerBitmap->getLastBitmapImageAtFrame(currentFrame(), 0);
        if (bitmapImage == nullptr) { return; }
        if (select()->somethingSelected())
        {
            g_clipboardBitmapImage = bitmapImage->copy(select()->mySelectionRect().toRect());  // copy part of the image
        }
        else
        {
            g_clipboardBitmapImage = *bitmapImage;  // copy the whole image
        }
        clipboardBitmapOk = true;
        if (g_clipboardBitmapImage.image() != nullptr)
            QApplication::clipboard()->setImage(*g_clipboardBitmapImage.image());
    }
    if (layer->type() == Layer::VECTOR)
    {
        clipboardVectorOk = true;
        VectorImage *vectorImage = static_cast<VectorImage*>(layer->getLastKeyFrameAtPosition(currentFrame()));
        if (vectorImage == nullptr) { return; }
        g_clipboardVectorImage = *vectorImage;  // copy the image
    }
}

void Editor::paste()
{
    Layer* layer = mObject->getLayer(layers()->currentLayerIndex());
    if (layer != nullptr)
    {
        if (layer->type() == Layer::BITMAP && g_clipboardBitmapImage.image() != nullptr)
        {
            backup(tr("Paste"));

            BitmapImage tobePasted = g_clipboardBitmapImage.copy();
            qDebug() << "to be pasted --->" << tobePasted.image()->size();
            if (select()->somethingSelected())
            {
                QRectF selection = select()->mySelectionRect();
                if (g_clipboardBitmapImage.width() <= selection.width() && g_clipboardBitmapImage.height() <= selection.height())
                {
                    tobePasted.moveTopLeft(selection.topLeft());
                }
                else
                {
                    tobePasted.transform(selection, true);
                }
            }
            mScribbleArea->handleDrawingOnEmptyFrame();
            BitmapImage *bitmapImage = static_cast<BitmapImage*>(layer->getLastKeyFrameAtPosition(currentFrame()));
            Q_CHECK_PTR(bitmapImage);
            bitmapImage->paste(&tobePasted); // paste the clipboard
        }
        else if (layer->type() == Layer::VECTOR && clipboardVectorOk)
        {
            backup(tr("Paste"));
            deselectAll();
            mScribbleArea->handleDrawingOnEmptyFrame();
            VectorImage* vectorImage = static_cast<VectorImage*>(layer->getLastKeyFrameAtPosition(currentFrame()));
            Q_CHECK_PTR(vectorImage);
            vectorImage->paste(g_clipboardVectorImage);  // paste the clipboard
            select()->setSelection(vectorImage->getSelectionRect(), false);
        }
    }
    emit frameModified(mFrame);
}

void Editor::flipSelection(bool flipVertical)
{
    mScribbleArea->flipSelection(flipVertical);
}

void Editor::clipboardChanged()
{
    if (clipboardBitmapOk == false)
    {
        g_clipboardBitmapImage.setImage(new QImage(QApplication::clipboard()->image()));
        g_clipboardBitmapImage.bounds() = QRect(g_clipboardBitmapImage.topLeft(), g_clipboardBitmapImage.image()->size());
        //qDebug() << "New clipboard image" << g_clipboardBitmapImage.image()->size();
    }
    else
    {
        clipboardBitmapOk = false;
        //qDebug() << "The image has been saved in the clipboard";
    }
}

void Editor::setLayerVisibility(LayerVisibility visibility) {
    mScribbleArea->setLayerVisibility(visibility);
    emit updateTimeLine();
}

LayerVisibility Editor::layerVisibility()
{
    return mScribbleArea->getLayerVisibility();
}

void Editor::increaseLayerVisibilityIndex()
{
    mScribbleArea->increaseLayerVisibilityIndex();
    emit updateTimeLine();
}

void Editor::decreaseLayerVisibilityIndex()
{
    mScribbleArea->decreaseLayerVisibilityIndex();
    emit updateTimeLine();
}

void Editor::addTemporaryDir(QTemporaryDir* const dir)
{
    mTemporaryDirs.append(dir);
}

void Editor::clearTemporary()
{
    while(!mTemporaryDirs.isEmpty())
    {
        QTemporaryDir* t = mTemporaryDirs.takeLast();
        t->remove();
        delete t;
    }
}

Status Editor::openObject(const QString& strFilePath, const std::function<void(int)>& progressChanged, const std::function<void(int)>& progressRangeChanged)
{
    // Check for potential issues with the file
    Q_ASSERT(!strFilePath.isEmpty());
    QFileInfo fileInfo(strFilePath);
    DebugDetails dd;
    dd << QString("Raw file path: %1").arg(strFilePath);
    dd << QString("Resolved file path: %1").arg(fileInfo.absoluteFilePath());
    if (fileInfo.isDir())
    {
        return Status(Status::ERROR_FILE_CANNOT_OPEN,
                      dd,
                      tr("Could not open file"),
                      tr("The file you have selected is a directory, so we are unable to open it. "
                         "If you are are trying to open a project that uses the old structure, "
                         "please open the file ending with .pcl, not the data folder."));
    }
    if (!fileInfo.exists())
    {
        return Status(Status::FILE_NOT_FOUND,
                      dd,
                      tr("Could not open file"),
                      tr("The file you have selected does not exist, so we are unable to open it. "
                         "Please make sure that you've entered the correct path and that the file is accessible and try again."));
    }
    if (!fileInfo.isReadable())
    {
        dd << QString("Permissions: 0x%1").arg(QString::number(fileInfo.permissions(), 16));
        return Status(Status::ERROR_FILE_CANNOT_OPEN,
                      dd,
                      tr("Could not open file"),
                      tr("This program does not have permission to read the file you have selected. "
                         "Please check that you have read permissions for this file and try again."));
    }

    int progress = 0;
    FileManager fm(this);
    connect(&fm, &FileManager::progressChanged, [&progress, &progressChanged](int p)
    {
        progressChanged(progress = p);
    });
    connect(&fm, &FileManager::progressRangeChanged, [&progressRangeChanged](int max)
    {
        progressRangeChanged(max + 3);
    });

    QString fullPath = fileInfo.absoluteFilePath();

    Object* object = fm.load(fullPath);

    Status fmStatus = fm.error();
    if (!fmStatus.ok())
    {
        dd.collect(fmStatus.details());
        fmStatus.setDetails(dd);
        return fmStatus;
    }

    if (object == nullptr)
    {
        return Status(Status::ERROR_FILE_CANNOT_OPEN,
                      dd,
                      tr("Could not open file"),
                      tr("An unknown error occurred while trying to load the file and we are not able to load your file."));
    }

    setObject(object);

    progressChanged(progress + 1);

    layers()->notifyAnimationLengthChanged();
    setFps(playback()->fps());

    return Status::OK;
}

Status Editor::setObject(Object* newObject)
{
    Q_ASSERT(newObject);

    if (newObject == mObject.get())
    {
        return Status::SAFE;
    }

    clearUndoStack();
    mObject.reset(newObject);

    g_clipboardVectorImage.setObject(newObject);

    updateObject();

    // Make sure that object is fully loaded before calling managers.
    for (BaseManager* m : mAllManagers)
    {
        m->load(mObject.get());
    }

    if (mViewManager)
    {
        connect(newObject, &Object::layerViewChanged, mViewManager, &ViewManager::viewChanged);
    }

    emit objectLoaded();

    return Status::OK;
}

void Editor::updateObject()
{
    setCurrentLayerIndex(mObject->data()->getCurrentLayer());
    scrubTo(mObject->data()->getCurrentFrame());

    mAutosaveCounter = 0;
    mAutosaveNeverAskAgain = false;

    if (mPreferenceManager)
    {
        mObject->setActiveFramePoolSize(mPreferenceManager->getInt(SETTING::FRAME_POOL_SIZE));
    }

    emit updateLayerCount();
}

bool Editor::importBitmapImage(const QString& filePath, int space)
{
    QImageReader reader(filePath);

    Q_ASSERT(layers()->currentLayer()->type() == Layer::BITMAP);
    auto layer = static_cast<LayerBitmap*>(layers()->currentLayer());

    QImage img(reader.size(), QImage::Format_ARGB32_Premultiplied);
    if (img.isNull())
    {
        return false;
    }

    const QPoint pos(view()->getImportView().dx() - (img.width() / 2),
                     view()->getImportView().dy() - (img.height() / 2));

    while (reader.read(&img))
    {
        int frameNumber = mFrame;
        if (!layer->keyExists(frameNumber))
        {
            addNewKey();
        }
        BitmapImage* bitmapImage = layer->getBitmapImageAtFrame(frameNumber);
        BitmapImage importedBitmapImage(pos, img);
        bitmapImage->paste(&importedBitmapImage);
        emit frameModified(bitmapImage->pos());

        if (space > 1) {
            frameNumber += space;
        } else {
            frameNumber += 1;
        }
        scrubTo(frameNumber);

        backup(tr("Import Image"));

        // Workaround for tiff import getting stuck in this loop
        if (!reader.supportsAnimation())
        {
            break;
        }
    }

    return true;
}

bool Editor::importVectorImage(const QString& filePath)
{
    Q_ASSERT(layers()->currentLayer()->type() == Layer::VECTOR);

    auto layer = static_cast<LayerVector*>(layers()->currentLayer());

    VectorImage* vectorImage = layer->getVectorImageAtFrame(currentFrame());
    if (vectorImage == nullptr)
    {
        addNewKey();
        vectorImage = layer->getVectorImageAtFrame(currentFrame());
    }

    VectorImage importedVectorImage;
    bool ok = importedVectorImage.read(filePath);
    if (ok)
    {
        importedVectorImage.selectAll();
        vectorImage->paste(importedVectorImage);
        emit frameModified(importedVectorImage.pos());

        backup(tr("Import Image"));
    }

    return ok;
}

bool Editor::importImage(const QString& filePath)
{
    Layer* layer = layers()->currentLayer();

    if (view()->getImportFollowsCamera())
    {
        LayerCamera* camera = static_cast<LayerCamera*>(layers()->getLastCameraLayer());
        QTransform transform = camera->getViewAtFrame(currentFrame());
        view()->setImportView(transform);
    }
    switch (layer->type())
    {
    case Layer::BITMAP:
        return importBitmapImage(filePath);

    case Layer::VECTOR:
        return importVectorImage(filePath);

    default:
    {
        //mLastError = Status::ERROR_INVALID_LAYER_TYPE;
        return false;
    }
    }
}

bool Editor::importGIF(const QString& filePath, int numOfImages)
{
    Layer* layer = layers()->currentLayer();
    if (layer->type() == Layer::BITMAP)
    {
        return importBitmapImage(filePath, numOfImages);
    }
    return false;
}

void Editor::selectAll() const
{
    Layer* layer = layers()->currentLayer();

    QRectF rect;
    if (layer->type() == Layer::BITMAP)
    {
        // Selects the drawn area (bigger or smaller than the screen). It may be more accurate to select all this way
        // as the drawing area is not limited
        BitmapImage *bitmapImage = static_cast<BitmapImage*>(layer->getLastKeyFrameAtPosition(mFrame));
        if (bitmapImage == nullptr) { return; }

        rect = bitmapImage->bounds();
    }
    else if (layer->type() == Layer::VECTOR)
    {
        VectorImage *vectorImage = static_cast<VectorImage*>(layer->getLastKeyFrameAtPosition(mFrame));
        if (vectorImage != nullptr)
        {
            vectorImage->selectAll();
            rect = vectorImage->getSelectionRect();
        }
    }
    select()->setSelection(rect, false);
}

void Editor::deselectAll() const
{
    Layer* layer = layers()->currentLayer();
    if (layer == nullptr) { return; }

    if (layer->type() == Layer::VECTOR)
    {
        VectorImage *vectorImage = static_cast<VectorImage*>(layer->getLastKeyFrameAtPosition(mFrame));
        if (vectorImage != nullptr)
        {
            vectorImage->deselectAll();
        }
    }

    select()->resetSelectionProperties();
}

void Editor::updateFrame(int frameNumber)
{
    mScribbleArea->updateFrame(frameNumber);
}

void Editor::updateCurrentFrame()
{
    mScribbleArea->updateCurrentFrame();
}

void Editor::setCurrentLayerIndex(int i)
{
    mCurrentLayerIndex = i;

    Layer* layer = mObject->getLayer(i);
    for (auto mgr : mAllManagers)
    {
        mgr->workingLayerChanged(layer);
    }
}

void Editor::scrubTo(int frame)
{
    if (frame < 1) { frame = 1; }
    mFrame = frame;

    emit scrubbed(frame);

    // FIXME: should not emit Timeline update here.
    // Editor must be an individual class.
    // Will remove all Timeline related code in Editor class.
    if (mPlaybackManager && !mPlaybackManager->isPlaying())
    {
        emit updateTimeLine(); // needs to update the timeline to update onion skin positions
    }
    mObject->updateActiveFrames(frame);
}

void Editor::scrubForward()
{
    int nextFrame = mFrame + 1;
    if (!playback()->isPlaying()) {
        playback()->playScrub(nextFrame);
    }
    scrubTo(nextFrame);
}

void Editor::scrubBackward()
{
    if (currentFrame() > 1)
    {
        int previousFrame = mFrame - 1;
        if (!playback()->isPlaying()) {
            playback()->playScrub(previousFrame);
        }
        scrubTo(previousFrame);
    }
}

KeyFrame* Editor::addNewKey()
{
    return addKeyFrame(layers()->currentLayerIndex(), currentFrame());
}

KeyFrame* Editor::addKeyFrame(int layerNumber, int frameIndex)
{
    Layer* layer = mObject->getLayer(layerNumber);
    Q_ASSERT(layer);

    if (!layer->visible())
    {
        mScribbleArea->showLayerNotVisibleWarning();
        return nullptr;
    }

    // Find next available space for a keyframe (where either no key exists or there is an empty sound key)
    while (layer->keyExists(frameIndex))
    {
        if (layer->type() == Layer::SOUND
            && layer->getKeyFrameAt(frameIndex)->fileName().isEmpty()
            && layer->removeKeyFrame(frameIndex))
        {
            break;
        }
        else
        {
            frameIndex += 1;
        }
    }

    bool ok = layer->addNewKeyFrameAt(frameIndex);
    if (ok)
    {
        scrubTo(frameIndex); // currentFrameChanged() emit inside.
        emit frameModified(frameIndex);
        layers()->notifyAnimationLengthChanged();
        if (layer->type() == Layer::CAMERA) // to update camera path
            emit layer->keyframeAdded(frameIndex);
    }
    return layer->getKeyFrameAt(frameIndex);
}

void Editor::removeKey()
{
    Layer* layer = layers()->currentLayer();
    Q_ASSERT(layer != nullptr);

    if (!layer->visible())
    {
        mScribbleArea->showLayerNotVisibleWarning();
        return;
    }

    if (!layer->keyExistsWhichCovers(currentFrame()))
    {
        return;
    }

    backup(tr("Remove frame"));

    deselectAll();
    layer->removeKeyFrame(currentFrame());
    if (layer->type() == Layer::CAMERA)
        emit layer->keyframeDeleted(currentFrame());
    scrubBackward();
    layers()->notifyAnimationLengthChanged();
    emit layers()->currentLayerChanged(layers()->currentLayerIndex()); // trigger timeline repaint.
}

void Editor::scrubNextKeyFrame()
{
    Layer* layer = layers()->currentLayer();
    Q_ASSERT(layer);

    int nextPosition = layer->getNextKeyFramePosition(currentFrame());
    scrubTo(nextPosition);
}

void Editor::scrubPreviousKeyFrame()
{
    Layer* layer = mObject->getLayer(layers()->currentLayerIndex());
    Q_ASSERT(layer);

    int prevPosition = layer->getPreviousKeyFramePosition(currentFrame());
    scrubTo(prevPosition);
}

void Editor::switchVisibilityOfLayer(int layerNumber)
{
    Layer* layer = mObject->getLayer(layerNumber);
    if (layer != nullptr) layer->switchVisibility();
    mScribbleArea->onLayerChanged();

    emit updateTimeLine();
}

void Editor::swapLayers(int i, int j)
{
    mObject->swapLayers(i, j);
    if (j < i)
    {
        layers()->setCurrentLayer(j + 1);
    }
    else
    {
        layers()->setCurrentLayer(j - 1);
    }
    emit updateTimeLine();
    mScribbleArea->onLayerChanged();
}

void Editor::prepareSave()
{
    for (auto mgr : mAllManagers)
    {
        mgr->save(mObject.get());
    }
}

void Editor::clearCurrentFrame()
{
    mScribbleArea->clearImage();
}
