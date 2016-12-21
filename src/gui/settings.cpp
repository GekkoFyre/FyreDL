/**
 **  ______             ______ _
 **  |  ___|            |  _  \ |
 **  | |_ _   _ _ __ ___| | | | |
 **  |  _| | | | '__/ _ \ | | | |
 **  | | | |_| | | |  __/ |/ /| |____
 **  \_|  \__, |_|  \___|___/ \_____/
 **        __/ |
 **       |___/
 **
 **   Thank you for using "FyreDL" for your download management needs!
 **   Copyright (C) 2016. GekkoFyre.
 **
 **
 **   FyreDL is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   FyreDL is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with FyreDL.  If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **   The latest source code updates can be obtained from [ 1 ] below at your
 **   discretion. A web-browser or the 'git' application may be required.
 **
 **   [ 1 ] - https://github.com/GekkoFyre/FyreDL
 **
 ********************************************************************************/

/**
 * @file settings.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-10
 * @brief The code and functions behind the, 'settings.ui', designer file.
 */

#include "settings.hpp"
#include "ui_settings.h"
#include <QFont>

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);

    // http://stackoverflow.com/questions/9701983/qt-how-to-create-a-setting-window-like-in-gtk
    // http://www.qtcentre.org/threads/25823-Font-size-increase-in-QtableWidget
    // http://stackoverflow.com/questions/19434391/in-qt-how-to-resize-icons-in-a-table
    QFont font;
    QSize size;
    font.setPointSize(14);
    font.setFamily("Arial");
    size.setHeight(48);
    size.setWidth(48);
    const int rowCount = ui->settings_list_widget->count();
    ui->settings_list_widget->setIconSize(size);
    for (int i = 0; i < rowCount; ++i) {
        QListWidgetItem *selectedItem = ui->settings_list_widget->item(i);
        selectedItem->setFont(font);
    }
}

Settings::~Settings()
{
    delete ui;
}

void Settings::on_buttonBox_accepted()
{
    // 'Save' has been selected
}

void Settings::on_buttonBox_rejected()
{
    // 'Cancel' has been selected
}
