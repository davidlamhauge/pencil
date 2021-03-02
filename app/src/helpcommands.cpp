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

#include "helpcommands.h"
#include <QDesktopServices>
#include <QUrl>

HelpCommands::HelpCommands()
{

}

HelpCommands::~HelpCommands()
{
}

void HelpCommands::interfaceIntro()
{
    QString url = "https://www.youtube.com/watch?v=8AzdWEDPBG0";
    QDesktopServices::openUrl(QUrl(url));
}
