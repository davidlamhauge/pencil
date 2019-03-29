#ifndef INTERPOLATEKEYFRAMES_H
#define INTERPOLATEKEYFRAMES_H

#include <QDialog>

namespace Ui {
class InterpolateKeyframes;
}

class InterpolateKeyframes : public QDialog
{
    Q_OBJECT

public:
    explicit InterpolateKeyframes(QWidget *parent = nullptr);
    ~InterpolateKeyframes();

private:
    Ui::InterpolateKeyframes *ui;
};

#endif // INTERPOLATEKEYFRAMES_H
