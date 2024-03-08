#include "previewframesdialog.h"
#include "ui_previewframesdialog.h"

previewFramesDialog::previewFramesDialog(int maxFrame, int currentFrame) :
    QDialog(),
    ui(new Ui::previewFramesDialog)
{
    ui->setupUi(this);
    ui->frameSpinBox->setMinimum(1);
    ui->frameSpinBox->setMaximum(maxFrame);
    ui->frameSpinBox->setValue(currentFrame);
    connect(ui->frameSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &previewFramesDialog::frameChanged);

}

previewFramesDialog::~previewFramesDialog()
{
    delete ui;
}


void previewFramesDialog::frameChanged(int frame)
{

}
