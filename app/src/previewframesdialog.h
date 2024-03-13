#ifndef PREVIEWFRAMESDIALOG_H
#define PREVIEWFRAMESDIALOG_H

#include <QDialog>

namespace Ui {
class PreviewFramesDialog;
}

class PreviewFramesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreviewFramesDialog(int maxFrame, int currentFrame);
    ~PreviewFramesDialog();

    void gotoFrame();
    void frameChanged(int frame);

    void setMaxFrame(int max) { mMaxFrame = max; }
    int  getMaxFrame() { return mMaxFrame; }
    void setCurrentFrame(int curr) { mCurrentFrame = curr; }
    int getCurrentFrame() { return mCurrentFrame; }

signals:
    void currentFrameChanged(int);

private:
    int mMinFrame = 1;
    int mMaxFrame = 100;
    int mCurrentFrame = 1;
    Ui::PreviewFramesDialog *ui;
};

#endif // PREVIEWFRAMESDIALOG_H
