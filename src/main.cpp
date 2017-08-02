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
 * @file main.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-08
 * @brief This file is self-explanatory.
 */

#include "gui/mainwindow.hpp"
#include "default_var.hpp"
#include "singleton_proc.hpp"
#include <QApplication>

#ifdef Q_OS_LINUX
extern "C" {
#include <X11/Xlib.h>
};

#endif

#define BOOST_FILESYSTEM_NO_DEPRECATED

int main(int argc, char *argv[])
{
    try {
        SingletonProcess singleton(37563);
        if (!singleton()) {
            std::cerr << "Another FyreDL instance is already open!" << std::endl;
            return 1; // Exit with status code '1'
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    // https://github.com/notepadqq/notepadqq/issues/323
    #ifdef __linux__
    Display *d = XOpenDisplay(nullptr);
    std::unique_ptr<Screen> s = std::make_unique<Screen>();
    s.reset(DefaultScreenOfDisplay(d));
    int width = s->width;
    double ratio = ((double)width / FYREDL_DEFAULT_RESOLUTION_WIDTH);
    if (ratio > 1.1) {
        qputenv("QT_DEVICE_PIXEL_RATIO", QString::number(ratio).toLatin1());
    }

    #else
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
    #endif

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
