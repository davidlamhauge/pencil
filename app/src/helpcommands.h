/*

Pencil2D - Traditional Animation Software
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#ifndef HELPCOMMANDS_H
#define HELPCOMMANDS_H

#include <QObject>

class QWidget;

class HelpCommands : public QObject
{
    Q_OBJECT
public:
    explicit HelpCommands();
    virtual ~HelpCommands();

    // interface
    void interfaceIntro();

    // Tools

    // Palette
};

#endif // HELPCOMMANDS_H
