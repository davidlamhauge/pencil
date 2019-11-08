#include "framecommentwidget.h"
#include "ui_framecommentwidget.h"

#include "editor.h"
#include "layermanager.h"
#include "playbackmanager.h"
#include <QDebug>

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
    dialogueTextChanged();
    actionTextChanged();
    notesTextChanged();
    updateConnections();
}

void FrameCommentWidget::updateUI()
{

}

void FrameCommentWidget::setCore(Editor *editor)
{
    mEditor = editor;
    mLayer = editor->layers()->currentLayer();
    mKeyframe = mLayer->getKeyFrameAt(mEditor->currentFrame());
    currentFrameChanged(mEditor->currentFrame());
}

void FrameCommentWidget::dialogueTextChanged()
{
    QString text = ui->pteDialogue->toPlainText();
    int len = text.length();
    ui->labDialogueCount->setText(tr("%1 chars").arg(QString::number(len)));
    if (len < 251)
        ui->btnApplyComments->setEnabled(true);
    else
        ui->btnApplyComments->setEnabled(false);
}

void FrameCommentWidget::actionTextChanged()
{
    QString text = ui->pteAction->toPlainText();
    int len = text.length();
    ui->labActionCount->setText(tr("%1 chars").arg(QString::number(len)));
    if (len < 251)
        ui->btnApplyComments->setEnabled(true);
    else
        ui->btnApplyComments->setEnabled(false);
}

void FrameCommentWidget::notesTextChanged()
{
    QString text = ui->pteNotes->toPlainText();
    int len = text.length();
    ui->labNotesCount->setText(tr("%1 chars").arg(QString::number(len)));
    if (len < 251)
        ui->btnApplyComments->setEnabled(true);
    else
        ui->btnApplyComments->setEnabled(false);
}

void FrameCommentWidget::currentFrameChanged(int frame)
{
    if (!mIsPlaying)
    {
        if (mLayer->firstKeyFramePosition() <= frame)
        {
            fillFrameComments();
            ui->btnApplyComments->setEnabled(true);
        }
        else
        {
            clearFrameCommentsFields();
            ui->btnApplyComments->setEnabled(false);
        }
    }
}

void FrameCommentWidget::currentLayerChanged(int index)
{
    Q_UNUSED(index)
    mLayer = mEditor->layers()->currentLayer();
    currentFrameChanged(mEditor->currentFrame());
}

void FrameCommentWidget::clearFrameCommentsFields()
{
    ui->pteDialogue->clear();
    ui->pteAction->clear();
    ui->pteNotes->clear();
    if (!mLayer->keyExists(mEditor->currentFrame()))
        ui->labLayerFrame->setText("");
}

void FrameCommentWidget::applyFrameComments()
{
    mKeyframe = mLayer->getKeyFrameAt(mEditor->currentFrame());
    if (mKeyframe == nullptr)
        mKeyframe = mLayer->getKeyFrameAt(mLayer->getPreviousFrameNumber(mEditor->currentFrame(), true));
    if (mKeyframe == nullptr) { return; }
    mKeyframe->setDialogueComment(ui->pteDialogue->toPlainText());
    mKeyframe->setActionComment(ui->pteAction->toPlainText());
    mKeyframe->setSlugComment(ui->pteNotes->toPlainText());
    ui->labLayerFrame->setText(tr("%1 #%2 :").arg(mLayer->name()).arg(QString::number(mKeyframe->pos())));
    mLayer->setModified(mKeyframe->pos(), true);
}

void FrameCommentWidget::playStateChanged(bool isPlaying)
{
    mIsPlaying = isPlaying;
    if (!mIsPlaying)
    {
        mLayer = mEditor->layers()->currentLayer();
        currentFrameChanged(mEditor->currentFrame());
    }
}

void FrameCommentWidget::updateConnections()
{
    if (isVisible())
    {
        connectAll();
    }
    else
    {
        disconnectAll();
    }
}

void FrameCommentWidget::fillFrameComments()
{
    mKeyframe = mLayer->getKeyFrameAt(mEditor->currentFrame());
    if (mKeyframe == nullptr)
        mKeyframe = mLayer->getKeyFrameAt(mLayer->getPreviousFrameNumber(mEditor->currentFrame(), true));
    if (mKeyframe == nullptr) { return; }
    ui->pteDialogue->setPlainText(mKeyframe->getDialogueComment());
    ui->pteAction->setPlainText(mKeyframe->getActionComment());
    ui->pteNotes->setPlainText(mKeyframe->getSlugComment());
    ui->labLayerFrame->setText(tr("%1 #%2 :").arg(mEditor->layers()->currentLayer()->name()).arg(QString::number(mKeyframe->pos())));
}

void FrameCommentWidget::connectAll()
{
    connect(ui->pteDialogue, &QPlainTextEdit::textChanged, this, &FrameCommentWidget::dialogueTextChanged);
    connect(ui->pteAction, &QPlainTextEdit::textChanged, this, &FrameCommentWidget::actionTextChanged);
    connect(ui->pteNotes, &QPlainTextEdit::textChanged, this, &FrameCommentWidget::notesTextChanged);
    connect(mEditor, &Editor::currentFrameChanged, this, &FrameCommentWidget::currentFrameChanged);
    connect(mEditor->layers(), &LayerManager::currentLayerChanged, this, &FrameCommentWidget::currentLayerChanged);
    connect(ui->btnClearFields, &QPushButton::clicked, this, &FrameCommentWidget::clearFrameCommentsFields);
    connect(ui->btnApplyComments, &QPushButton::clicked, this, &FrameCommentWidget::applyFrameComments);
    connect(mEditor, &Editor::objectLoaded, this, &FrameCommentWidget::fillFrameComments);
    connect(mEditor->playback(), &PlaybackManager::playStateChanged, this, &FrameCommentWidget::playStateChanged);
}

void FrameCommentWidget::disconnectAll()
{
    disconnect(ui->pteDialogue, &QPlainTextEdit::textChanged, this, &FrameCommentWidget::dialogueTextChanged);
    disconnect(ui->pteAction, &QPlainTextEdit::textChanged, this, &FrameCommentWidget::actionTextChanged);
    disconnect(ui->pteNotes, &QPlainTextEdit::textChanged, this, &FrameCommentWidget::notesTextChanged);
    disconnect(mEditor, &Editor::currentFrameChanged, this, &FrameCommentWidget::currentFrameChanged);
    disconnect(mEditor->layers(), &LayerManager::currentLayerChanged, this, &FrameCommentWidget::currentLayerChanged);
    disconnect(ui->btnClearFields, &QPushButton::clicked, this, &FrameCommentWidget::clearFrameCommentsFields);
    disconnect(ui->btnApplyComments, &QPushButton::clicked, this, &FrameCommentWidget::applyFrameComments);
    disconnect(mEditor, &Editor::objectLoaded, this, &FrameCommentWidget::fillFrameComments);
    disconnect(mEditor->playback(), &PlaybackManager::playStateChanged, this, &FrameCommentWidget::playStateChanged);
}

