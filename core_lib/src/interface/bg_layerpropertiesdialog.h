#ifndef BG_LAYERPROPERTIESDIALOG_H
#define BG_LAYERPROPERTIESDIALOG_H

#include <QDialog>

namespace Ui {
class BG_LayerPropertiesDialog;
}

class BG_LayerPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BG_LayerPropertiesDialog(QWidget *parent = nullptr);
    BG_LayerPropertiesDialog(QString layerName, int startFrame, int endFrame,
                             int direction, int pixels);
    ~BG_LayerPropertiesDialog();

private:
    Ui::BG_LayerPropertiesDialog *ui;
};

#endif // BG_LAYERPROPERTIESDIALOG_H
