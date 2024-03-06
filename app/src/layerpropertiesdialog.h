#ifndef LAYERPROPERTIESDIALOG_H
#define LAYERPROPERTIESDIALOG_H

#include <QDialog>

namespace Ui {
class layerPropertiesDialog;
}

class layerPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit layerPropertiesDialog( QString name, qreal distance);
    ~layerPropertiesDialog();

    void showInstructions();
    QString updateName();
    qreal updateDistance();

private:
    Ui::layerPropertiesDialog *ui;
};

#endif // LAYERPROPERTIESDIALOG_H
