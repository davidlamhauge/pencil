#include "framecommentwidget.h"
#include "ui_framecommentwidget.h"

#include "editor.h"
#include "keyframe.h"
#include "framecommentdialog.h"

#include "layermanager.h"
#include "playbackmanager.h"

FrameCommentWidget::FrameCommentWidget(QWidget *parent) :
    BaseDockWidget(parent)
{
    QWidget* innerWidget = new QWidget;
    setWindowTitle(tr("Frame Comments"));

    ui = new Ui::FrameComment;
    ui->setupUi(innerWidget);
    setWidget(innerWidget);
}

FrameCommentWidget::~FrameCommentWidget()
{
    delete ui;
}

void FrameCommentWidget::initUI()
{
    connect(this, &FrameCommentWidget::visibilityChanged, this, &FrameCommentWidget::updateConnections);
    updateConnections();
    ui->labLayerName->setText(mEditor->layers()->currentLayer()->name());
}

void FrameCommentWidget::updateUI()
{
}

void FrameCommentWidget::setCore(Editor *editor)
{
    mEditor = editor;
}

void FrameCommentWidget::dialogueTextChanged()
{
    if (mKeyFrame == nullptr)
        return;
    mKeyFrame->setDialogueComment(mFrameCommentDialog->getCurrentKeyFrame()->getDialogueComment());
    QSignalBlocker b(ui->textEditDialogue);
    ui->textEditDialogue->setPlainText(mKeyFrame->getDialogueComment());
    int len = ui->textEditDialogue->toPlainText().length();
    ui->labelDialogueCounter->setText(tr("%1 chars").arg(QString::number(len)));
}

void FrameCommentWidget::actionTextChanged()
{
    if (!mKeyFrame)
        return;
    mKeyFrame->setActionComment(mFrameCommentDialog->getCurrentKeyFrame()->getActionComment());
    QSignalBlocker b(ui->textEditAction);
    ui->textEditAction->setPlainText(mKeyFrame->getActionComment());
    int len = ui->textEditAction->toPlainText().length();
    ui->labelActionCounter->setText(tr("%1 chars").arg(QString::number(len)));
}

void FrameCommentWidget::slugTextChanged()
{
    if (!mKeyFrame)
        return;
    mKeyFrame->setSlugComment(mFrameCommentDialog->getCurrentKeyFrame()->getSlugComment());
    QSignalBlocker b(ui->textEditSlug);
    ui->textEditSlug->setPlainText(mKeyFrame->getSlugComment());
    int len = ui->textEditSlug->toPlainText().length();
    ui->labelSlugCounter->setText(tr("%1 chars").arg(QString::number(len)));
}

void FrameCommentWidget::currentFrameChanged(int frame)
{
    if (!mIsPlaying)
    {
        if (frame >= mEditor->layers()->currentLayer()->firstKeyFramePosition())
        {
            mKeyFrame = getKeyFrame();
            fillComments();
        }
    }
}

void FrameCommentWidget::currentLayerChanged(int index)
{
    Q_UNUSED(index)
    currentFrameChanged(mEditor->currentFrame());
    ui->labLayerName->setText(mEditor->layers()->currentLayer()->name());
}

void FrameCommentWidget::playStateChanged(bool isPlaying)
{
    mIsPlaying = isPlaying;
    if (!mIsPlaying)
    {
        currentFrameChanged(mEditor->currentFrame());
        ui->btnEditComments->setEnabled(true);
    }
    else
    {
        ui->btnEditComments->setEnabled(false);
    }
}

void FrameCommentWidget::updateConnections()
{
    if (!isVisible())
    {
        disconnectNotifiers();
    }
    else
    {
        makeConnections();
    }
}

void FrameCommentWidget::fillComments()
{
    if (mKeyFrame == nullptr) { return; }

    QSignalBlocker b(ui->textEditDialogue);
    QSignalBlocker b2(ui->textEditAction);
    QSignalBlocker b3(ui->textEditSlug);

    ui->textEditDialogue->setPlainText(mKeyFrame->getDialogueComment());
    ui->textEditAction->setPlainText(mKeyFrame->getActionComment());
    ui->textEditSlug->setPlainText(mKeyFrame->getSlugComment());
}

void FrameCommentWidget::openFrameCommentDialog()
{
    if (mFrameCommentDialog != nullptr)
        return;

    mFrameCommentDialog = new FrameCommentDialog(this);
    mFrameCommentDialog->setModal(true);
    mFrameCommentDialog->setAttribute(Qt::WA_DeleteOnClose);
    mFrameCommentDialog->setCore(mEditor);
    mFrameCommentDialog->initUI();
    mFrameCommentDialog->setWindowFlags(mFrameCommentDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    mFrameCommentDialog->show();
    connect(mFrameCommentDialog, &FrameCommentDialog::dialogueCommentChanged, this, &FrameCommentWidget::dialogueTextChanged);
    connect(mFrameCommentDialog, &FrameCommentDialog::actionCommentChanged, this, &FrameCommentWidget::actionTextChanged);
    connect(mFrameCommentDialog, &FrameCommentDialog::slugCommentChanged, this, &FrameCommentWidget::slugTextChanged);
    connect(mFrameCommentDialog, &FrameCommentDialog::currentFrameChanged, this, &FrameCommentWidget::currentFrameChanged);

    connect(mFrameCommentDialog, &FrameCommentDialog::finished, [=]
    {
        disconnect(mFrameCommentDialog, &FrameCommentDialog::dialogueCommentChanged, this, &FrameCommentWidget::dialogueTextChanged);
        disconnect(mFrameCommentDialog, &FrameCommentDialog::actionCommentChanged, this, &FrameCommentWidget::actionTextChanged);
        disconnect(mFrameCommentDialog, &FrameCommentDialog::slugCommentChanged, this, &FrameCommentWidget::slugTextChanged);
        disconnect(mFrameCommentDialog, &FrameCommentDialog::currentFrameChanged, this, &FrameCommentWidget::currentFrameChanged);
        mFrameCommentDialog = nullptr;
    });

    if (mEditor->layers()->currentLayer()->keyExists(mEditor->currentFrame()))
    {
        mKeyFrame = getKeyFrame();
    }
}

KeyFrame* FrameCommentWidget::getKeyFrame()
{
    const int frame = mEditor->currentFrame();
    const Layer* layer = mEditor->layers()->currentLayer();
    KeyFrame* keyframe = layer->getKeyFrameAt(frame);
    if (keyframe == nullptr)
        keyframe = layer->getKeyFrameAt(layer->getPreviousFrameNumber(frame, true));
    if (keyframe == nullptr) { return nullptr; }

    return keyframe;
}

void FrameCommentWidget::makeConnections()
{
    connect(ui->btnEditComments, &QPushButton::clicked, this, &FrameCommentWidget::openFrameCommentDialog);

    connect(mEditor->layers(), &LayerManager::currentLayerChanged, this, &FrameCommentWidget::currentLayerChanged);
    connect(mEditor, &Editor::objectLoaded, this, &FrameCommentWidget::fillComments);
    connect(mEditor->playback(), &PlaybackManager::playStateChanged, this, &FrameCommentWidget::playStateChanged);

}

void FrameCommentWidget::disconnectNotifiers()
{
    disconnect(ui->btnEditComments, &QPushButton::clicked, this, &FrameCommentWidget::openFrameCommentDialog);

    disconnect(mEditor->layers(), &LayerManager::currentLayerChanged, this, &FrameCommentWidget::currentLayerChanged);
    disconnect(mEditor, &Editor::objectLoaded, this, &FrameCommentWidget::fillComments);
    disconnect(mEditor->playback(), &PlaybackManager::playStateChanged, this, &FrameCommentWidget::playStateChanged);
}
