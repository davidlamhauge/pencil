#include "framecommentdialog.h"
#include "ui_framecommentdialog.h"

#include "layermanager.h"
#include "framecommentwidget.h"
#include "keyframe.h"

FrameCommentDialog::FrameCommentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FrameCommentDialog)
{
    ui->setupUi(this);
}

FrameCommentDialog::~FrameCommentDialog()
{
    delete ui;
}

void FrameCommentDialog::setCore(Editor *editor)
{
    mEditor = editor;
    mLayerManager = mEditor->layers();
    mCommentWidget = new FrameCommentWidget();
}

void FrameCommentDialog::initUI()
{
    connect(ui->btnDone, &QPushButton::clicked, this, &FrameCommentDialog::close);
    connect(ui->btnFirstFrame, &QPushButton::clicked, this, &FrameCommentDialog::scrubToFirstFrame);
    connect(ui->btnPreviousFrame, &QPushButton::clicked, this, &FrameCommentDialog::scrubToPreviousFrame);
    connect(ui->btnNextFrame, &QPushButton::clicked, this, &FrameCommentDialog::scrubToNextFrame);
    connect(ui->btnLastFrame, &QPushButton::clicked, this, &FrameCommentDialog::scrubToLastFrame);
    connect(ui->textEditDialogue, &QPlainTextEdit::textChanged, this, &FrameCommentDialog::updateDialogue);
    connect(ui->textEditAction, &QPlainTextEdit::textChanged, this, &FrameCommentDialog::updateAction);
    connect(ui->textEditSlug, &QPlainTextEdit::textChanged, this, &FrameCommentDialog::updateSlug);

    for (int i = mLayerManager->getLastLayerIndex(); i > -1; i--)
    {
        ui->comboBoxLayers->addItem(QString::number(i) + "_" + mLayerManager->getLayer(i)->name());
    }
    ui->comboBoxLayers->setCurrentIndex(mLayerManager->count() - 1 - mLayerManager->currentLayerIndex());
    connect(ui->comboBoxLayers, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FrameCommentDialog::layerChanged);

    updateNavigation();
}

KeyFrame* FrameCommentDialog::getCurrentKeyFrame()
{
    return mKeyframe;
}

void FrameCommentDialog::scrubToFirstFrame()
{
    mEditor->scrubTo(mLayerManager->currentLayer()->firstKeyFramePosition());
    emit currentFrameChanged(mEditor->currentFrame());
    ui->labFrameNumber->setText(QString::number(mEditor->currentFrame()));
    mKeyframe = mLayerManager->currentLayer()->getKeyFrameAt(mEditor->currentFrame());
    initCommentFields(mKeyframe);
    updateNavigation();
    if (!mInputFieldsEnabled)
    {
        enableInputFields();
        mInputFieldsEnabled = true;
    }
}

void FrameCommentDialog::scrubToPreviousFrame()
{
    mEditor->scrubTo(mLayerManager->currentLayer()->getPreviousKeyFramePosition(mEditor->currentFrame()));
    emit currentFrameChanged(mEditor->currentFrame());
    ui->labFrameNumber->setText(QString::number(mEditor->currentFrame()));
    mKeyframe = mLayerManager->currentLayer()->getKeyFrameAt(mEditor->currentFrame());
    initCommentFields(mKeyframe);
    updateNavigation();
    if (!mInputFieldsEnabled)
    {
        enableInputFields();
        mInputFieldsEnabled = true;
    }
}

void FrameCommentDialog::scrubToNextFrame()
{
    mEditor->scrubTo(mLayerManager->currentLayer()->getNextKeyFramePosition(mEditor->currentFrame()));
    emit currentFrameChanged(mEditor->currentFrame());
    ui->labFrameNumber->setText(QString::number(mEditor->currentFrame()));
    mKeyframe = mLayerManager->currentLayer()->getKeyFrameAt(mEditor->currentFrame());
    initCommentFields(mKeyframe);
    updateNavigation();
    if (!mInputFieldsEnabled)
    {
        enableInputFields();
        mInputFieldsEnabled = true;
    }
}

void FrameCommentDialog::scrubToLastFrame()
{
    mEditor->scrubTo(mLayerManager->currentLayer()->getMaxKeyFramePosition());
    emit currentFrameChanged(mEditor->currentFrame());
    ui->labFrameNumber->setText(QString::number(mEditor->currentFrame()));
    mKeyframe = mLayerManager->currentLayer()->getKeyFrameAt(mEditor->currentFrame());
    initCommentFields(mKeyframe);
    updateNavigation();
    if (!mInputFieldsEnabled)
    {
        enableInputFields();
        mInputFieldsEnabled = true;
    }
}

void FrameCommentDialog::layerChanged(int index)
{
    QString name = ui->comboBoxLayers->itemText(index);
    int len = name.length();
    QString layerIndex = name.remove(1, len - 1);
    index = layerIndex.toInt();
    mEditor->layers()->setCurrentLayer(index);

    updateNavigation();
}

void FrameCommentDialog::updateDialogue()
{
    mKeyframe->setDialogueComment(ui->textEditDialogue->toPlainText());
    mKeyframe->setModified(true);
    emit dialogueCommentChanged();
}

void FrameCommentDialog::updateAction()
{
    mKeyframe->setActionComment(ui->textEditAction->toPlainText());
    mKeyframe->setModified(true);
    emit actionCommentChanged();
}

void FrameCommentDialog::updateSlug()
{
    mKeyframe->setSlugComment(ui->textEditSlug->toPlainText());
    mKeyframe->setModified(true);
    emit slugCommentChanged();
}

void FrameCommentDialog::initCommentFields(KeyFrame *key)
{
    if (key == nullptr)
        return;
    ui->textEditDialogue->setPlainText(key->getDialogueComment());
    ui->textEditAction->setPlainText(key->getActionComment());
    ui->textEditSlug->setPlainText(key->getSlugComment());
}

void FrameCommentDialog::enableInputFields()
{
    ui->textEditDialogue->setEnabled(true);
    ui->textEditAction->setEnabled(true);
    ui->textEditSlug->setEnabled(true);
    mInputFieldsEnabled = true;
}

void FrameCommentDialog::disableInputFields()
{
    if (!mLayerManager->currentLayer()->keyExists(mEditor->currentFrame()))
    {
        QSignalBlocker b(ui->textEditDialogue);
        ui->textEditDialogue->clear();
        QSignalBlocker b2(ui->textEditAction);
        ui->textEditAction->clear();
        QSignalBlocker b3(ui->textEditSlug);
        ui->textEditSlug->clear();
    }

    ui->textEditDialogue->setEnabled(false);
    ui->textEditAction->setEnabled(false);
    ui->textEditSlug->setEnabled(false);
    mInputFieldsEnabled = false;
}

void FrameCommentDialog::updateNavigation()
{
    ui->labFrameNumber->setText(QString::number(mEditor->currentFrame()));

    if (!mLayerManager->currentLayer()->keyExists(mEditor->currentFrame()))
    {
        disableInputFields();
        mInputFieldsEnabled = false;
    }
    else
    {
        enableInputFields();
        mInputFieldsEnabled = true;
        mKeyframe = mLayerManager->currentLayer()->getKeyFrameAt(mEditor->currentFrame());
        initCommentFields(mKeyframe);
    }

    ui->btnFirstFrame->setEnabled(true);
    ui->btnPreviousFrame->setEnabled(true);
    ui->btnNextFrame->setEnabled(true);
    ui->btnLastFrame->setEnabled(true);
    if (mEditor->currentFrame() == mLayerManager->currentLayer()->firstKeyFramePosition())
    {
         ui->btnFirstFrame->setEnabled(false);
         ui->btnPreviousFrame->setEnabled(false);
    }
    if (mEditor->currentFrame() == mLayerManager->currentLayer()->getMaxKeyFramePosition())
    {
         ui->btnLastFrame->setEnabled(false);
         ui->btnNextFrame->setEnabled(false);
    }
}
