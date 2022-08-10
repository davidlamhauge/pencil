#ifndef LAYERPROPERTIESDIALOG_H
#define LAYERPROPERTIESDIALOG_H

#include <QDialog>

namespace Ui {
class LayerPropertiesDialog;
}

class LayerPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LayerPropertiesDialog(const QString& name, qreal distance);
    ~LayerPropertiesDialog();

    QString getName();
    qreal getDistance();

private:
    Ui::LayerPropertiesDialog *ui;

};

#endif // LAYERPROPERTIESDIALOG_H
