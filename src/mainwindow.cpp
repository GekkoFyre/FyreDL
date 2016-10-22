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
#include "settings.hpp"
#include <boost/filesystem.hpp>
#include <QInputDialog>
#include <QModelIndex>
#include <QMessageBox>
#include <QFileDialog>
#include <QList>
#include <QSortFilterProxyModel>
#include <QItemSelectionModel>

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
    ui->downloadView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->downloadView->horizontalHeader()->setStretchLastSection(true); // http://stackoverflow.com/questions/16931569/qstandarditemmodel-inside-qtableview
    ui->downloadView->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(ui->downloadView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_downloadView_customContextMenuRequested(QPoint)));
    QObject::connect(routines, SIGNAL(sendXferStats(GekkoFyre::CmnRoutines::CurlDlStats)), this, SLOT(recvXferStats(GekkoFyre::CmnRoutines::CurlDlStats)));
    QObject::connect(routines, SIGNAL(sendXferPtr(GekkoFyre::CmnRoutines::CurlDlPtr)), this, SLOT(recvXferPtr(GekkoFyre::CmnRoutines::CurlDlPtr)));
    QObject::connect(this, SIGNAL(updateDlStats()), this, SLOT(manageDlStats()));

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
    QObject::connect(add_url, SIGNAL(sendDetails(std::string,double,int,double,int,int,GekkoFyre::DownloadStatus,std::string,std::string)), this, SLOT(sendDetails(std::string,double,int,double,int,int,GekkoFyre::DownloadStatus,std::string,std::string)));
    add_url->setAttribute(Qt::WA_DeleteOnClose, true);
    add_url->open();
    // delete add_url;
    return;
}

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
        try {
            dl_history = routines->readDownloadInfo(CFG_HISTORY_FILE);
        } catch (const std::exception &e) {
            QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
        }
    } else {
        return;
    }

    dlModel->removeRows(0, (int)dl_history.size(), QModelIndex());
    for (size_t i = 0; i < dl_history.size(); ++i) {
        if (!dl_history.at(i).file_loc.empty() || dl_history.at(i).ext_info.status_ok == true) {
            if (dl_history.at(i).ext_info.content_length > 0) {
                insertNewRow(dl_history.at(i).ext_info.effective_url,
                             dl_history.at(i).ext_info.content_length, 0, 0, 0, 0,
                             dl_history.at(i).dlStatus, dl_history.at(i).ext_info.effective_url,
                             dl_history.at(i).file_loc);
            } else {
                QMessageBox::information(this, tr("Problem!"), tr("The size of the download could not be "
                                                                  "determined. Please try again."),
                                         QMessageBox::Ok);
                return;
            }
        }
    }

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
{
    QList<std::vector<QString>> list = dlModel->getList();
}

void MainWindow::insertNewRow(const std::string &fileName, const double &fileSize, const int &downloaded,
                              const double &progress, const int &upSpeed, const int &downSpeed,
                              const GekkoFyre::DownloadStatus &status, const std::string &url,
                              const std::string &destination)
{
    dlModel->insertRows(0, 1, QModelIndex());

    QModelIndex index = dlModel->index(0, MN_FILENAME_COL, QModelIndex());
    dlModel->setData(index, routines->extractFilename(QString::fromStdString(fileName)), Qt::DisplayRole);

    index = dlModel->index(0, MN_FILESIZE_COL, QModelIndex());
    dlModel->setData(index, routines->bytesToKilobytes(fileSize), Qt::DisplayRole);

    index = dlModel->index(0, MN_DOWNLOADED_COL, QModelIndex());
    dlModel->setData(index, QString::number(downloaded), Qt::DisplayRole);

    index = dlModel->index(0, MN_PROGRESS_COL, QModelIndex());
    dlModel->setData(index, QString::number(progress), Qt::DisplayRole);

    index = dlModel->index(0, MN_UPSPEED_COL, QModelIndex());
    dlModel->setData(index, QString::number(upSpeed), Qt::DisplayRole);

    index = dlModel->index(0, MN_DOWNSPEED_COL, QModelIndex());
    dlModel->setData(index, QString::number(downSpeed), Qt::DisplayRole);

    index = dlModel->index(0, MN_STATUS_COL, QModelIndex());
    dlModel->setData(index, routines->convDlStat_toString(status), Qt::DisplayRole);

    index = dlModel->index(0, MN_DESTINATION_COL, QModelIndex());
    dlModel->setData(index, QString::fromStdString(url), Qt::DisplayRole);

    index = dlModel->index(0, MN_DESTINATION_COL, QModelIndex());
    dlModel->setData(index, QString::fromStdString(destination), Qt::DisplayRole);

    return;
}

/**
 * @brief MainWindow::removeSelRows
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10-12
 * @note   <http://doc.qt.io/qt-5/qtwidgets-itemviews-addressbook-example.html>
 */
void MainWindow::removeSelRows()
{
    // Be sure not to delete items from within slots connected to signals that have the item (or its
    // index) as their parameter.
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedIndexes();
    int countRow = indexes.count();

    bool flagDif = false;
    for (int i = countRow; i > 1; --i) {
        if ((indexes.at(i - 1).row() - 1) != indexes.at(i - 2).row()) {
            flagDif = true;
        }
    }

    if (!flagDif) {
        try {
            routines->delDownloadItem(ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString());
        } catch (const std::exception &e) {
            QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
            return;
        }

        dlModel->removeRows(indexes.at(0).row(), countRow, QModelIndex());
    } else {
        for (int i = countRow; i > 0; --i) {
            dlModel->removeRow(indexes.at(i - 1).row(), QModelIndex());
        }
    }
}

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
{
    QCoreApplication::quit(); // Exit with status code '0'
    return;
}

void MainWindow::on_action_About_triggered()
{
    QMessageBox::information(this, tr("Problem!"), tr("This function is not yet implemented."),
                             QMessageBox::Ok);
    return;
}

void MainWindow::on_action_Documentation_triggered()
{
    QMessageBox::information(this, tr("Problem!"), tr("This function is not yet implemented."),
                             QMessageBox::Ok);
    return;
}

void MainWindow::on_addurlToolBtn_clicked()
{
    addDownload();
    return;
}

void MainWindow::on_addfileToolBtn_clicked()
{
    return;
}

void MainWindow::on_printToolBtn_clicked()
{
    return;
}

void MainWindow::on_dlstartToolBtn_clicked()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedIndexes();
    int countRow = indexes.count();

    bool flagDif = false;
    for (int i = countRow; i > 1; --i) {
        if ((indexes.at(i - 1).row() - 1) != indexes.at(i - 2).row()) {
            flagDif = true;
        }
    }

    if (!flagDif) {
        try {
            const QString url = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString();
            if (routines->convDlStat_StringToEnum(ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_STATUS_COL)).toString()) != GekkoFyre::DownloadStatus::Downloading) {
                GekkoFyre::CmnRoutines::CurlInfo verify = routines->verifyFileExists(ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString());
                if (verify.response_code == 200) {
                    routines->modifyDlState(url, GekkoFyre::DownloadStatus::Downloading);
                    QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
                    dlModel->setData(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Downloading), Qt::DisplayRole);
                    routines->fileStream(url, ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString());
                }
            }
        } catch (const std::exception &e) {
            QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
            return;
        }
    }
}

void MainWindow::on_dlpauseToolBtn_clicked()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedIndexes();
    int countRow = indexes.count();

    bool flagDif = false;
    for (int i = countRow; i > 1; --i) {
        if ((indexes.at(i - 1).row() - 1) != indexes.at(i - 2).row()) {
            flagDif = true;
        }
    }

    if (!flagDif) {
        try {
            if (routines->convDlStat_StringToEnum(ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_STATUS_COL)).toString()) != GekkoFyre::DownloadStatus::Paused) {
                routines->modifyDlState(ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString(), GekkoFyre::DownloadStatus::Paused);
                QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
                dlModel->setData(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Paused), Qt::DisplayRole);
            }
        } catch (const std::exception &e) {
            QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
            return;
        }
    }
}

void MainWindow::on_dlstopToolBtn_clicked()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedIndexes();
    int countRow = indexes.count();

    bool flagDif = false;
    for (int i = countRow; i > 1; --i) {
        if ((indexes.at(i - 1).row() - 1) != indexes.at(i - 2).row()) {
            flagDif = true;
        }
    }

    if (!flagDif) {
        try {
            if (routines->convDlStat_StringToEnum(ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_STATUS_COL)).toString()) != GekkoFyre::DownloadStatus::Stopped) {
                routines->modifyDlState(ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString(), GekkoFyre::DownloadStatus::Stopped);
                QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
                dlModel->setData(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Stopped), Qt::DisplayRole);
            }
        } catch (const std::exception &e) {
            QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
            return;
        }
    }
}

void MainWindow::on_removeToolBtn_clicked()
{
    removeSelRows();
}

void MainWindow::on_clearhistoryToolBtn_clicked()
{
    return;
}

void MainWindow::on_settingsToolBtn_clicked()
{
    Settings *settingsUi = new Settings(this);
    settingsUi->setAttribute(Qt::WA_DeleteOnClose, true);
    settingsUi->open();
}

void MainWindow::sendDetails(const std::string &fileName, const double &fileSize, const int &downloaded,
                             const double &progress, const int &upSpeed, const int &downSpeed,
                             const GekkoFyre::DownloadStatus &status, const std::string &url,
                             const std::string &destination)
{
    QList<std::vector<QString>> list = dlModel->getList();
    std::vector<QString> fileNameVector;
    fileNameVector.push_back(QString::fromStdString(fileName));
    if (!list.contains(fileNameVector) && !list.contains(std::vector<QString>())) {
        insertNewRow(fileName, fileSize, downloaded, progress, upSpeed, downSpeed, status, url, destination);
        return;
    } else {
        QMessageBox::information(this, tr("Duplicate entry"), tr("A duplicate of, \"%1\", was entered!")
                                 .arg(QString::fromStdString(fileName)), QMessageBox::Ok);
        return;
    }

    return;
}

/**
 * @brief MainWindow::recvXferStats receives the statistics regarding a download.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10-23
 * @note   <http://stackoverflow.com/questions/9086372/how-to-compare-pointers>
 * @param info is the struct related to the download info.
 */
void MainWindow::recvXferStats(const GekkoFyre::CmnRoutines::CurlDlStats &info)
{
    GekkoFyre::CmnRoutines::CurlDlStats temp_info;
    temp_info.dlnow = info.dlnow;
    temp_info.dltotal = info.dltotal;
    temp_info.upnow = info.upnow;
    temp_info.uptotal = info.uptotal;
    temp_info.cur_time = info.cur_time;
    temp_info.easy = info.easy;

    bool alreadyExists = false;
    for (size_t i = 0; i < dl_stat.size(); ++i) {
        if (dl_stat.at(i).easy == temp_info.easy) { // Compare memory address for equality
            alreadyExists = true;
        }
    }

    if (!alreadyExists) {
        dl_stat.push_back(temp_info);
    }

    return;
}

void MainWindow::recvXferPtr(const GekkoFyre::CmnRoutines::CurlDlPtr &ptr_info)
{
    for (size_t i = 0; i < dl_stat.size(); ++i) {
        if (ptr_info.ptr == dl_stat.at(i).easy) {
            dl_stat.at(i).url = ptr_info.url;
            emit updateDlStats();
            return;
        }
    }

    return;
}

/**
 * @brief MainWindow::manageDlStats
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10-23
 * @note   <http://www.qtcentre.org/threads/18082-Hot-to-get-a-QModelIndexList-from-a-model>
 */
void MainWindow::manageDlStats()
{
    for (int i = 0; i < dlModel->getList().size(); ++i) {
        QModelIndex find_index = dlModel->index(i, 8);
        for (size_t j = 0; j < dl_stat.size(); ++j) {
            if (ui->downloadView->model()->data(find_index).toString().toStdString() == dl_stat.at(j).url) {
                try {
                    dlModel->setData(dlModel->index(i, MN_DOWNSPEED_COL), QString::number(dl_stat.at(j).dlnow), Qt::DisplayRole); // Download speed
                    dlModel->setData(dlModel->index(i, MN_DOWNLOADED_COL), QString::number(dl_stat.at(j).dltotal), Qt::DisplayRole); // Downloaded total

                    dlModel->setData(dlModel->index(i, MN_UPSPEED_COL), QString::number(dl_stat.at(j).upnow), Qt::DisplayRole); // Upload speed
                } catch (const std::exception &e) {
                    QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                    return;
                }
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::on_downloadView_customContextMenuRequested
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10-15
 * @note   <https://forum.qt.io/topic/31233/how-to-create-a-custom-context-menu-for-qtableview/3>
 * @param pos
 */
void MainWindow::on_downloadView_customContextMenuRequested(const QPoint &pos)
{
    ui->downloadView->indexAt(pos);

    QMenu *menu = new QMenu(this);
    menu->addAction(new QAction(tr("Edit"), this));
    menu->addAction(new QAction(tr("Delete"), this));

    menu->popup(ui->downloadView->viewport()->mapToGlobal(pos));
}
