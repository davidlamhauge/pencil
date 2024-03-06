#include "layerpropertiesdialog.h"
#include "ui_layerpropertiesdialog.h"

#include <QPushButton>
#include <QMessageBox>

layerPropertiesDialog::layerPropertiesDialog(QString name, qreal distance) :
    QDialog(),
    ui(new Ui::layerPropertiesDialog)
{
    ui->setupUi(this);
    ui->lineEditName->setText(name);
    ui->doubleSpinBoxDistance->setValue(distance);
    connect(ui->btnInstructions, &QPushButton::clicked, this, &layerPropertiesDialog::showInstructions);
}

layerPropertiesDialog::~layerPropertiesDialog()
{
    delete ui;
}

void layerPropertiesDialog::showInstructions()
{
    QMessageBox mBox;
}

QString layerPropertiesDialog::updateName()
{
    return ui->lineEditName->text();
}

qreal layerPropertiesDialog::updateDistance()
{
    return ui->doubleSpinBoxDistance->value();
}

