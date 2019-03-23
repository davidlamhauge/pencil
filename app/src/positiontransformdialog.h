#ifndef POSITIONTRANSFORMDIALOG_H
#define POSITIONTRANSFORMDIALOG_H

#include <QDialog>
#include <QRectF>
#include "editor.h"
#include "scribblearea.h"
#include "layermanager.h"
#include "layer.h"
#include "layerbitmap.h"
#include "layervector.h"

namespace Ui {
class PositionTransformDialog;
}

class PositionTransformDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PositionTransformDialog(QWidget *parent = nullptr);
    ~PositionTransformDialog();

    void initDialog(Editor* editor);
    void initTempLayer();
    QRectF getOrg() { return mOrg; }
    void setOrg(QRectF rectF) { mOrg = rectF; }

private:

public slots:
    void movePatternChanged();
    void updateSelectionValues();
    void resetPositions();
    void closeUi();

private:
    Ui::PositionTransformDialog *ui;
    Editor* mEditor = nullptr;
    ScribbleArea* mScribb = nullptr;

    // Temporary layer
    Layer* tmpLayer = nullptr;
    LayerBitmap* mTmpBitmap = nullptr;
    LayerVector* mTmpVector = nullptr;
    LayerManager* mLayerMgr = nullptr;
    QString mTmpLayerName = "";

    QRectF mOrg;
};

#endif // POSITIONTRANSFORMDIALOG_H
