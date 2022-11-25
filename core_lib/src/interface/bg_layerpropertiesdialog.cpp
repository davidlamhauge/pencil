#include "bg_layerpropertiesdialog.h"
#include "ui_bg_layerpropertiesdialog.h"

BG_LayerPropertiesDialog::BG_LayerPropertiesDialog(QString layerName, int startFrame, int endFrame, int direction, int pixels) :
    QDialog(),
    ui(new Ui::BG_LayerPropertiesDialog)
{
    ui->setupUi(this);
    ui->leLayerName->setText(layerName);
    ui->spinBoxStartFrame->setValue(startFrame);
    ui->spinBoxEndFrame->setValue(endFrame);
    ui->comboBoxDirection->setCurrentIndex(direction);
    ui->spinBoxPixels->setValue(pixels);
}

BG_LayerPropertiesDialog::~BG_LayerPropertiesDialog()
{
    delete ui;
}
