#ifndef FRAMECOMMENTDIALOG_H
#define FRAMECOMMENTDIALOG_H

#include <QDialog>
#include "editor.h"

class LayerManager;
class KeyFrame;
class FrameCommentWidget;

namespace Ui {
class FrameCommentDialog;
}

class FrameCommentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FrameCommentDialog(QWidget *parent = nullptr);
    ~FrameCommentDialog();

    void setCore(Editor* editor);
    void initUI();
    void updateUI();

signals:
    void dialogueCommentChanged();
    void actionCommentChanged();
    void slugCommentChanged();

private:
    Ui::FrameCommentDialog *ui;

    // Navigation
    void scrubToFirstFrame();
    void scrubToPreviousFrame();
    void scrubToNextFrame();
    void scrubToLastFrame();

    // Update Dialogue, Action and Slug
    void updateDialogue();
    void updateAction();
    void updateSlug();

    // init Dialogue, Action and Slug
    void initCommentFields(KeyFrame* key);

    void enableInputFields();
    void disableInputFields();
    bool mInputFieldsEnabled = true;
    void updateNavigation();

    Editor* mEditor = nullptr;
    LayerManager* mLayerManager = nullptr;
    KeyFrame* mKeyframe = nullptr;
    FrameCommentWidget* mCommentWidget = nullptr;

};

#endif // FRAMECOMMENTDIALOG_H
