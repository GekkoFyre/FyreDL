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
 **   Copyright (C) 2016-2017. GekkoFyre.
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
 * @file mainwindow.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief The code and functions behind the, 'mainwindow.ui', designer file.
 */

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "settings.hpp"
#include "./../curl_easy.hpp"
#include "about.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <boost/system/error_code.hpp>
#include <cmath>
#include <fstream>
#include <iostream>
#include <qmetatype.h>
#include <QInputDialog>
#include <QModelIndex>
#include <QMessageBox>
#include <QFileDialog>
#include <QList>
#include <QSortFilterProxyModel>
#include <QItemSelectionModel>
#include <QMutex>
#include <QShortcut>
#include <QUrl>
#include <QDir>
#include <QDateTime>
#include <QDate>
#include <QHash>
#include <QStandardItemModel>

namespace sys = boost::system;
namespace fs = boost::filesystem;
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    routines = std::make_shared<GekkoFyre::CmnRoutines>(this);

    QMessageBox::warning(this, tr("FyreDL"), tr("FyreDL is currently under intense development at this "
                                                "time! Please only use at your own risk."),
                         QMessageBox::Ok);

    curl_multi = new GekkoFyre::CurlMulti();
    gk_torrent_client = new GekkoFyre::GkTorrentClient(this);

    // http://wiki.qt.io/QThreads_general_usage
    // https://mayaposch.wordpress.com/2011/11/01/how-to-really-truly-use-qthreads-the-full-explanation/
    curl_multi_thread = new QThread;
    curl_multi->moveToThread(curl_multi_thread);
    QObject::connect(this, SIGNAL(sendStartDownload(QString,QString,bool)), curl_multi, SLOT(recvNewDl(QString,QString,bool)));
    // QObject::connect(this, SIGNAL(sendStopDownload(QString)), curl_multi, SLOT(recvStopDl(QString)));
    QObject::connect(this, SIGNAL(finish_curl_multi_thread()), curl_multi_thread, SLOT(quit()));
    QObject::connect(this, SIGNAL(finish_curl_multi_thread()), curl_multi, SLOT(deleteLater()));
    QObject::connect(curl_multi_thread, SIGNAL(finished()), curl_multi_thread, SLOT(deleteLater()));
    curl_multi_thread->start();

    dlModel = new downloadModel(this);
    ui->downloadView->setModel(dlModel);
    ui->downloadView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->downloadView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->downloadView->horizontalHeader()->setStretchLastSection(true); // http://stackoverflow.com/questions/16931569/qstandarditemmodel-inside-qtableview
    ui->downloadView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->downloadView->setColumnHidden(MN_HIDDEN_UNIQUE_ID, true);

    QPointer<downloadDelegate> dlDel = new downloadDelegate(dlModel);
    ui->downloadView->setItemDelegate(dlDel);

    QObject::connect(ui->downloadView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_downloadView_customContextMenuRequested(QPoint)));
    QObject::connect(this, SIGNAL(updateDlStats()), this, SLOT(manageDlStats()));

    try {
        readFromHistoryFile();
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
    }

    QObject::connect(this, SIGNAL(terminate_xfers()), this, SLOT(terminate_curl_downloads()));

    QPointer<QShortcut> upKeyOverride = new QShortcut(QKeySequence(Qt::Key_Up), ui->downloadView);
    QPointer<QShortcut> downKeyOverride = new QShortcut(QKeySequence(Qt::Key_Down), ui->downloadView);
    QObject::connect(upKeyOverride, SIGNAL(activated()), this, SLOT(keyUpDlModelSlot()));
    QObject::connect(downKeyOverride, SIGNAL(activated()), this, SLOT(keyDownDlModelSlot()));

    resetDlStateStartup();
}

MainWindow::~MainWindow()
{
    delete ui;
    emit terminate_xfers();
    gk_dl_info_cache.clear();
}

/**
 * @brief MainWindow::addDownload adds a URL and its properties/information to the 'downloadView' TableView widget.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @note   <http://doc.qt.io/qt-5/qtwidgets-itemviews-addressbook-example.html#addresswidget-class-implementation>
 *         <https://en.wikipedia.org/wiki/List_of_HTTP_status_codes#2xx_Success>
 *         <http://mirror.internode.on.net/pub/test/>
 *         <http://doc.qt.io/qt-5/signalsandslots.html>
 * @param  url The URL of the file you wish to add.
 */
void MainWindow::addDownload()
{
    QPointer<AddURL> add_url = new AddURL(this);
    QObject::connect(add_url, SIGNAL(sendDetails(std::string,double,int,double,int,int,GekkoFyre::DownloadStatus,std::string,std::string,GekkoFyre::HashType,std::string,long long,bool,std::string,std::string,GekkoFyre::DownloadType)),
                     this, SLOT(sendDetails(std::string,double,int,double,int,int,GekkoFyre::DownloadStatus,std::string,std::string,GekkoFyre::HashType,std::string,long long,bool,std::string,std::string,GekkoFyre::DownloadType)));
    add_url->setAttribute(Qt::WA_DeleteOnClose, true);
    add_url->open();
    return;
}

/**
 * @brief MainWindow::readFromHistoryFile
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param fileName
 */
void MainWindow::readFromHistoryFile()
{
    std::vector<GekkoFyre::GkCurl::CurlDlInfo> dl_history;
    std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_torrent_history;
    dl_history = routines->readCurlItems();
    gk_torrent_history = routines->readTorrentInfo(true);

    dlModel->removeRows(0, (int)dl_history.size(), QModelIndex());
    for (size_t i = 0; i < dl_history.size(); ++i) {
        if (!dl_history.at(i).file_loc.empty()) {
            if (dl_history.at(i).ext_info.content_length > 0) {
                insertNewRow(dl_history.at(i).ext_info.effective_url,
                             dl_history.at(i).ext_info.content_length, 0, 0, 0, 0,
                             dl_history.at(i).dlStatus, dl_history.at(i).ext_info.effective_url,
                             dl_history.at(i).file_loc, dl_history.at(i).unique_id,
                             GekkoFyre::DownloadType::HTTP);
            } else {
                QMessageBox::information(this, tr("Problem!"), tr("The size of the download could not be "
                                                                  "determined. Please try again."),
                                         QMessageBox::Ok);
            }
        } else {
            QMessageBox::warning(this, tr("Error!"), tr("The item below has no download destination! FyreDL cannot "
                                                                "proceed with insertion of said item.\n\n\"%1\"\n\nTherefore "
                                                                "an attempt is being made at deleting this from the database.")
                    .arg(QUrl(QString::fromStdString(dl_history.at(i).ext_info.effective_url)).fileName()), QMessageBox::Ok);
            if (!dl_history.at(i).unique_id.empty()) {
                routines->delCurlItem("", dl_history.at(i).unique_id);
            }
        }
    }

    for (size_t i = 0; i < gk_torrent_history.size(); ++i) {
        if (!gk_torrent_history.at(i).general.down_dest.empty()) {
            GekkoFyre::GkTorrent::TorrentInfo gk_torrent_element = gk_torrent_history.at(i);
            double content_length = ((double)gk_torrent_element.general.num_pieces * (double)gk_torrent_element.general.piece_length);
            if (content_length > 0) {
                insertNewRow(gk_torrent_element.general.torrent_name, content_length, 0, 0, 0, 0,
                             gk_torrent_element.general.dlStatus, gk_torrent_element.general.magnet_uri,
                             gk_torrent_element.general.down_dest, gk_torrent_element.general.unique_id,
                             GekkoFyre::DownloadType::Torrent);
            } else {
                QMessageBox::warning(this, tr("Error!"), tr("Content length for the below torrent is either zero or "
                                                                    "negative! FyreDL cannot proceed with insertion "
                                                                    "of said torrent.\n\n%1")
                        .arg(QString::fromStdString(gk_torrent_element.general.torrent_name)), QMessageBox::Ok);
            }
        } else {
            QMessageBox::warning(this, tr("Error!"), tr("The torrent below has no download destination! FyreDL cannot "
                                                                "proceed with insertion of said torrent.\n\n\"%1\"")
                    .arg(QString::fromStdString(gk_torrent_history.at(i).general.torrent_name)), QMessageBox::Ok);
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

/**
 * @brief MainWindow::insertNewRow controls the insertion of data into the QTableView object, 'downloadView'.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @param fileName The name of the file to be inserted.
 * @param fileSize The size of the file.
 * @param downloaded How much data has already been downloaded.
 * @param progress The current progress towards download completion.
 * @param upSpeed Upload speed, i.e. for torrents.
 * @param downSpeed How fast the file is downloading at.
 * @param status Current download status, whether it be Paused, Stopped, Downloading, etc.
 * @param url The effective URL of the download in question.
 * @param destination The location of the download on the user's local storage.
 */
void MainWindow::insertNewRow(const std::string &fileName, const double &fileSize, const int &downloaded,
                              const double &progress, const int &upSpeed, const int &downSpeed,
                              const GekkoFyre::DownloadStatus &status, const std::string &url,
                              const std::string &destination, const std::string &unique_id,
                              const GekkoFyre::DownloadType &download_type)
{
    try {
        dlModel->insertRows(0, 1, QModelIndex());

        QModelIndex index = dlModel->index(0, MN_FILENAME_COL, QModelIndex());
        dlModel->setData(index, routines->extractFilename(QString::fromStdString(fileName)), Qt::DisplayRole);

        index = dlModel->index(0, MN_FILESIZE_COL, QModelIndex());
        dlModel->setData(index, routines->numberConverter(fileSize), Qt::DisplayRole);

        index = dlModel->index(0, MN_DOWNLOADED_COL, QModelIndex());
        dlModel->setData(index, QString::number(downloaded), Qt::DisplayRole);

        index = dlModel->index(0, MN_PROGRESS_COL, QModelIndex());
        dlModel->setData(index, QString::number(progress), Qt::DisplayRole);

        index = dlModel->index(0, MN_UPSPEED_COL, QModelIndex());
        dlModel->setData(index, QString::number(upSpeed), Qt::DisplayRole);

        index = dlModel->index(0, MN_DOWNSPEED_COL, QModelIndex());
        dlModel->setData(index, QString::number(downSpeed), Qt::DisplayRole);

        index = dlModel->index(0, MN_SEEDERS_COL, QModelIndex());
        dlModel->setData(index, tr("<N/A>"), Qt::DisplayRole);

        index = dlModel->index(0, MN_LEECHERS_COL, QModelIndex());
        dlModel->setData(index, tr("<N/A>"), Qt::DisplayRole);

        index = dlModel->index(0, MN_STATUS_COL, QModelIndex());
        dlModel->setData(index, routines->convDlStat_toString(status), Qt::DisplayRole);

        index = dlModel->index(0, MN_DESTINATION_COL, QModelIndex());
        dlModel->setData(index, QString::fromStdString(destination), Qt::DisplayRole);

        index = dlModel->index(0, MN_URL_COL, QModelIndex());
        dlModel->setData(index, QString::fromStdString(url), Qt::DisplayRole);

        index = dlModel->index(0, MN_HIDDEN_UNIQUE_ID, QModelIndex());
        dlModel->setData(index, QString::fromStdString(unique_id), Qt::DisplayRole);

        // Initialize the graphs
        initCharts(QString::fromStdString(unique_id), QString::fromStdString(destination), download_type);
        return;
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
        return;
    }

    return;
}

/**
 * @brief MainWindow::removeSelRows will remove/delete the currently selected rows from QTableView, 'downloadView',
 * and the appropriate XML history file.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-10-12
 * @note <http://doc.qt.io/qt-5/qtwidgets-itemviews-addressbook-example.html>
 * @see MainWindow::on_removeToolBtn_clicked()
 */
void MainWindow::removeSelRows()
{
    // Be sure not to delete items from within slots connected to signals that have the item (or its
    // index) as their parameter.
    const QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    const int countRow = indexes.count();

    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            try {
                const QString file_dest = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();
                const QString sel_row_unique_id = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_HIDDEN_UNIQUE_ID)).toString();
                std::vector<GekkoFyre::Global::DownloadInfo>::size_type i = 0;
                while (i < gk_dl_info_cache.size()) {
                    if (gk_dl_info_cache.at(i).unique_id == sel_row_unique_id) {
                        if (gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::HTTP || gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::FTP) {
                            // Remove the downloadable object from the Google LevelDB database
                            routines->delCurlItem(file_dest);

                            // Remove the downloadable object from the memory cache and then break the while-loop
                            gk_dl_info_cache.erase(gk_dl_info_cache.begin() + i);
                            break;
                        } else if (gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::Torrent || gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::TorrentMagnetLink) {
                            // Remove the downloadable object from the Google LevelDB database
                            routines->delTorrentItem(gk_dl_info_cache.at(i).unique_id.toStdString());

                            // Remove the downloadable object from the memory cache and then break the while-loop
                            gk_dl_info_cache.erase(gk_dl_info_cache.begin() + i);
                            break;
                        }
                    } else {
                        ++i;
                    }
                }

                // Remove the associated graph(s) from memory
                delCharts(sel_row_unique_id.toStdString());

                // ...and then the GUI
                dlModel->removeRows(indexes.at(0).row(), countRow, QModelIndex());

                return;
            } catch (const std::exception &e) {
                QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
                return;
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::resetDlStateStartup() scans for downloads stuck in a 'Downloading' state and resets them to 'Paused'.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-01
 */
void MainWindow::resetDlStateStartup()
{
    bool alreadyMentioned = false;
    for (int i = 0; i < dlModel->getList().size(); ++i) {
        QModelIndex find_index = dlModel->index(i, MN_STATUS_COL);
        if (find_index.isValid()) {
            const QString stat_string = ui->downloadView->model()->data(find_index).toString();
            if (stat_string == routines->convDlStat_toString(GekkoFyre::DownloadStatus::Downloading)) {
                QModelIndex file_dest_index = dlModel->index(i, MN_DESTINATION_COL);
                const QString file_dest_string = ui->downloadView->model()->data(file_dest_index).toString();
                QModelIndex unique_id_index = dlModel->index(i, MN_HIDDEN_UNIQUE_ID);
                const QString unique_id_string = ui->downloadView->model()->data(unique_id_index).toString();

                try {
                    for (const auto &item: gk_dl_info_cache) {
                        if (item.unique_id == unique_id_string) {
                            if (item.dl_type == GekkoFyre::DownloadType::HTTP || item.dl_type == GekkoFyre::DownloadType::FTP) {
                                bool ret_state = routines->modifyCurlItem(file_dest_string.toStdString(),
                                                                          GekkoFyre::DownloadStatus::Paused);
                                bool ret_update_col = dlModel->updateCol(find_index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Paused), MN_STATUS_COL);
                                if ((!ret_state || !ret_update_col) && !alreadyMentioned) {
                                    QMessageBox::warning(this, ("Problem!"), tr("There has been an error at startup. FyreDL was unable to correctly reset the state of some columns, either due to a GUI or database error."), QMessageBox::Ok);
                                    alreadyMentioned = true;
                                }
                            } else if (item.dl_type == GekkoFyre::DownloadType::Torrent || item.dl_type == GekkoFyre::DownloadType::TorrentMagnetLink) {
                                bool ret_state = routines->modifyTorrentItem(item.unique_id.toStdString(),
                                                                             GekkoFyre::DownloadStatus::Stopped);
                                bool ret_update_col = dlModel->updateCol(find_index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Stopped), MN_STATUS_COL);
                                if ((!ret_state || !ret_update_col) && !alreadyMentioned) {
                                    QMessageBox::warning(this, ("Problem!"), tr("There has been an error at startup. FyreDL was unable to correctly reset the state of some columns, either due to a GUI or database error."), QMessageBox::Ok);
                                    alreadyMentioned = true;
                                }
                            } else {
                                break;
                            }
                        }
                    }
                } catch (const std::exception &e) {
                    QMessageBox::warning(this, tr("Error!"), e.what(), QMessageBox::Ok);
                    return;
                }
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::initCharts
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-03
 * @param unique_id A unique identifier for the download in question.
 * @param down_dest Refers to the location of the download in question on local storage.
 * @param download_type The type of download, whether it be HTTP(S), FTP(S), or BitTorrent.
 */
void MainWindow::initCharts(const QString &unique_id, const QString &down_dest,
                            const GekkoFyre::DownloadType &download_type)
{
    if (!unique_id.isEmpty()) {
        if (gk_dl_info_cache.size() > 0) {
            for (size_t i = 0; i < gk_dl_info_cache.size(); ++i) {
                if (gk_dl_info_cache.at(i).unique_id == unique_id) {
                    throw std::invalid_argument(tr("Value, '%1', already exists when it shouldn't!")
                                                        .arg(gk_dl_info_cache.at(i).unique_id).toStdString());
                }
            }
        }

        GekkoFyre::Global::DownloadInfo graph_info;
        graph_info.xfer_graph.down_speed_vals.push_back(std::make_pair(0.0, 0.0));
        graph_info.xfer_graph.down_speed_init = false;
        graph_info.unique_id = unique_id;
        graph_info.dl_dest = down_dest;
        graph_info.dl_type = download_type;
        graph_info.stats.timer_begin = 0;

        GekkoFyre::GkGraph::GkXferStats xfer_stats_temp;
        xfer_stats_temp.download_rate = 0.0;
        xfer_stats_temp.upload_rate = 0.0;
        xfer_stats_temp.download_total = 0;
        xfer_stats_temp.upload_total = 0;
        xfer_stats_temp.progress_ppm = 0;
        graph_info.stats.xfer_stats.push_back(xfer_stats_temp);

        if (download_type == GekkoFyre::DownloadType::HTTP || download_type == GekkoFyre::DownloadType::FTP) {
            // Download type is either HTTP or FTP
            std::vector<GekkoFyre::GkCurl::CurlDlInfo> curl_dl_info = routines->readCurlItems();
            for (size_t j = 0; j < curl_dl_info.size(); ++j) {
                if (curl_dl_info.at(j).unique_id == unique_id.toStdString()) {
                    graph_info.curl_info = curl_dl_info.at(j);
                    graph_info.stats.content_length = graph_info.curl_info.value().ext_info.content_length;
                    break;
                }
            }
        } else if (download_type == GekkoFyre::DownloadType::Torrent ||
                download_type == GekkoFyre::DownloadType::TorrentMagnetLink) {
            // Download type is BitTorrent
            std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_torrent_info = routines->readTorrentInfo(false);
            for (size_t j = 0; j < gk_torrent_info.size(); ++j) {
                if (gk_torrent_info.at(j).general.unique_id == graph_info.unique_id.toStdString()) {
                    graph_info.to_info = gk_torrent_info.at(j);
                    graph_info.stats.content_length = ((double)graph_info.to_info.value().general.num_pieces *
                            (double)graph_info.to_info.value().general.piece_length);
                    break;
                }
            }
        } else {
            throw std::invalid_argument(tr("An invalid download type has been given!").toStdString());
        }

        gk_dl_info_cache.push_back(graph_info);
        return;
    } else {
        throw std::invalid_argument(tr("Unable to initialize charts! 'file_dest' is empty!").toStdString());
    }

    return;
}

/**
 * @brief MainWindow::displayCharts when giving the appropriate information, a chart will be displayed in
 * 'ui->tabStatusWidget', at index 'TAB_INDEX_GRAPH'.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11-13
 * @note <http://doc.qt.io/qt-5/qtcharts-modeldata-example.html>
 *       <http://doc.qt.io/qt-5/qtcharts-index.html>
 *       <http://doc.qt.io/qt-5/qtcharts-splinechart-example.html>
 *       <http://doc.qt.io/qt-5/qtcharts-zoomlinechart-chartview-cpp.html>
 * @param file_dest The file destination of the download in question, relating to the chart being displayed
 */
void MainWindow::displayCharts(const QString &unique_id)
{
    return;
}

/**
 * @brief MainWindow::delCharts will delete a chart and remove it from user-view when given the right
 * information. It is primarily used for when a download is deleted and/or the user switches away from
 * 'ui->tabStatusWidget', at index 'TAB_INDEX_GRAPH'.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11-13
 * @param file_dest The file destination of the download in question, relating to the chart being displayed
 * @see MainWindow::removeSelRows(), MainWindow::on_clearhistoryToolBtn_clicked()
 */
void MainWindow::delCharts(const std::string &file_dest)
{
    return;
}

void MainWindow::updateChart()
{
    return;
}

/**
 * @brief The model definition for the object, 'contentsView', within the 'mainwindow.ui' designer file.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-21
 * @note <http://doc.qt.io/qt-5/qtwidgets-itemviews-simpledommodel-example.html>
 *       <http://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html>
 *       <http://stackoverflow.com/questions/28997619/populating-qtreeview-from-list-of-file-paths>
 *       <http://stackoverflow.com/questions/27585077/create-qtreewidget-directories-based-on-file-paths-names>
 */
void MainWindow::contentsView_update()
{
    if (ENBL_GUI_CONTENTS_VIEW) {
        QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
        if (indexes.size() > 0) {
            if (indexes.at(0).isValid()) {
                const QString unique_id = ui->downloadView->model()->data(
                        ui->downloadView->model()->index(indexes.at(0).row(), MN_HIDDEN_UNIQUE_ID)).toString();
                for (size_t k = 0; k < gk_dl_info_cache.size(); ++k) {
                    try {

                        // Verify that we have the right download item
                        if (gk_dl_info_cache.at(k).unique_id == unique_id) {

                            // Verify that this download is of the type 'BitTorrent'
                            if (gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::Torrent ||
                                    gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::TorrentMagnetLink) {

                                // Read the XML data into memory
                                std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_ti = routines->readTorrentInfo(false);
                                std::ostringstream oss_data;
                                for (size_t i = 0; i < gk_ti.size(); ++i) {
                                    if (gk_ti.at(i).general.unique_id == unique_id.toStdString()) {
                                        QHash <QString, QPair<QString, QString>> columnData; // <root, <child, parent>>
                                        std::vector<GekkoFyre::GkTorrent::TorrentFile> gk_tf_vec = gk_ti.at(i).files_vec;

                                        for (size_t j = 0; j < gk_tf_vec.size(); ++j) {
                                            GekkoFyre::GkTorrent::TorrentFile gk_tf_element = gk_tf_vec.at(j);
                                            fs::path boost_path(gk_tf_element.file_path);

                                            // Process the XML data
                                            for (auto &indice: boost_path) {
                                                QString cv_root = QString::fromStdString(boost_path.parent_path().string());
                                                columnData.insertMulti(cv_root, qMakePair(QString::fromStdString(indice.string()), cv_root));
                                            }
                                        }

                                        QSet<QString> files_toProc; // <child, parent>, directories ready to be processed.
                                        for (auto const &entry: columnData) {
                                            // This will find all the directories and the appropriate column number for each directory.
                                            QString child = entry.first;
                                            QString parent = entry.second;

                                            fs::path boost_child_path(child.toStdString());
                                            QString dir_name = QDir(parent).dirName();

                                            if (boost_child_path.has_extension() && dir_name != child) {
                                                // We are quite likely to have a file, and not a (sub-)directory
                                                QString full_path = parent + fs::path::preferred_separator + child;
                                                files_toProc.insert(full_path);
                                            }
                                        }

                                        QList<QString> dirs_pair = files_toProc.values();

                                        cV_model = new QStandardItemModel();
                                        QStandardItem *topLevelItem = cV_model->invisibleRootItem();

                                        // Iterate over each directory string
                                        QStringList dir_list;
                                        for (int j = 0; j < dirs_pair.size(); ++j) {
                                            dir_list << dirs_pair.at(j);
                                        }

                                        qSort(dir_list.begin(), dir_list.end());
                                        for (auto const &item: dir_list) {
                                            QStringList splitName = item.split(fs::path::preferred_separator);

                                            // First part of the string is defo parent item
                                            // Check to make sure not to add duplicate
                                            if (cV_model->findItems(splitName[0], Qt::MatchFixedString).size() == 0) {
                                                QStandardItem *parentItem = new QStandardItem(splitName[0]);
                                                topLevelItem->appendRow(parentItem);
                                            }

                                            cV_addItems(topLevelItem, splitName);
                                        }

                                        QStringList headers;
                                        headers << tr("Dir/File");
                                        cV_model->setHorizontalHeaderLabels(headers);
                                        ui->contentsView->setModel(cV_model);
                                    }
                                }
                            } else {
                                ui->contentsView->setModel(nullptr);
                            }
                        }
                    } catch (const std::exception &e) {
                        QMessageBox::warning(this, tr("Error!"), tr("%1\n\nAborting...").arg(e.what()), QMessageBox::Ok);
                        QCoreApplication::exit(-1); // Terminate application with return code '-1'.
                    }
                }
            }
        }
    }

    return;
}

void MainWindow::cV_addItems(QStandardItem *parent, const QStringList &elements)
{
    for (auto const &element: elements) {
        // First check if this element already exists in the hierarchy
        int noOfChildren = parent->rowCount();

        // If there are child objects under specified parent
        if (noOfChildren > 0) {
            // Create dict to store all child objects under parent for test
            QHash<QString, QStandardItem*> childObjsList;

            // Iterate over indexes and get names of all child objects
            for (int i = 0; i < noOfChildren; ++i) {
                QStandardItem *childObj(parent->child(i));

                if (childObj != nullptr) {
                    childObjsList[childObj->text()] = childObj;
                } else {
                    throw std::runtime_error(tr("'childObj' is null within 'contentView'!").toStdString());
                }
            }

            if (childObjsList.contains(element)) {
                // Only run recursive function if there are still elements to work on
                if (elements.size() > 0) {
                    QStringList split_elements;
                    for (int i = 1; i < elements.size(); ++i) {
                        split_elements << elements.at(i);
                    }

                    cV_addItems(childObjsList[element], split_elements);
                }

                return;
            } else {
                // Item does not exist yet, create it and parent
                QStandardItem *newObj = new QStandardItem(element);
                parent->appendRow(newObj);

                // Only run recursive function if there are still elements
                if (elements.size() > 0) {
                    QStringList split_elements;
                    for (int i = 1; i < elements.size(); ++i) {
                        split_elements << elements.at(i);
                    }

                    cV_addItems(newObj, split_elements);
                }

                return;
            }
        } else {
            // If there are no existing child objects, it's safe to create the item and parent it
            QStandardItem *newObj = new QStandardItem(element);
            parent->appendRow(newObj);

            // Only run recursive function if there are still elements to work on
            if (elements.size() > 0) {
                // Now run the recursive function again with the latest object as the parent and the rest of the
                // elements as children.
                QStringList split_elements;
                for (int i = 1; i < elements.size(); ++i) {
                    split_elements << elements.at(i);
                }

                cV_addItems(newObj, split_elements);
            }

            return;
        }
    }
}

/**
 * @brief MainWindow::askDeleteHttpItem poses a QMessageBox to the user, asking whether they want to delete a pre-existing download
 * before restarting the same one, to the same destination.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-01
 * @param file_dest The destination of the download on the user's local storage.
 * @return Whether the user selected 'yes', or 'no/cancel'.
 */
bool MainWindow::askDeleteHttpItem(const QString &file_dest, const QString &unique_id, const bool &noRestart)
{
    fs::path dest_boost_path(file_dest.toStdString());
    if (fs::exists(dest_boost_path)) {
        QMessageBox file_ask;
        file_ask.setWindowTitle(tr("Pre-existing file!"));
        if (noRestart) {
            file_ask.setText(tr("Do you want to erase the file, \"%1\"?").arg(file_dest));
        } else {
            file_ask.setText(tr("Do you want to erase the file and then restart the download?"));
        }

        file_ask.setIcon(QMessageBox::Information);
        file_ask.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        file_ask.setDefaultButton(QMessageBox::No);
        file_ask.setModal(false);
        int ret = file_ask.exec();

        switch (ret) {
            case QMessageBox::Yes:
                try {
                    sys::error_code ec;
                    fs::remove(dest_boost_path, ec);
                    if (ec != sys::errc::success) {
                        throw ec.category().name();
                    }

                    if (!noRestart) {
                        startHttpDownload(file_dest, unique_id, false);
                    }

                    return true;
                } catch (const std::exception &e) {
                    QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                    return false;
                }
            case QMessageBox::No:
                return false;
            case QMessageBox::Cancel:
                return false;
            default:
                return false;
        }
    }

    return false;
}

/**
 * @brief MainWindow::startHttpDownload contains the routines necessary to instantiate a new HTTP/FTP download.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-01
 * @param file_dest The destination of the download in question on the user's local storage.
 * @param resumeDl Whether we are resuming a pre-existing download item or not. There must be certain conditions in-place
 * for this to work.
 */
void MainWindow::startHttpDownload(const QString &file_dest, const QString &unique_id, const bool &resumeDl)
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            const QString url = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString();
            QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
            const GekkoFyre::DownloadStatus status = routines->convDlStat_StringToEnum(ui->downloadView->model()->data(index).toString());
            try {
                for (size_t k = 0; k < gk_dl_info_cache.size(); ++k) {
                    if (gk_dl_info_cache.at(k).unique_id == unique_id) {
                        if (gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::HTTP ||
                                gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::FTP) {
                            // TODO: QFutureWatcher<GekkoFyre::CurlMulti::CurlInfo> *verifyFileFutWatch;
                            if (status != GekkoFyre::DownloadStatus::Downloading) {
                                double freeDiskSpace = (double)routines->freeDiskSpace(QDir(file_dest).absolutePath());
                                GekkoFyre::GkCurl::CurlInfoExt extended_info = GekkoFyre::CurlEasy::curlGrabInfo(url);
                                if ((unsigned long int)((extended_info.content_length * FREE_DSK_SPACE_MULTIPLIER) < freeDiskSpace)) {
                                    routines->modifyCurlItem(file_dest.toStdString(),
                                                             GekkoFyre::DownloadStatus::Downloading);
                                    dlModel->updateCol(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Downloading), MN_STATUS_COL);

                                    QObject::connect(GekkoFyre::routine_singleton::instance(), SIGNAL(sendXferStats(GekkoFyre::GkCurl::CurlProgressPtr)), this, SLOT(recvCurl_XferStats(GekkoFyre::GkCurl::CurlProgressPtr)));
                                    QObject::connect(GekkoFyre::routine_singleton::instance(), SIGNAL(sendDlFinished(GekkoFyre::GkCurl::DlStatusMsg)), this, SLOT(recvDlFinished(GekkoFyre::GkCurl::DlStatusMsg)));

                                    // This is required for signaling, otherwise QVariant does not know the type.
                                    qRegisterMetaType<GekkoFyre::GkCurl::CurlProgressPtr>("curlProgressPtr");
                                    qRegisterMetaType<GekkoFyre::GkCurl::DlStatusMsg>("DlStatusMsg");

                                    // Emit the signal data necessary to initiate a download
                                    emit sendStartDownload(url, file_dest, resumeDl);
                                    return;
                                } else {
                                    throw std::runtime_error(tr("Not enough free disk space!").toStdString());
                                }
                            }
                        }
                    }
                }
            } catch (const std::exception &e) {
                QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::startTorrentDl instantiates a new BitTorrent session and thus, the beginning/resuming of a download.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-23
 * @param unique_id is the unique identifier relating to the BitTorrent download item in question. It is gathered from
 * the XML history file or the hidden QTableView column, 'MN_HIDDEN_UNIQUE_ID'.
 * @param resumeDl Whether to resume a pre-existing BitTorrent download or not. There must be certain conditions in-place
 * for this to work.
 */
void MainWindow::startTorrentDl(const QString &unique_id, const bool &resumeDl)
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
    std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_ti = routines->readTorrentInfo(false);
    for (auto const &indice: gk_ti) {
        if (indice.general.unique_id == unique_id.toStdString()) {
            QObject::connect(gk_torrent_client, SIGNAL(xfer_torrent_info(GekkoFyre::GkTorrent::TorrentResumeInfo)), this, SLOT(recvBitTorrent_XferStats(GekkoFyre::GkTorrent::TorrentResumeInfo)));
            gk_torrent_client->startTorrentDl(indice);
            routines->modifyTorrentItem(unique_id.toStdString(), GekkoFyre::DownloadStatus::Downloading);
            dlModel->updateCol(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Downloading), MN_STATUS_COL);
            break;
        }
    }

    return;
}

void MainWindow::haltDownload()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            try {
                const QString url = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString();
                const QString dest = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();
                const QString unique_id = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_HIDDEN_UNIQUE_ID)).toString();

                QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
                const GekkoFyre::DownloadStatus status = routines->convDlStat_StringToEnum(ui->downloadView->model()->data(index).toString());

                for (size_t k = 0; k < gk_dl_info_cache.size(); ++k) {
                    if (gk_dl_info_cache.at(k).unique_id == unique_id) {
                        if (gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::HTTP ||
                                gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::FTP) {
                            if (status != GekkoFyre::DownloadStatus::Stopped) {
                                if (status != GekkoFyre::DownloadStatus::Completed) {
                                    if (status != GekkoFyre::DownloadStatus::Failed) {
                                        if (status != GekkoFyre::DownloadStatus::Invalid) {
                                            QObject::connect(this, SIGNAL(sendStopDownload(QString)), GekkoFyre::routine_singleton::instance(), SLOT(recvStopDl(QString)));
                                            emit sendStopDownload(dest);
                                            routines->modifyCurlItem(dest.toStdString(),
                                                                     GekkoFyre::DownloadStatus::Stopped);
                                            dlModel->updateCol(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Stopped), MN_STATUS_COL);
                                            askDeleteHttpItem(dest, unique_id, true);
                                        }
                                    } else {
                                        QMessageBox::warning(this, tr("Error!"), tr("Please delete the download before re-adding the information once more!"), QMessageBox::Ok);
                                        return;
                                    }
                                }
                            }
                        } else if (gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::Torrent) {
                            // Torrent
                            return;
                        }
                    }
                }
            } catch (const std::exception &e) {
                QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                return;
            }
        }
    }
}

/**
 * @brief MainWindow::resumeDownload initiates the transfer of a download. By default, this will resume transfer
 * of a 'paused' download, but any newly added files must be set to a 'paused' state first. Newly added files that are found
 * to already be existing will cause a prompt to be displayed to the user, asking if they want to delete these files in order
 * to continue the transfer.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-10
 * @note <https://solarianprogrammer.com/2012/10/17/cpp-11-async-tutorial/>
 *       <http://doc.qt.io/qt-5/qthreadpool.html>
 *       <http://doc.qt.io/qt-5/qthread.html>
 *       <http://en.cppreference.com/w/cpp/thread/packaged_task>
 */
void MainWindow::resumeDownload()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            const QString dest = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();
            fs::path dest_boost_path(dest.toStdString());
            QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
            const GekkoFyre::DownloadStatus status = routines->convDlStat_StringToEnum(ui->downloadView->model()->data(index).toString());
            for (size_t k = 0; k < gk_dl_info_cache.size(); ++k) {
                const QString unique_id = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_HIDDEN_UNIQUE_ID)).toString();
                if (gk_dl_info_cache.at(k).unique_id == unique_id) {
                    if (gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::HTTP ||
                            gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::FTP) {
                        try {
                            switch (status) {
                                case GekkoFyre::DownloadStatus::Paused:
                                    if (fs::exists(dest_boost_path) && fs::is_regular_file(dest_boost_path)) {
                                        // The file still exists, so resume downloading since it's from a paused state!
                                        startHttpDownload(dest, unique_id, true);
                                    } else {
                                        startHttpDownload(dest, unique_id, false);
                                    }

                                    return;
                                case GekkoFyre::DownloadStatus::Unknown:
                                    // TODO: Fill out the code for this!
                                case GekkoFyre::DownloadStatus::Stopped:
                                    if (fs::exists(dest_boost_path) && fs::is_regular_file(dest_boost_path)) {
                                        // We have an existing file!
                                        QMessageBox file_ask;
                                        file_ask.setIcon(QMessageBox::Information);
                                        file_ask.setWindowTitle(tr("Pre-existing file!"));
                                        file_ask.setText(
                                                tr("A pre-existing file, \"%1\", with the same name has been detected. "
                                                           "Would you like to continue with the download? Press 'No' to delete or "
                                                           "'Cancel' to deal with manually on your own.").arg(dest));
                                        file_ask.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                                        file_ask.setDefaultButton(QMessageBox::Yes);
                                        file_ask.setModal(false);
                                        int ret = file_ask.exec();

                                        switch (ret) {
                                            case QMessageBox::Yes:
                                                // TODO: Add smarts to see if the pre-existing file is the same size as the download in question!
                                                startHttpDownload(dest, unique_id, true);
                                                return;
                                            case QMessageBox::No:
                                                if (fs::exists(dest_boost_path) && fs::is_regular_file(dest_boost_path)) {
                                                    sys::error_code ec;
                                                    fs::remove(dest_boost_path, ec);
                                                    if (ec != sys::errc::success) {
                                                        throw ec.category().name();
                                                    }

                                                    startHttpDownload(dest, unique_id, false);
                                                } else {
                                                    throw std::runtime_error(
                                                            tr("Error with deleting file and/or starting download!")
                                                                    .toStdString());
                                                }

                                                return;
                                            case QMessageBox::Cancel:
                                                return;
                                            default:
                                                return;
                                        }
                                    } else {
                                        // We have NO existing file...
                                        startHttpDownload(dest, unique_id, false);
                                    }

                                    return;
                                default:
                                    QMessageBox::warning(this, tr("Error!"), tr("Please delete the download before re-adding the information once more!"), QMessageBox::Ok);
                                    return;
                            }
                        } catch (const std::exception &e) {
                            QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                        }
                    } else if (gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::Torrent ||
                            gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::TorrentMagnetLink) {
                        // Torrent
                        startTorrentDl(gk_dl_info_cache.at(k).unique_id, false);
                        return;
                    }
                }
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::pauseDownload resumes the download if the connection is paused and pause is deactivated. Otherwise,
 * it halts the download mid-transfer and maintains a connection to the server for as long as naturally possible.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-02
 */
void MainWindow::pauseDownload()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();

    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            try {
                const QString url = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString();
                const QString dest = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();
                const QString unique_id = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_HIDDEN_UNIQUE_ID)).toString();

                QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
                const GekkoFyre::DownloadStatus status = routines->convDlStat_StringToEnum(ui->downloadView->model()->data(index).toString());

                if (status != GekkoFyre::DownloadStatus::Paused) {
                    for (size_t k = 0; k < gk_dl_info_cache.size(); ++k) {
                        if (gk_dl_info_cache.at(k).unique_id == unique_id) {
                            if (gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::HTTP ||
                                gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::FTP) {
                                if (status != GekkoFyre::DownloadStatus::Completed && status != GekkoFyre::DownloadStatus::Stopped &&
                                    status != GekkoFyre::DownloadStatus::Failed && status != GekkoFyre::DownloadStatus::Invalid) {
                                    QObject::connect(this, SIGNAL(sendStopDownload(QString)), GekkoFyre::routine_singleton::instance(), SLOT(recvStopDl(QString)));
                                    emit sendStopDownload(dest);
                                    routines->modifyCurlItem(dest.toStdString(), GekkoFyre::DownloadStatus::Paused);
                                    dlModel->updateCol(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Paused), MN_STATUS_COL);
                                } else {
                                    QMessageBox::warning(this, tr("Error!"), tr("Please delete the download before re-adding the information once more!"), QMessageBox::Ok);
                                    return;
                                }
                            } else if (gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::Torrent) {
                                // Torrent
                                return;
                            }
                        }
                    }
                }
            } catch (const std::exception &e) {
                QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                return;
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::restartDownload restarts the download from the beginning. Prompts the user with a QMessageBox
 * first, notifying them of the surety of their action.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-02
 */
void MainWindow::restartDownload()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            const QString dest = ui->downloadView->model()->data(
                    ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();
            const QString unique_id = ui->downloadView->model()->data(
                    ui->downloadView->model()->index(indexes.at(0).row(), MN_HIDDEN_UNIQUE_ID)).toString();
            fs::path dest_boost_path(dest.toStdString());
            QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
            const GekkoFyre::DownloadStatus status = routines->convDlStat_StringToEnum(
                    ui->downloadView->model()->data(index).toString());
            for (size_t k = 0; k < gk_dl_info_cache.size(); ++k) {
                if (gk_dl_info_cache.at(k).unique_id == unique_id) {
                    if (gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::HTTP ||
                            gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::FTP) {
                        if (status != GekkoFyre::DownloadStatus::Invalid) {
                            if (status != GekkoFyre::DownloadStatus::Failed) {
                                QMessageBox file_ask;
                                file_ask.setIcon(QMessageBox::Information);
                                file_ask.setWindowTitle(tr("Pre-existing file!"));
                                file_ask.setText(
                                        tr("Are you sure you wish to restart the download of file, \"%1\", from the very beginning?").arg(dest));
                                file_ask.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                                file_ask.setDefaultButton(QMessageBox::No);
                                file_ask.setModal(false);
                                int ret = file_ask.exec();

                                try {
                                    switch (ret) {
                                        case QMessageBox::Yes:
                                            if (fs::exists(dest_boost_path) && fs::is_regular_file(dest_boost_path)) {
                                                sys::error_code ec;
                                                fs::remove(dest_boost_path, ec);
                                                if (ec != sys::errc::success) {
                                                    throw ec.category().name();
                                                }

                                                startHttpDownload(dest, unique_id, false);
                                            } else {
                                                throw std::runtime_error(tr("Error with deleting file and/or starting download!")
                                                                                 .toStdString());
                                            }

                                            return;
                                        case QMessageBox::No:
                                            return;
                                        default:
                                            return;
                                    }
                                } catch (const std::exception &e) {
                                    QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                                    return;
                                }
                            }
                        }
                    } else if (gk_dl_info_cache.at(k).dl_type == GekkoFyre::DownloadType::Torrent) {
                        // Torrent
                        return;
                    }
                }
            }
        }
    }
}

/**
 * @brief MainWindow::clearOldHistory will clear any completed downloads from the QTableView, 'downloadView', and from
 * the corresponding XML history file.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @note  <http://stackoverflow.com/questions/8613737/how-can-i-remove-elements-from-a-qlist-while-iterating-over-it-using-foreach>
 */
void MainWindow::clearOldHistory()
{
    QMessageBox file_ask;
    file_ask.setIcon(QMessageBox::Information);
    file_ask.setWindowTitle(tr("FyreDL"));
    file_ask.setText(tr("Are you sure about clearing all completed downloads?"));
    file_ask.setStandardButtons(QMessageBox::Apply | QMessageBox::No | QMessageBox::Cancel);
    file_ask.setDefaultButton(QMessageBox::No);
    file_ask.setModal(false);
    int ret = file_ask.exec();

    switch (ret) {
        case QMessageBox::Apply:
        {
            QModelIndexList indexes;
            for (int i = 0; i < dlModel->getList().size(); ++i) {
                QModelIndex find_index = dlModel->index(i, MN_STATUS_COL);
                const QString stat_string = ui->downloadView->model()->data(find_index).toString();
                if (stat_string == routines->convDlStat_toString(GekkoFyre::DownloadStatus::Completed)) {
                    indexes.push_back(find_index);
                    QModelIndex file_dest_index = dlModel->index(i, MN_DESTINATION_COL);
                    const QString file_dest_string = ui->downloadView->model()->data(file_dest_index).toString();
                    const fs::path boost_file_dest(file_dest_string.toStdString());

                    try {
                        if (!fs::is_directory(boost_file_dest) && fs::is_regular_file(boost_file_dest)) {
                            delCharts(file_dest_string.toStdString());
                            routines->delCurlItem(file_dest_string);
                        } else {
                            QModelIndex serial_col_index = dlModel->index(i, MN_HIDDEN_UNIQUE_ID);
                            const std::string serial_col_string = ui->downloadView->model()->data(serial_col_index).toString().toStdString();
                            delCharts(serial_col_string);
                            routines->delTorrentItem(serial_col_string);
                        }
                    } catch (const std::exception &e) {
                        QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                        return;
                    }
                }
            }

            for (int i = indexes.count(); i > 0; --i) {
                dlModel->removeRow(indexes.at(i - 1).row(), QModelIndex());
            }

            return;
        }
        case QMessageBox::No:
            return;
        case QMessageBox::Cancel:
            return;
        default:
            return;
    }

    return;
}

void MainWindow::removeDlItem()
{
    removeSelRows();
}

/**
 * @brief MainWindow::general_extraDetails appears as a selectable tab for the download in question, where extra details
 * about the item are provided.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-03
 * @see MainWindow::manageDlStats()
 */
void MainWindow::general_extraDetails()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            const QModelIndex url_index = dlModel->index(indexes.at(0).row(), MN_URL_COL, QModelIndex());
            const QModelIndex dest_index = dlModel->index(indexes.at(0).row(), MN_DESTINATION_COL, QModelIndex());
            const QModelIndex size_index = dlModel->index(indexes.at(0).row(), MN_FILESIZE_COL, QModelIndex());
            const QModelIndex unique_id_index = dlModel->index(indexes.at(0).row(), MN_HIDDEN_UNIQUE_ID, QModelIndex());
            const QString url_string = ui->downloadView->model()->data(url_index).toString();
            const QString dest_string = ui->downloadView->model()->data(dest_index).toString();
            const QString size_string = ui->downloadView->model()->data(size_index).toString();
            const QString unique_id_string = ui->downloadView->model()->data(unique_id_index).toString();
            ui->fileNameDataLabel->setText(QUrl(url_string).fileName());
            ui->destDataLabel->setText(QString::fromStdString(fs::path(dest_string.toStdString()).remove_filename().string()));
            ui->totSizeDataLabel->setText(size_string);
            ui->urlDataLabel->setText(url_string);

            long long insert_time = 0;
            long long complt_time = 0;
            double content_length = 0;
            std::string hash_val_given = "";
            std::string hash_val_calc = "";
            QString hashType = "";
            for (size_t i = 0; i < gk_dl_info_cache.size(); ++i) {
                if (gk_dl_info_cache.at(i).unique_id == unique_id_string) {
                    if (gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::HTTP ||
                            gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::FTP) {
                        // This is a HTTP(S) or FTP(S) download!
                        if (gk_dl_info_cache.at(i).curl_info.is_initialized()) {
                            insert_time = gk_dl_info_cache.at(i).curl_info.get().insert_timestamp;
                            complt_time = gk_dl_info_cache.at(i).curl_info.get().complt_timestamp;
                            content_length = gk_dl_info_cache.at(i).curl_info.get().ext_info.content_length;
                            hash_val_given = gk_dl_info_cache.at(i).curl_info.get().hash_val_given;
                            hash_val_calc = gk_dl_info_cache.at(i).curl_info.get().hash_val_rtrnd;
                            hashType = routines->convHashType_toString(gk_dl_info_cache.at(i).curl_info.get().hash_type);
                        }

                        break;
                    } else if (gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::Torrent ||
                            gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::TorrentMagnetLink) {
                        // This is a BitTorrent download!
                        if (gk_dl_info_cache.at(i).to_info.is_initialized()) {
                            insert_time = gk_dl_info_cache.at(i).to_info.get().general.insert_timestamp;
                            complt_time = gk_dl_info_cache.at(i).to_info.get().general.complt_timestamp;
                            content_length = ((double)gk_dl_info_cache.at(i).to_info.get().general.num_pieces *
                                              (double)gk_dl_info_cache.at(i).to_info.get().general.piece_length);
                            hash_val_given = tr("N/A").toStdString();
                            hash_val_calc = tr("N/A").toStdString();
                            hashType = tr("Unknown");
                        }
                    } else {
                        // Throw an exception!
                        throw std::invalid_argument(tr("An invalid download-type has been given! It must be either a "
                                                               "HTTP(S)/FTP(S) or BitTorrent download.").toStdString());
                    }
                }
            }

            double curr_dl_amount = 0;
            double curr_dl_speed = 0;
            if (gk_dl_info_cache.size() > 0) {
                for (size_t i = 0; i < gk_dl_info_cache.size(); ++i) {
                    if (gk_dl_info_cache.at(i).dl_dest == dest_string) {
                        if (gk_dl_info_cache.at(i).stats.xfer_stats.size() > 0) {
                            curr_dl_amount = (double)gk_dl_info_cache.at(i).stats.xfer_stats.back().download_total;
                            curr_dl_speed = gk_dl_info_cache.at(i).stats.xfer_stats.back().download_rate;
                            break;
                        }

                        break;
                    }
                }
            }

            if (curr_dl_amount != 0) {
                double estWaitTime = routines->estimatedTimeLeft(content_length, curr_dl_amount, curr_dl_speed);
                ui->estWaitTimeDataLabel->setText(routines->timeBeautify(estWaitTime));
            } else {
                ui->estWaitTimeDataLabel->setText(tr("N/A"));
            }

            ui->hashTypeDataLabel->setText(hashType);

            if (insert_time > 0) {
                // We have a timestamp
                ui->createTimeDataLabel->setText(QDateTime::fromTime_t((uint)insert_time).toString());
            } else {
                // Insert a 'N/A'
                ui->createTimeDataLabel->setText(tr("N/A"));
            }

            if (complt_time > 0) {
                // We have a timestamp
                ui->compTimeDataLabel->setText(QDateTime::fromTime_t((uint)complt_time).toString());
            } else {
                // Insert a 'N/A'
                ui->compTimeDataLabel->setText(tr("N/A"));
            }

            if (!hash_val_given.empty()) {
                // We have a hash value!
                ui->givenHashDataLabel->setText(QString::fromStdString(hash_val_given));
            } else {
                // Insert a 'N/A'
                ui->givenHashDataLabel->setText(tr("N/A"));
            }

            if (!hash_val_calc.empty()) {
                // We have a calculated hash value!
                ui->calcHashDataLabel->setText(QString::fromStdString(hash_val_calc));
            } else {
                // Insert a 'N/A'
                ui->calcHashDataLabel->setText(tr("N/A"));
            }
        }
    }

    return;
}

/**
 * @brief MainWindow::transfer_extraDetails appears as a selectable tab for the download in question, where extra details
 * about the item are provided.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-03
 * @see MainWindow::manageDlStats()
 */
void MainWindow::transfer_extraDetails()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            QModelIndex url_index = dlModel->index(indexes.at(0).row(), MN_URL_COL, QModelIndex());
            const QString url_string = ui->downloadView->model()->data(url_index).toString();
        }
    }

    return;
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
    About *about_dialog = new About(this);
    about_dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    about_dialog->open();
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
    resumeDownload();
    return;
}

void MainWindow::on_dlpauseToolBtn_clicked()
{
    pauseDownload();
    return;
}

void MainWindow::on_dlstopToolBtn_clicked()
{
    haltDownload();
    return;
}

void MainWindow::on_removeToolBtn_clicked()
{
    removeDlItem();
    return;
}

void MainWindow::on_clearhistoryToolBtn_clicked()
{
    clearOldHistory();
    return;
}

void MainWindow::on_settingsToolBtn_clicked()
{
    Settings *settingsUi = new Settings(this);
    settingsUi->setAttribute(Qt::WA_DeleteOnClose, true);
    settingsUi->open();
}

void MainWindow::on_restartToolBtn_clicked()
{
    restartDownload();
    return;
}

/**
 * @brief MainWindow::on_actionComma_Separated_Values_triggered upon activation, will export the list of downloads
 * within 'downloadView' to a given text file in the format of a CSV file.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-06
 * @note <http://doc.qt.io/qt-5/qfiledialog.html#getSaveFileName>
 */
void MainWindow::on_actionComma_Separated_Values_triggered()
{
    QMessageBox::information(this, tr("Problem!"), tr("This function is not implemented yet! Check "
                                                              "back another time."), QMessageBox::Ok);
    return;
}

/**
 * @brief MainWindow::on_actionRe_set_all_dialog_prompts_triggered
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-03
 */
void MainWindow::on_actionRe_set_all_dialog_prompts_triggered()
{
    QMessageBox::information(this, tr("Problem!"), tr("This function is not implemented yet! Check "
                                                              "back another time."), QMessageBox::Ok);
}

/**
 * @brief MainWindow::on_actionPreference_s_triggered
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-03
 */
void MainWindow::on_actionPreference_s_triggered()
{
    QMessageBox::information(this, tr("Problem!"), tr("This function is not implemented yet! Check "
                                                              "back another time."), QMessageBox::Ok);
}

/**
 * @brief MainWindow::on_action_Halt_triggered
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-03
 */
void MainWindow::on_action_Halt_triggered()
{
    haltDownload();
    return;
}

/**
 * @brief MainWindow::on_action_Resume_triggered
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-03
 */
void MainWindow::on_action_Resume_triggered()
{
    resumeDownload();
    return;
}

/**
 * @brief MainWindow::on_action_Pause_triggered
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-03
 */
void MainWindow::on_action_Pause_triggered()
{
    pauseDownload();
    return;
}

/**
 * @brief MainWindow::on_action_Cleanup_erroneous_tasks_triggered
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-03
 */
void MainWindow::on_action_Cleanup_erroneous_tasks_triggered()
{
    clearOldHistory();
    return;
}

/**
 * @brief MainWindow::on_actionR_estart_triggered
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-03
 */
void MainWindow::on_actionR_estart_triggered()
{
    restartDownload();
    return;
}

/**
 * @brief MainWindow::on_actionEdi_t_triggered
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-03
 */
void MainWindow::on_actionEdi_t_triggered()
{}

/**
 * @brief MainWindow::on_action_Delete_triggered
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-03
 */
void MainWindow::on_action_Delete_triggered()
{
    removeDlItem();
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

/**
 * @brief MainWindow::on_downloadView_activated is initiated whenever a row in 'downloadView' is activated say by
 * pressing Enter, double-clicking, etc.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-04
 * @param index The currently selected row.
 */
void MainWindow::on_downloadView_activated(const QModelIndex &index)
{
    Q_UNUSED(index);
    general_extraDetails();
    transfer_extraDetails();
    contentsView_update();
}

/**
 * @brief MainWindow::on_downloadView_clicked is initiated whenever a row is activated by a single mouse-click.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-04
 * @param index The currently selected row.
 */
void MainWindow::on_downloadView_clicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    general_extraDetails();
    transfer_extraDetails();
    contentsView_update();
}

/**
 * @brief MainWindow::keyUpDlModelSlot is activated whenever the 'Key_Up' is pressed, anywhere inside the program.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-04
 */
void MainWindow::keyUpDlModelSlot()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    int row = indexes.at(0).row();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            --row;
            ui->downloadView->selectRow(row);

            updateChart();
            general_extraDetails();
            transfer_extraDetails();
            contentsView_update();
        }
    }
}

/**
 * @brief MainWindow::keyDownDlModelSlot is activated whenever the 'Key_Down' is pressed, anywhere inside the program.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-04
 */
void MainWindow::keyDownDlModelSlot()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    int row = indexes.at(0).row();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            ++row;
            ui->downloadView->selectRow(row);

            updateChart();
            general_extraDetails();
            transfer_extraDetails();
            contentsView_update();
        }
    }
}

void MainWindow::sendDetails(const std::string &fileName, const double &fileSize, const int &downloaded,
                             const double &progress, const int &upSpeed, const int &downSpeed,
                             const GekkoFyre::DownloadStatus &status, const std::string &url,
                             const std::string &destination, const GekkoFyre::HashType &hash_type,
                             const std::string &hash_val, const long long &resp_code, const bool &stat_ok,
                             const std::string &stat_msg, const std::string &unique_id,
                             const GekkoFyre::DownloadType &down_type)
{
    QList<std::vector<QString>> list = dlModel->getList();

    for (int i = 0; i < list.size(); ++i) {
        for (size_t j = 0; j < list.at(i).size(); ++j) {
            if (down_type == GekkoFyre::DownloadType::HTTP || down_type == GekkoFyre::DownloadType::FTP) {
                if (list.at(i).at(j) == QString::fromStdString(destination)) {
                    QMessageBox::information(this, tr("Duplicate entry..."), tr("There has been an attempt at a duplicate "
                                                                                        "entry!\n\n%1")
                            .arg(QString::fromStdString(destination)), QMessageBox::Ok);
                    return;
                }
            } else {
                if (list.at(i).at(j) == QString::fromStdString(url)) {
                    QMessageBox::information(this, tr("Duplicate entry..."), tr("There has been an attempt at a duplicate "
                                                                                        "entry!\n\n%1")
                            .arg(QString::fromStdString(destination)), QMessageBox::Ok);
                    return;
                }
            }
        }
    }

    insertNewRow(fileName, fileSize, downloaded, progress, upSpeed, downSpeed, status, url, destination,
                 unique_id, down_type);

    if (down_type == GekkoFyre::DownloadType::HTTP || down_type == GekkoFyre::DownloadType::FTP) {
        // Write the output to an XML file, with file-name 'CFG_HISTORY_FILE'
        GekkoFyre::GkCurl::CurlDlInfo dl_info;
        dl_info.dlStatus = status;
        dl_info.file_loc = destination;
        dl_info.ext_info.content_length = fileSize;
        dl_info.ext_info.effective_url = url;
        dl_info.ext_info.response_code = resp_code;
        dl_info.ext_info.status_ok = stat_ok;
        dl_info.ext_info.status_msg = stat_msg;
        dl_info.insert_timestamp = 0;
        dl_info.hash_type = hash_type;
        dl_info.hash_val_given = hash_val;
        dl_info.unique_id = unique_id;
        routines->addCurlItem(dl_info);
    }

    return;
}

/**
 * @brief MainWindow::recvCurl_XferStats receives the statistics regarding a HTTP(S)/FTP(S) download.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10-23
 * @note   <http://stackoverflow.com/questions/9086372/how-to-compare-pointers>
 *         <http://doc.qt.io/qt-5/qmutexlocker.html>
 *         <http://stackoverflow.com/questions/19981161/using-qmutexlocker-to-protect-shared-variables-when-running-function-with-qtconc>
 * @param info is the struct related to the download info.
 * @see MainWindow::manageDlStats()
 */
void MainWindow::recvCurl_XferStats(const GekkoFyre::GkCurl::CurlProgressPtr &info)
{
    QMutex curl_multi_mutex;
    GekkoFyre::GkGraph::GkXferStats stats_temp;
    std::time_t cur_time_temp;
    std::time(&cur_time_temp);
    stats_temp.cur_time = cur_time_temp;
    curl_multi_mutex.lock();

    stats_temp.download_rate = info.stat.back().dlnow;
    stats_temp.upload_rate = info.stat.back().upnow;
    stats_temp.download_total = info.stat.back().dltotal; // TODO: Modify this value to be the file-size of the download instead.
    stats_temp.upload_total = info.stat.back().uptotal;

    for (size_t i = 0; i < gk_dl_info_cache.size(); ++i) {
        if (gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::HTTP ||
                gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::FTP) {
            if (gk_dl_info_cache.at(i).dl_dest == QString::fromStdString(info.file_dest)) {
                if (gk_dl_info_cache.at(i).stats.timer_begin == 0) {
                    // If it doesn't exist, push the whole of 'prog_temp' onto the private, class-global 'dl_stat' and
                    // thusly updateDlStats().
                    std::time(&gk_dl_info_cache.at(i).stats.timer_begin);
                    gk_dl_info_cache.at(i).stats.xfer_stats.push_back(stats_temp);
                    emit updateDlStats();
                    goto finish_loop;
                } else {
                    // If it does exist, then only update certain elements instead and thusly updateDlStats() also.
                    gk_dl_info_cache.at(i).stats.xfer_stats.push_back(stats_temp);
                    emit updateDlStats();
                    goto finish_loop;
                }
            }
        }
    }

    finish_loop: ;
    curl_multi_mutex.unlock();
    return;
}

/**
 * @brief MainWindow::manageDlStats is central to managing the statistics of a HTTP(S)/FTP(S)/BitTorrent download, updating
 * the relevant columns on the GUI, and if needed, updating the database also.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-10-23
 * @see MainWindow::recvCurl_XferStats(), MainWindow::recvBitTorrent_XferStats()
 */
void MainWindow::manageDlStats()
{
    try {
        for (int i = 0; i < dlModel->getList().size(); ++i) {
            QModelIndex find_index = dlModel->index(i, MN_DESTINATION_COL);
            QModelIndex unique_id_index = dlModel->index(i, MN_HIDDEN_UNIQUE_ID);
            const QString dest_string = ui->downloadView->model()->data(find_index).toString();
            const QString unique_id_string = ui->downloadView->model()->data(unique_id_index).toString();

            for (size_t j = 0;  j < gk_dl_info_cache.size(); ++j) {
                if (gk_dl_info_cache.at(j).stats.xfer_stats.size() > 1) {
                    if (unique_id_string == gk_dl_info_cache.at(j).unique_id) {
                        GekkoFyre::GkGraph::GkXferStats xfer_stat_indice = gk_dl_info_cache.at(j).stats.xfer_stats.back();
                        std::ostringstream oss_dl_bps, oss_ul_bps;

                        oss_dl_bps << routines->numberConverter(xfer_stat_indice.download_rate).toStdString() << tr("/sec").toStdString();

                        if (xfer_stat_indice.upload_rate.is_initialized()) {
                            oss_ul_bps << routines->numberConverter(xfer_stat_indice.upload_rate.value()).toStdString() << tr("/sec").toStdString();
                        } else {
                            oss_ul_bps << "0" << tr("/sec").toStdString();
                        }

                        long cur_dl_amount = xfer_stat_indice.download_total;
                        // long cur_ul_amount = xfer_stat_indice.upload_total;

                        if (gk_dl_info_cache.at(j).dl_type == GekkoFyre::DownloadType::Torrent || gk_dl_info_cache.at(j).dl_type == GekkoFyre::DownloadType::TorrentMagnetLink) {
                            double progress_percent = std::round((xfer_stat_indice.progress_ppm / 1000000) * 100);
                            dlModel->updateCol(dlModel->index(i, MN_PROGRESS_COL), QString::number(progress_percent), MN_PROGRESS_COL);
                        } else if (gk_dl_info_cache.at(j).dl_type == GekkoFyre::DownloadType::HTTP || gk_dl_info_cache.at(j).dl_type == GekkoFyre::DownloadType::FTP) {
                            long cur_file_size = GekkoFyre::CmnRoutines::getFileSize(gk_dl_info_cache.at(j).dl_dest.toStdString());
                            dlModel->updateCol(dlModel->index(i, MN_PROGRESS_COL), routines->percentDownloaded(gk_dl_info_cache.at(j).stats.content_length, (double)cur_file_size), MN_PROGRESS_COL);
                        }

                        dlModel->updateCol(dlModel->index(i, MN_DOWNSPEED_COL), QString::fromStdString(oss_dl_bps.str()), MN_DOWNSPEED_COL);
                        dlModel->updateCol(dlModel->index(i, MN_UPSPEED_COL), QString::fromStdString(oss_ul_bps.str()), MN_UPSPEED_COL);
                        dlModel->updateCol(dlModel->index(i, MN_DOWNLOADED_COL), routines->numberConverter((double)cur_dl_amount), MN_DOWNLOADED_COL);

                        if (xfer_stat_indice.cur_time.is_initialized()) {
                            if (gk_dl_info_cache.at(j).stats.timer_begin == 0) {
                                throw std::invalid_argument(tr("An invalid timer value has been given! Please contact the developer about this bug and with what conditions caused it.").toStdString());
                            }

                            double passed_time = std::difftime(xfer_stat_indice.cur_time.value(), gk_dl_info_cache.at(j).stats.timer_begin);
                            gk_dl_info_cache.at(j).xfer_graph.down_speed_vals.push_back(std::make_pair(xfer_stat_indice.download_rate, passed_time));
                        }

                        general_extraDetails();
                        transfer_extraDetails();
                        updateChart();
                        continue;
                    }
                } else {
                    continue;
                }
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @brief MainWindow::downloadFin is a slot that is executed upon finishing of a download.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10-15
 * @param status The information pertaining to the completed download.
 */
void MainWindow::recvDlFinished(const GekkoFyre::GkCurl::DlStatusMsg &status)
{
    try {
        for (int i = 0; i < dlModel->getList().size(); ++i) {
            QModelIndex find_index_url = dlModel->index(i, MN_URL_COL);
            QModelIndex find_index_dest = dlModel->index(i, MN_DESTINATION_COL);
            QString url = ui->downloadView->model()->data(find_index_url).toString();
            QString dest = ui->downloadView->model()->data(find_index_dest).toString();

            if (dest.toStdString() == status.file_loc) {
                QModelIndex stat_index = dlModel->index(i, MN_STATUS_COL);
                if (routines->convDlStat_StringToEnum(ui->downloadView->model()->data(stat_index).toString()) ==
                    GekkoFyre::DownloadStatus::Downloading) {

                    GekkoFyre::GkFile::FileHash file_hash;
                    std::vector<GekkoFyre::GkCurl::CurlDlInfo> dl_mini_info_vec = routines->readCurlItems(true);
                    for (size_t j = 0; j < dl_mini_info_vec.size(); ++j) {
                        if (dl_mini_info_vec.at(j).file_loc == status.file_loc) {
                            switch (dl_mini_info_vec.at(j).hash_type) {
                                case GekkoFyre::HashType::CannotDetermine:
                                case GekkoFyre::HashType::None:
                                    file_hash = routines->cryptoFileHash(
                                            QString::fromStdString(status.file_loc), GekkoFyre::HashType::SHA1, "");
                                    break;
                                default:
                                    file_hash = routines->cryptoFileHash(
                                            QString::fromStdString(status.file_loc), dl_mini_info_vec.at(j).hash_type,
                                            QString::fromStdString(dl_mini_info_vec.at(j).hash_val_given));
                                    break;
                            }

                            break;
                        }
                    }

                    routines->modifyCurlItem(status.file_loc, GekkoFyre::DownloadStatus::Completed,
                                             QDateTime::currentDateTime().toTime_t(),
                                             file_hash.checksum.toStdString(), file_hash.hash_type);
                    dlModel->updateCol(stat_index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Completed),
                                       MN_STATUS_COL);

                    // Update the 'downloaded amount' because the statistics routines are not always accurate, due to only
                    // running every few seconds at most. This causes inconsistencies.
                    dlModel->updateCol(dlModel->index(i, MN_DOWNLOADED_COL),
                                       routines->numberConverter(status.content_len), MN_DOWNLOADED_COL);
                    return;
                }
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), QString("%1\n\nAborting...").arg(e.what()), QMessageBox::Ok);
        QCoreApplication::exit(-1); // Terminate application with return code '-1'.
        return;
    }

    return;
}

void MainWindow::terminate_curl_downloads()
{
    emit finish_curl_multi_thread();
    curl_multi_thread->quit();
    curl_multi_thread->wait();
}

/**
 * @brief MainWindow::recvBitTorrent_XferStats receives the processed statistics from the built-in BitTorrent client.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-22
 * @param gk_xfer_info is the statistics in question, in the form of a struct.
 * @see MainWindow::manageDlStats()
 */
void MainWindow::recvBitTorrent_XferStats(const GekkoFyre::GkTorrent::TorrentResumeInfo &gk_xfer_info)
{
    QMutex to_ses_mutex;
    GekkoFyre::GkGraph::GkXferStats stats_temp;
    std::time_t cur_time_temp;
    std::time(&cur_time_temp);
    stats_temp.cur_time = cur_time_temp;
    to_ses_mutex.lock();
    stats_temp.progress_ppm = gk_xfer_info.xfer_stats.get().progress_ppm;
    stats_temp.upload_rate = gk_xfer_info.xfer_stats.get().ul_rate;
    stats_temp.download_rate = gk_xfer_info.xfer_stats.get().dl_rate;
    stats_temp.download_total = gk_xfer_info.total_downloaded;
    stats_temp.upload_total = gk_xfer_info.total_uploaded;
    stats_temp.num_pieces_dled = gk_xfer_info.xfer_stats.get().num_pieces_downloaded;

    for (size_t i = 0; i < gk_dl_info_cache.size(); ++i) {
        if (gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::Torrent ||
                gk_dl_info_cache.at(i).dl_type == GekkoFyre::DownloadType::TorrentMagnetLink) {
            if (gk_dl_info_cache.at(i).dl_dest == QString::fromStdString(gk_xfer_info.save_path)) {
                if (gk_dl_info_cache.at(i).stats.timer_begin == 0) {
                    // If it doesn't exist, push the whole of 'prog_temp' onto the private, class-global 'dl_stat' and
                    // thusly updateDlStats().
                    std::time(&gk_dl_info_cache.at(i).stats.timer_begin);
                    gk_dl_info_cache.at(i).stats.xfer_stats.push_back(stats_temp);
                    gk_dl_info_cache.at(i).to_info.get().to_resume_info = gk_xfer_info;

                    emit updateDlStats();
                    goto finish_loop;
                } else {
                    // If it does exist, then only update certain elements instead and thusly updateDlStats() also.
                    gk_dl_info_cache.at(i).stats.xfer_stats.push_back(stats_temp);
                    gk_dl_info_cache.at(i).to_info.get().to_resume_info = gk_xfer_info;

                    emit updateDlStats();
                    goto finish_loop;
                }
            }
        }
    }

    finish_loop: ;
    to_ses_mutex.unlock();
    return;
}
