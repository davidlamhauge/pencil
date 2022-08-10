#include "layerpropertiesdialog.h"
#include "ui_layerpropertiesdialog.h"

LayerPropertiesDialog::LayerPropertiesDialog(const QString &name, qreal distance) :
    QDialog(),
    ui(new Ui::LayerPropertiesDialog)
{
    ui->setupUi(this);

    ui->leName->setText(name);
    ui->sbDistance->setValue(distance);
}

LayerPropertiesDialog::~LayerPropertiesDialog()
{
    delete ui;
}

QString LayerPropertiesDialog::getName()
{
    return ui->leName->text();
}

qreal LayerPropertiesDialog::getDistance()
{
    return ui->sbDistance->value();
}
