#ifndef POSITIONTRANSFORMDIALOG_H
#define POSITIONTRANSFORMDIALOG_H

#include <QDialog>

namespace Ui {
class PositionTransformDialog;
}

class PositionTransformDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PositionTransformDialog(QWidget *parent = nullptr);
    ~PositionTransformDialog();

private:
    Ui::PositionTransformDialog *ui;
};

#endif // POSITIONTRANSFORMDIALOG_H
