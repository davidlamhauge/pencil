#ifndef POSITIONTRANSFORMDIALOG_H
#define POSITIONTRANSFORMDIALOG_H

#include <QDialog>
#include <QRectF>
#include "editor.h"
#include "scribblearea.h"

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

private:

public slots:
    void movePatternChanged();
    void updateSelectionValues();
    void closeUi();

private:
    Ui::PositionTransformDialog *ui;
    Editor* mEditor = nullptr;
    ScribbleArea* mScribb = nullptr;
};

#endif // POSITIONTRANSFORMDIALOG_H
