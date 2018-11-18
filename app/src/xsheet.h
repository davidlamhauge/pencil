#ifndef XSHEET_H
#define XSHEET_H

#include "basedockwidget.h"

#include <QDialog>
#include "layermanager.h"
#include "editor.h"
#include "qdebug.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMediaPlayer>

namespace Ui {
class Xsheet;
}

class Xsheet : public BaseDockWidget
{
    Q_OBJECT

    void initUI() override;
    void updateUI() override;

public:
    explicit Xsheet(QWidget *parent = nullptr);
    explicit Xsheet(Editor* editor, QWidget *parent = nullptr);
    ~Xsheet() override;
public slots:
    void newOpenScene();
    void updateXsheet();
    void showScrub(int frame);
    void updateScrub(int frame);
    void lengthChanged(int frames);

private slots:
    void loadAudio(QString fileName);
    void selectLayerFrame(const QModelIndex &current, const QModelIndex &previous);
    void addLayerFrame(int row, int column);
    void fillXsheet();
    void loadPapa();
    void erasePapa();
    void loadLipsync();
    void saveLipsync();
    void saveCsv();
    void addFrame();
    void removeFrame();

private:
    void initXsheet();
    void writePapa();
    bool layerNameExists(QString LayerName);
    int getLayerType(Layer* layer);
    void selectItem(int row, int column);
    QColor getLayerColor(int color);
    QStringList* mLayerNames;
    QStringList* mPapaLines;            // for filling DIAL column
    Ui::Xsheet *ui;
    Editor* mEditor = nullptr;
    QMediaPlayer* player = nullptr;
    int mAudioOffset;
    int mLayerCount;
    int mCurrentFrame;
    int mTimeLineLength;
    QTableWidget* mTableWidget;
    QTableWidgetItem* mTableItem;
};

#endif // XSHEET_H
