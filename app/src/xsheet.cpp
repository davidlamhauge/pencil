/*

Pencil - Traditional Animation Software
Copyright (C) 2012-2018 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include "xsheet.h"
#include "ui_xsheet.h"
#include <QSettings>
#include <QStringList>
#include <QMessageBox>
#include "pencildef.h"
#include "layer.h"

Xsheet::Xsheet(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Xsheet)
{
    ui->setupUi(this);
    sl = new QStringList;
    mTableWidget = ui->tableXsheet;
    connect(mTableWidget, SIGNAL(cellClicked(int, int)), this, SLOT(selectLayerFrame(int, int)));
}

Xsheet::~Xsheet()
{
    delete ui;
}

void Xsheet::updateUi(LayerManager &lMgr, Editor *&editor)
{
    mLayerMgr = &lMgr;
    mEditor = editor;
    QSettings settings(PENCIL2D, PENCIL2D);
    mTimeLineLength = settings.value(SETTING_TIMELINE_SIZE,240).toInt();
    initXsheet();
    fillXsheet();
}

void Xsheet::selectLayerFrame(int row, int column)
{
    if (column > 0 && column <= mLayerCount)
    {
        mTableItem = new QTableWidgetItem();
        mTableItem = mTableWidget->item(0, column);
        Layer* layer = mLayerMgr->findLayerByName(mTableItem->text());
        if (layer == nullptr) { return; }
        mLayerMgr->setCurrentLayer(layer);
        mEditor->scrubTo(row);
    }
    initXsheet();
    fillXsheet();
}

void Xsheet::fillXsheet()
{
    // fill Xsheet
    for (int i = 1; i <= mTimeLineLength; i++)
    {
        mTableWidget->setRowHeight(i,16);
        mTableItem = new QTableWidgetItem(QString::number(i));
        mTableWidget->setItem(i, 0, mTableItem);
        for (int j = 1; j <= mLayerCount; j++)
        {
            if (mLayerMgr->findLayerByName(mTableWidget->item(0,j)->text())->keyExists(i))
            {
                int type = getLayerType(mLayerMgr->findLayerByName(mTableWidget->item(0,j)->text()));
                mTableItem = new QTableWidgetItem(QString::number(i));
                mTableItem->setBackgroundColor(getLayerColor(type));
                mTableWidget->setItem(i, j, mTableItem);
            }
/*
            else
            {
                mTableWidget->setItem(i, j, new QTableWidgetItem(" "));
            }
            */
        }
    }
}

void Xsheet::initXsheet()
{
    // clear backgroundcolors
    for (int i = 1; i < sl->size(); i++)
        for (int j = 1; j < mTimeLineLength; j++)
        {
            mTableItem = new QTableWidgetItem("");
            mTableItem->setBackgroundColor(Qt::white);
            mTableWidget->setItem(i, j, mTableItem);
        }
    mLayerCount = 0;
    sl->clear();
    for (int i = 0; i < mLayerMgr->count(); i++)
    {   // count Bitmap and Vector layers (duplicate names NOT supported)
        if (mLayerMgr->getLayer(i)->type() == 1 || mLayerMgr->getLayer(i)->type() == 2)
        {
            if (!sl->contains(mLayerMgr->getLayer(i)->name()))
            {
                mLayerCount++;
                sl->append(mLayerMgr->getLayer(i)->name());
            }
            else
            {
                int ret = QMessageBox::critical(new QWidget,
                               tr("Layer name duplicate!"),
                               tr("Identical layer names '%1' not supported in Xsheet").arg(mLayerMgr->getLayer(i)->name()),
                               QMessageBox::Ok);
                Q_UNUSED(ret);
            }
        }
    }
    this->setMinimumWidth(mLayerCount * 50 + 130);
    mTableWidget->setRowCount(mTimeLineLength + 1);
    mTableWidget->setColumnCount(mLayerCount + 2);
    // set column width for layers
    for (int i = 0; i < mTableWidget->columnCount(); i++)
    {
        mTableWidget->setColumnWidth(i, 50);
    }
    // set headers of Xsheet
    mTableWidget->setRowHeight(0,16);
    mTableItem = new QTableWidgetItem("#");
    mTableItem->setBackgroundColor(QColor(Qt::lightGray));
    mTableWidget->setItem(0, 0, mTableItem);
    for (int i = 0; i < sl->size(); i++)
    {
        int type = getLayerType(mLayerMgr->findLayerByName(sl->at(i)));
        mTableItem = new QTableWidgetItem(sl->at(i));
        mTableItem->setBackgroundColor(getLayerColor(type));
        mTableWidget->setItem(0, i + 1, mTableItem);
    }
    mTableItem = new QTableWidgetItem("DIAL");
    mTableItem->setBackgroundColor(QColor(Qt::lightGray));
    mTableWidget->setItem(0, sl->size() + 1, mTableItem);
}

int Xsheet::getLayerType(Layer *layer)
{
    return layer->type();
}

QColor Xsheet::getLayerColor(int color)
{
    if (color == 1) { return QColor(151, 176, 244); }
    else            { return QColor(150, 242, 150); }
}
