#ifndef PREVIEWFRAMESDIALOG_H
#define PREVIEWFRAMESDIALOG_H

#include <QDialog>

namespace Ui {
class previewFramesDialog;
}

class previewFramesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit previewFramesDialog(int maxFrame, int currentFrame);
    ~previewFramesDialog();

    void gotoFrame();
    void frameChanged(int frame);

    void setMaxFrame(int max) { mMaxFrame = max; }
    int  getMaxFrame() { return mMaxFrame; }

private:
    int mMinFrame = 1;
    int mMaxFrame = 100;
    Ui::previewFramesDialog *ui;
};

#endif // PREVIEWFRAMESDIALOG_H
