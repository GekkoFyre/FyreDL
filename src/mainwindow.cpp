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
 * @file cmnroutines.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief The C++ code behind the, 'mainwindow.ui', designer file.
 */

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "addurl.hpp"
#include <QString>
#include <QInputDialog>
#include <QtWidgets/QFileDialog>
#include <QModelIndex>
#include <QStringList>
#include <QMessageBox>
#include <QFileDialog>
#include <QList>
#include <vector>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QMessageBox::warning(this, tr("FyreDL"), tr("FyreDL is currently under intense development at this "
                                                "time! Please only use at your own risk."),
                         QMessageBox::Ok);

    routines = new GekkoFyre::CmnRoutines();

    dlModel = new downloadModel(this);
    ui->downloadView->setModel(dlModel);
    ui->downloadView->horizontalHeader()->setStretchLastSection(true); // http://stackoverflow.com/questions/16931569/qstandarditemmodel-inside-qtableview
    // QModelIndex downloadIndex = downloadModel->index(QModelIndex());
}

MainWindow::~MainWindow()
{
    delete routines;
    delete ui;
}

/**
 * @brief MainWindow::addDownload adds a URL and its properties/information to the 'downloadView' TableView
 * widget.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @note   <http://doc.qt.io/qt-5/qtwidgets-itemviews-addressbook-example.html#addresswidget-class-implementation>
 *         <https://en.wikipedia.org/wiki/List_of_HTTP_status_codes#2xx_Success>
 *         <http://mirror.internode.on.net/pub/test/>
 * @param  url The URL of the file you wish to add.
 */
void MainWindow::addDownload()
{
    AddURL *add_url = new AddURL(this);
    add_url->setAttribute(Qt::WA_DeleteOnClose, true);
    add_url->open();
    // delete add_url;
    return;
}

/**
 * @brief MainWindow::removeDownload
 * @author    Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param url
 */
void MainWindow::removeDownload(const QString &url)
{}

/**
 * @brief MainWindow::readFromHistoryFile
 * @author         Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param fileName
 */
void MainWindow::readFromHistoryFile(const QString &fileName)
{}

/**
 * @brief MainWindow::writeToHistoryFile
 * @author         Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param fileName
 */
void MainWindow::writeToHistoryFile(const QString &fileName)
{}

void MainWindow::on_action_Open_a_File_triggered()
{
    addDownload();
    return;
}

void MainWindow::on_actionEnter_URL_triggered()
{
    addDownload();
    return;
}

void MainWindow::on_actionE_xit_triggered()
{}

void MainWindow::on_action_About_triggered()
{}

void MainWindow::on_action_Documentation_triggered()
{}

void MainWindow::on_addurlToolBtn_clicked()
{
    addDownload();
    return;
}

void MainWindow::on_addfileToolBtn_clicked()
{
    addDownload();
    return;
}

void MainWindow::on_printToolBtn_clicked()
{}

void MainWindow::on_dlstartToolBtn_clicked()
{}

void MainWindow::on_dlpauseToolBtn_clicked()
{}

void MainWindow::on_dlstopToolBtn_clicked()
{}

void MainWindow::on_removeToolBtn_clicked()
{}

void MainWindow::on_clearhistoryToolBtn_clicked()
{}

void MainWindow::on_settingsToolBtn_clicked()
{}
