#include "positiontransformdialog.h"
#include "ui_positiontransformdialog.h"

PositionTransformDialog::PositionTransformDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PositionTransformDialog)
{
    ui->setupUi(this);
}

PositionTransformDialog::~PositionTransformDialog()
{
    delete ui;
}
