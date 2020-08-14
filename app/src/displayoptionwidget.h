/*

Pencil - Traditional Animation Software
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#ifndef DISPLAYOPTIONDOCKWIDGET_H
#define DISPLAYOPTIONDOCKWIDGET_H

#include "basedockwidget.h"

namespace Ui
{
    class DisplayOption;
}
class Editor;
class QToolButton;
class ViewManager;

class DisplayOptionWidget : public BaseDockWidget
{
    Q_OBJECT
public:
    explicit DisplayOptionWidget(QWidget* parent);
    virtual ~DisplayOptionWidget() override;

    void initUI() override;
    void updateUI() override;

protected:
    int getMinHeightForWidth(int width) override;

private slots:
    void toggleMirror(bool);
    void toggleMirrorV(bool);
    void toggleOverlayCenter(bool isOn);
    void toggleOverlayThirds(bool isOn);
    void toggleOverlayGoldenRatio(bool isOn);
    void toggleOverlaySafeAreas(bool isOn);

private:
    void makeConnections();
    Ui::DisplayOption* ui = nullptr;
};

#endif // DISPLAYOPTIONDOCKWIDGET_H
