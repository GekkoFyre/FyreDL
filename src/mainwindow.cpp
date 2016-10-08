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
#include <boost/filesystem.hpp>
#include <QString>
#include <QInputDialog>
#include <QtWidgets/QFileDialog>
#include <QModelIndex>
#include <QStringList>
#include <QMessageBox>
#include <QFileDialog>
#include <QList>
#include <vector>

namespace fs = boost::filesystem;
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

    readFromHistoryFile();
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
 *         <http://doc.qt.io/qt-5/signalsandslots.html>
 * @param  url The URL of the file you wish to add.
 */
void MainWindow::addDownload()
{
    AddURL *add_url = new AddURL(this);
    QObject::connect(add_url, SIGNAL(finished(int)), this, SLOT(url_dialogIsFin(int)));
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
void MainWindow::readFromHistoryFile()
{
    std::vector<GekkoFyre::CmnRoutines::CurlDlInfo> dl_history;
    if (fs::exists(routines->findCfgFile(CFG_HISTORY_FILE)) &&
            fs::is_regular_file(routines->findCfgFile(CFG_HISTORY_FILE))) {
        dl_history = routines->readDownloadInfo(CFG_HISTORY_FILE);
    } else {
        return;
    }

    for (size_t i = 0; i < dl_history.size(); ++i) {
        if (!dl_history.at(i).file_loc.empty() || dl_history.at(i).ext_info.status_ok == true) {
            if (dl_history.at(i).ext_info.content_length > 0) {
                dlModel->insertRows(0, 1, QModelIndex());

                QModelIndex index = dlModel->index(0, 0, QModelIndex());
                dlModel->setData(index, routines->extractFilename(QString::fromStdString(dl_history.at(i).ext_info.effective_url)), Qt::DisplayRole);

                index = dlModel->index(0, 1, QModelIndex());
                dlModel->setData(index, QString::number(routines->bytesToKilobytes(dl_history.at(i).ext_info.content_length)), Qt::DisplayRole);

                index = dlModel->index(0, 2, QModelIndex());
                dlModel->setData(index, QString::number(0), Qt::DisplayRole);

                index = dlModel->index(0, 3, QModelIndex());
                dlModel->setData(index, QString::number(0), Qt::DisplayRole);

                index = dlModel->index(0, 4, QModelIndex());
                dlModel->setData(index, QString::number(0), Qt::DisplayRole);

                index = dlModel->index(0, 5, QModelIndex());
                dlModel->setData(index, QString::number(0), Qt::DisplayRole);

                index = dlModel->index(0, 6, QModelIndex());
                dlModel->setData(index, routines->convDlStat_toString(dl_history.at(i).dlStatus), Qt::DisplayRole);

                index = dlModel->index(0, 7, QModelIndex());
                dlModel->setData(index, QString::fromStdString(dl_history.at(i).file_loc), Qt::DisplayRole);
                return;
            } else {
                QMessageBox::information(this, tr("Problem!"), tr("The size of the download could not be "
                                                                  "determined. Please try again."),
                                         QMessageBox::Ok);
                return;
            }
        }
    }

    QMessageBox::warning(this, tr("Error!"), tr("An error was encountered within the internals of "
                                                "the application! Please restart the program."),
                         QMessageBox::Ok);
    return;
}

/**
 * @brief MainWindow::modifyHistoryFile
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10-06
 * @note   <http://stackoverflow.com/questions/4807709/how-can-i-change-the-value-of-the-elements-in-a-vector>
 *         <http://stackoverflow.com/questions/1453333/how-to-make-elements-of-vector-unique-remove-non-adjacent-duplicates>
 * @param xmlCfgFile
 */
void MainWindow::modifyHistoryFile()
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

/**
 * @brief MainWindow::url_dialogIsFin
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @note   <http://stackoverflow.com/questions/12470806/qdialog-exec-and-getting-result-value>
 * @param ret_code
 */
void MainWindow::url_dialogIsFin(const int &ret_code)
{
    if (ret_code == QDialog::Accepted) {
        readFromHistoryFile();
        return;
    }

    return;
}
