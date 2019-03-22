#include "positiontransformdialog.h"
#include "ui_positiontransformdialog.h"

PositionTransformDialog::PositionTransformDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PositionTransformDialog)
{
    ui->setupUi(this);
    ui->sbXMove->setEnabled(false);
    ui->sbYMove->setEnabled(false);
    connect(ui->rbEvenlyPosition, &QRadioButton::released, this, &PositionTransformDialog::movePatternChanged);
    connect(ui->rbUnevenlyPosition, &QRadioButton::released, this, &PositionTransformDialog::movePatternChanged);
    connect(ui->btnCancel, &QPushButton::clicked, this, &PositionTransformDialog::closeUi);
}

PositionTransformDialog::~PositionTransformDialog()
{
    delete ui;
}

void PositionTransformDialog::initDialog(Editor *editor)
{
    mEditor = editor;
    mScribb = mEditor->getScribbleArea();
    connect(mScribb, &ScribbleArea::selectionMoved, this, &PositionTransformDialog::updateSelectionValues);
    ui->labStartX->setText(QString::number(mScribb->getSelection().left()));
    ui->labStartY->setText(QString::number(mScribb->getSelection().top()));
    ui->labDiffX->setText("0");
    ui->labDiffY->setText("0");
    ui->labNewX->setText(QString::number(mScribb->getSelection().left()));
    ui->labNewY->setText(QString::number(mScribb->getSelection().top()));
}

void PositionTransformDialog::movePatternChanged()
{
    if (ui->rbEvenlyPosition->isChecked())
    {
        ui->sbXMove->setValue(0);
        ui->sbXMove->setEnabled(false);
        ui->sbYMove->setValue(0);
        ui->sbYMove->setEnabled(false);
    }
    else
    {
        ui->sbXMove->setEnabled(true);
        ui->sbYMove->setEnabled(true);
    }
}

void PositionTransformDialog::updateSelectionValues()
{
    QRectF org = mScribb->getSelection();
    QRectF rect = mScribb->myTransformedSelection;
    ui->labNewX->setText(QString::number(rect.left()));
    ui->labNewY->setText(QString::number(rect.top()));
    ui->labDiffX->setText(QString::number(rect.left() - org.left()));
    ui->labDiffY->setText(QString::number(rect.top() - org.top()));
}

void PositionTransformDialog::closeUi()
{
   close();
}
