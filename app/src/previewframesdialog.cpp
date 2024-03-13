#include "previewframesdialog.h"
#include "ui_previewframesdialog.h"

PreviewFramesDialog::PreviewFramesDialog(int maxFrame, int currentFrame) :
    QDialog(),
    ui(new Ui::PreviewFramesDialog)
{
    ui->setupUi(this);
    ui->frameSpinBox->setMinimum(1);
    ui->frameSpinBox->setMaximum(maxFrame);
    ui->frameSpinBox->setValue(currentFrame);
    setCurrentFrame(currentFrame);
    setMaxFrame(maxFrame);
    ui->labMaxNum->setText(QString::number(maxFrame));
    connect(ui->frameSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PreviewFramesDialog::frameChanged);
}

PreviewFramesDialog::~PreviewFramesDialog()
{
    delete ui;
}


void PreviewFramesDialog::frameChanged(int frame)
{
    setCurrentFrame(frame);
    ui->labFrame->setText(QString::number(frame));
}
