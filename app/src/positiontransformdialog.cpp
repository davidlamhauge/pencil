#include "positiontransformdialog.h"
#include "ui_positiontransformdialog.h"

#include <QFile>
#include <QStandardPaths>
#include <QDir>

PositionTransformDialog::PositionTransformDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PositionTransformDialog)
{
    ui->setupUi(this);
    ui->sbXMove->setEnabled(false);
    ui->sbYMove->setEnabled(false);
    connect(ui->rbEvenlyPosition, &QRadioButton::released, this, &PositionTransformDialog::movePatternChanged);
    connect(ui->rbUnevenlyPosition, &QRadioButton::released, this, &PositionTransformDialog::movePatternChanged);
    connect(ui->btnReset, &QPushButton::clicked, this, &PositionTransformDialog::resetPositions);
    connect(ui->btnCancel, &QPushButton::clicked, this, &PositionTransformDialog::closeUi);
}

PositionTransformDialog::~PositionTransformDialog()
{
    delete ui;
}

void PositionTransformDialog::initDialog(Editor *editor)
{
    mEditor = editor;
    mLayerMgr = mEditor->layers();
    mScribb = mEditor->getScribbleArea();
    connect(mScribb, &ScribbleArea::selectionMoved, this, &PositionTransformDialog::updateSelectionValues);
    ui->labStartX->setText(QString::number(mScribb->getSelection().left()));
    ui->labStartY->setText(QString::number(mScribb->getSelection().top()));
    ui->labDiffX->setText("0");
    ui->labDiffY->setText("0");
    ui->labNewX->setText(QString::number(mScribb->getSelection().left()));
    ui->labNewY->setText(QString::number(mScribb->getSelection().top()));
}

void PositionTransformDialog::initTempLayer()
{
    if (mLayerMgr->currentLayer()->type() == Layer::BITMAP)
    {
        tmpLayer = static_cast<LayerBitmap*>(mLayerMgr->createBitmapLayer("TMP"));
    }
    if (mLayerMgr->currentLayer()->type() == Layer::VECTOR)
    {
        tmpLayer = static_cast<LayerVector*>(mLayerMgr->createVectorLayer("TMP"));
    }
    if (tmpLayer == nullptr) { closeUi(); }
    for (int i = mLayerMgr->currentLayer()->firstKeyFramePosition(); i <= mLayerMgr->currentLayer()->getMaxKeyFramePosition(); i++)
    {

    }
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
    mOrg = mScribb->getSelection();
    setOrg(mOrg);
    QRectF rect = mScribb->myTransformedSelection;
    ui->labNewX->setText(QString::number(rect.left()));
    ui->labNewY->setText(QString::number(rect.top()));
    ui->labDiffX->setText(QString::number(rect.left() - mOrg.left()));
    ui->labDiffY->setText(QString::number(rect.top() - mOrg.top()));
}

void PositionTransformDialog::resetPositions()
{
    mScribb->myTempTransformedSelection = mOrg;
    qDebug() << "myTemp: " << mScribb->myTempTransformedSelection;

    mScribb->setSelection(mOrg);
    mScribb->applySelectionChanges();
    mScribb->deselectAll();
}

void PositionTransformDialog::closeUi()
{
   close();
}
