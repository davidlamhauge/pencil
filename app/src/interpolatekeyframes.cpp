#include "interpolatekeyframes.h"
#include "ui_interpolatekeyframes.h"

InterpolateKeyframes::InterpolateKeyframes(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InterpolateKeyframes)
{
    ui->setupUi(this);
}

InterpolateKeyframes::~InterpolateKeyframes()
{
    delete ui;
}
