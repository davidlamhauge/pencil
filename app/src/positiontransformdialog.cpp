#include "positiontransformdialog.h"
#include "ui_positiontransformdialog.h"

PositionTransformDialog::PositionTransformDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PositionTransformDialog)
{
    ui->setupUi(this);
    connect(ui->btnCancel, &QPushButton::clicked, this, &PositionTransformDialog::closeUi);
    connect(mScribb, &ScribbleArea::selectionChanged, this, &PositionTransformDialog::updateSelectionValues);
}

PositionTransformDialog::~PositionTransformDialog()
{
    delete ui;
}

void PositionTransformDialog::initDialog(Editor *editor)
{
    mEditor = editor;
    mScribb = mEditor->getScribbleArea();
    ui->sbXNew->setValue(static_cast<int>(mScribb->getSelection().left()));
    ui->sbYNew->setValue(static_cast<int>(mScribb->getSelection().top()));
    qDebug() << mScribb->getSelection().left();
}

void PositionTransformDialog::updateSelectionValues(QRectF rectF)
{
    qDebug() << "Hej!";
    ui->sbXNew->setValue(static_cast<int>(rectF.left()));
    ui->sbYNew->setValue(static_cast<int>(rectF.top()));
}

void PositionTransformDialog::closeUi()
{
   close();
}
