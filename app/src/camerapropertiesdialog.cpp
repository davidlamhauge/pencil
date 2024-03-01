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

#include "camerapropertiesdialog.h"
#include "ui_camerapropertiesdialog.h"

CameraPropertiesDialog::CameraPropertiesDialog(const QString& name, int width, int height) :
    QDialog(),
    ui(new Ui::CameraPropertiesDialog)
{
    ui->setupUi(this);

    ui->nameBox->setText(name);
    ui->widthBox->setValue(width);
    ui->heightBox->setValue(height);
}

CameraPropertiesDialog::~CameraPropertiesDialog()
{
    delete ui;
}

QString CameraPropertiesDialog::getName()
{
    return ui->nameBox->text();
}

void CameraPropertiesDialog::setName(const QString& name)
{
    ui->nameBox->setText(name);
}

int CameraPropertiesDialog::getWidth()
{
    return ui->widthBox->value();
}

void CameraPropertiesDialog::setWidth(int width)
{
    ui->widthBox->setValue(width);
}

int CameraPropertiesDialog::getHeight()
{
    return ui->heightBox->value();
}

void CameraPropertiesDialog::setHeight(int height)
{
    ui->heightBox->setValue(height);
}

qreal CameraPropertiesDialog::getDistance()
{
    return ui->distanceSpinBox->value();
}

void CameraPropertiesDialog::setDistance(qreal dist)
{
    ui->distanceSpinBox->setValue(dist);
}

qreal CameraPropertiesDialog::getAperture()
{
    return ui->apertureBox->itemData(ui->apertureBox->currentIndex()).toDouble();
}

void CameraPropertiesDialog::setAperture(qreal aperture)
{
    ui->apertureBox->setCurrentText(QString::number(aperture));
}
