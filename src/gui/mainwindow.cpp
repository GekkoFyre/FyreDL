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
 * @file mainwindow.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief The code and functions behind the, 'mainwindow.ui', designer file.
 */

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "settings.hpp"
#include "./../singleton_proc.hpp"
#include "./../curl_easy.hpp"
#include "about.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <boost/system/error_code.hpp>
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

    try {
        #ifdef _WIN32
        if (!routines->singleAppInstance_Win32()) {
            QCoreApplication::quit(); // Exit with status code '0'
        }
        #elif __linux__
        SingletonProcess singleton(37563);
        if (!singleton()) {
            throw std::runtime_error("Application is already open!");
        }
        #endif
    } catch (const std::exception &e) {
        QMessageBox::information(this, tr("Problem!"), QString("%1").arg(e.what()), QMessageBox::Ok);
        QCoreApplication::quit(); // Exit with status code '0'
        return;
    }

    QMessageBox::warning(this, tr("FyreDL"), tr("FyreDL is currently under intense development at this "
                                                "time! Please only use at your own risk."),
                         QMessageBox::Ok);

    routines = new GekkoFyre::CmnRoutines();
    curl_multi = new GekkoFyre::CurlMulti();

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

    dlModel = new downloadModel();
    ui->downloadView->setModel(dlModel);
    ui->downloadView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->downloadView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->downloadView->horizontalHeader()->setStretchLastSection(true); // http://stackoverflow.com/questions/16931569/qstandarditemmodel-inside-qtableview
    ui->downloadView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->downloadView->setColumnHidden(MN_HIDDEN_UNIQUE_ID, true);

    QObject::connect(ui->downloadView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_downloadView_customContextMenuRequested(QPoint)));
    QObject::connect(this, SIGNAL(updateDlStats()), this, SLOT(manageDlStats()));

    try {
        readFromHistoryFile();
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
    }

    QShortcut *upKeyOverride = new QShortcut(QKeySequence(Qt::Key_Up), ui->downloadView);
    QShortcut *downKeyOverride = new QShortcut(QKeySequence(Qt::Key_Down), ui->downloadView);
    QObject::connect(upKeyOverride, SIGNAL(activated()), this, SLOT(keyUpDlModelSlot()));
    QObject::connect(downKeyOverride, SIGNAL(activated()), this, SLOT(keyDownDlModelSlot()));

    curr_shown_graphs = "";
    resetDlStateStartup();
}

MainWindow::~MainWindow()
{
    try {
        GekkoFyre::GkSettings::FyreDL settings;
        settings.main_win_x = ui->centralWidget->geometry().width();
        settings.main_win_y = ui->centralWidget->geometry().height();
        if (routines->writeXmlSettings(settings) == 1) {
            routines->modifyXmlSettings(settings);
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
    }

    delete ui;
    emit finish_curl_multi_thread();
    delete routines;
    dl_stat.clear();
    graph_init.clear();
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
    QObject::connect(add_url, SIGNAL(sendDetails(std::string,double,int,double,int,int,GekkoFyre::DownloadStatus,std::string,std::string,GekkoFyre::HashType,std::string,long long,bool,std::string,std::string,GekkoFyre::DownloadType)),
                     this, SLOT(sendDetails(std::string,double,int,double,int,int,GekkoFyre::DownloadStatus,std::string,std::string,GekkoFyre::HashType,std::string,long long,bool,std::string,std::string,GekkoFyre::DownloadType)));
    add_url->setAttribute(Qt::WA_DeleteOnClose, true);
    add_url->open();
    return;
}

/**
 * @brief MainWindow::readFromHistoryFile
 * @author         Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param fileName
 */
void MainWindow::readFromHistoryFile()
{
    std::vector<GekkoFyre::GkCurl::CurlDlInfo> dl_history;
    std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_torrent_history;
    if (fs::exists(routines->findCfgFile(CFG_HISTORY_FILE)) &&
            fs::is_regular_file(routines->findCfgFile(CFG_HISTORY_FILE))) {
        try {
            dl_history = routines->readDownloadInfo(CFG_HISTORY_FILE);
            gk_torrent_history = routines->readTorrentInfo(true, CFG_HISTORY_FILE);
        } catch (const std::exception &e) {
            QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
        }
    } else {
        return;
    }

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
                                                                "proceed with insertion of said item.\n\n\"%1\"")
                    .arg(QUrl(QString::fromStdString(dl_history.at(i).ext_info.effective_url)).fileName()), QMessageBox::Ok);
        }
    }

    for (size_t i = 0; i < gk_torrent_history.size(); ++i) {
        if (!gk_torrent_history.at(i).down_dest.empty()) {
            GekkoFyre::GkTorrent::TorrentInfo gk_torrent_element = gk_torrent_history.at(i);
            double content_length = ((double)gk_torrent_element.num_pieces * (double)gk_torrent_element.piece_length);
            if (content_length > 0) {
                insertNewRow(gk_torrent_element.torrent_name, content_length, 0, 0, 0, 0,
                             gk_torrent_element.dlStatus, gk_torrent_element.magnet_uri,
                             gk_torrent_element.down_dest, gk_torrent_element.unique_id,
                             GekkoFyre::DownloadType::Torrent);
            } else {
                QMessageBox::warning(this, tr("Error!"), tr("Content length for the below torrent is either zero or "
                                                                    "negative! FyreDL cannot proceed with insertion "
                                                                    "of said torrent.\n\n%1")
                        .arg(QString::fromStdString(gk_torrent_element.torrent_name)), QMessageBox::Ok);
            }
        } else {
            QMessageBox::warning(this, tr("Error!"), tr("The torrent below has no download destination! FyreDL cannot "
                                                                "proceed with insertion of said torrent.\n\n\"%1\"")
                    .arg(QString::fromStdString(gk_torrent_history.at(i).torrent_name)), QMessageBox::Ok);
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

    index = dlModel->index(0, MN_STATUS_COL, QModelIndex());
    dlModel->setData(index, routines->convDlStat_toString(status), Qt::DisplayRole);

    index = dlModel->index(0, MN_DESTINATION_COL, QModelIndex());
    dlModel->setData(index, QString::fromStdString(destination), Qt::DisplayRole);

    index = dlModel->index(0, MN_URL_COL, QModelIndex());
    dlModel->setData(index, QString::fromStdString(url), Qt::DisplayRole);

    index = dlModel->index(0, MN_HIDDEN_UNIQUE_ID, QModelIndex());
    dlModel->setData(index, QString::fromStdString(unique_id), Qt::DisplayRole);

    // Initialize the graphs
    initCharts(QString::fromStdString(unique_id), download_type);

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
                const fs::path boost_file_dest(file_dest.toStdString());
                if (!fs::is_directory(boost_file_dest) && fs::is_regular_file(boost_file_dest)) {
                    // Now remove the row from the XML file...
                    routines->delDownloadItem(file_dest);

                    // Remove the associated graph(s) from memory
                    delCharts(file_dest.toStdString());
                } else {
                    const std::string serial_col = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_HIDDEN_UNIQUE_ID)).toString().toStdString();

                    // Now remove the row from the XML file...
                    routines->delTorrentItem(serial_col);

                    // Remove the associated graph(s) from memory
                    delCharts(serial_col);
                }

                // ...and then the GUI
                dlModel->removeRows(indexes.at(0).row(), countRow, QModelIndex());

                return;
            } catch (const std::exception &e) {
                QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
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
    for (int i = 0; i < dlModel->getList().size(); ++i) {
        QModelIndex find_index = dlModel->index(i, MN_STATUS_COL);
        if (find_index.isValid()) {
            const QString stat_string = ui->downloadView->model()->data(find_index).toString();
            if (stat_string == routines->convDlStat_toString(GekkoFyre::DownloadStatus::Downloading)) {
                QModelIndex file_dest_index = dlModel->index(i, MN_DESTINATION_COL);
                const QString file_dest_string = ui->downloadView->model()->data(file_dest_index).toString();

                try {
                    routines->modifyDlState(file_dest_string.toStdString(), GekkoFyre::DownloadStatus::Paused);
                    dlModel->updateCol(find_index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Paused), MN_STATUS_COL);
                } catch (const std::exception &e) {
                    QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                    return;
                }
            }
        }
    }

    return;
}

void MainWindow::initCharts(const QString &unique_id, const GekkoFyre::DownloadType &download_type)
{
    if (!unique_id.isEmpty()) {
        GekkoFyre::GkGraph::GraphInit graph;
        graph.down_speed.down_speed_vals.push_back(std::make_pair(0.0, 0.0));
        graph.down_speed.down_speed_init = false;
        graph.unique_id = unique_id;
        graph.down_info.dl_type = download_type;
        graph_init.push_back(graph);
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
    for (size_t i = 0; i < graph_init.size(); ++i) {
        if (ui->tabStatusWidget->currentIndex() == TAB_INDEX_GRAPH) {
            if (!graph_init.at(i).down_speed.down_speed_init) {
                // Create the needed 'series'
                graph_init.at(i).down_speed.down_speed_series = new QtCharts::QLineSeries(this);
                graph_init.at(i).down_speed.down_speed_init = true;
            }

            if (graph_init.at(i).unique_id == unique_id) {
                if (curr_shown_graphs != unique_id) {
                    // NOTE: Only clear and/or then show the QChartView once, if 'file_dest' has not changed despite this
                    // function being called.
                    QString prev_shown_graphs = curr_shown_graphs;
                    curr_shown_graphs = unique_id;

                    graph_init.at(i).down_speed.down_speed_series->setName(tr("Download speed spline %1").arg(i));

                    QString file_dest_string = "nullptr";
                    for (int k = 0; k < dlModel->getList().size(); ++k) {
                        QModelIndex find_id_index = dlModel->index(k, MN_HIDDEN_UNIQUE_ID);
                        const QString id_string = ui->downloadView->model()->data(find_id_index).toString();
                        if (id_string == unique_id) {
                            QModelIndex file_dest_index = dlModel->index(k, MN_DESTINATION_COL);
                            file_dest_string = ui->downloadView->model()->data(file_dest_index).toString();
                            break;
                        }
                    }

                    QFile qfile_path(file_dest_string);

                    for (size_t j = 0; j < graph_init.at(i).down_speed.down_speed_vals.size(); ++j) {
                        graph_init.at(i).down_speed.down_speed_series->append(graph_init.at(i).down_speed.down_speed_vals.at(j).second,
                                                                              graph_init.at(i).down_speed.down_speed_vals.at(j).first);
                    }

                    // Create the needed QChart and set its initial properties
                    QtCharts::QChart *chart = new QtCharts::QChart();
                    chart->addSeries(graph_init.at(i).down_speed.down_speed_series);
                    chart->setTitle(QString("%1").arg(qfile_path.fileName()));
                    chart->createDefaultAxes();
                    chart->axisY()->setTitleText(tr("Download speed (KB/sec)")); // The title of the y-axis
                    chart->axisX()->setTitleText(tr("Time passed (seconds)")); // The title of the x-axis
                    chart->legend()->hide();

                    // Create the QChartView, which displays the graph, and set some initial properties
                    QtCharts::QChartView *chartView = new QtCharts::QChartView(chart);
                    chartView->setRenderHint(QPainter::Antialiasing);

                    if (!ui->graphVerticalLayout->isEmpty()) { // Check to see if the layout contains any widgets
                        routines->clearLayout(ui->graphVerticalLayout);
                    }

                    for (size_t j = 0; j < graph_init.size(); ++j) {
                        if (graph_init.at(j).unique_id == prev_shown_graphs) {
                            graph_init.at(j).down_speed.down_speed_init = false;
                            break;
                        }
                    }

                    ui->graphVerticalLayout->insertWidget(0, chartView);
                    chartView->setAlignment(Qt::AlignCenter);
                    chartView->show();
                    ui->graphVerticalLayout->update();
                    return;
                } else {
                    for (size_t j = 0; j < graph_init.at(i).down_speed.down_speed_vals.size(); ++j) {
                        graph_init.at(i).down_speed.down_speed_series->append(graph_init.at(i).down_speed.down_speed_vals.at(j).second,
                                                                              graph_init.at(i).down_speed.down_speed_vals.at(j).first);
                    }

                    return;
                }
            }
        }
    }

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
    /*
    if (!graph_init.empty()) {
        for (size_t i = 0; i < graph_init.size(); ++i) {
            if (!graph_init.at(i).file_dest.isEmpty() &&
                    graph_init.at(i).down_speed.down_speed_series != nullptr) {
                if (graph_init.at(i).file_dest.toStdString() == file_dest) {
                    graph_init.erase(graph_init.begin() + (long)i);
                    return;
                }
            }
        }
    }

    throw std::invalid_argument(tr("'delCharts()' failed!").toStdString());
     */
}

void MainWindow::updateChart()
{
    if (ENBL_GUI_CHARTS) {
        QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();

        if (indexes.size() > 0) {
            if (indexes.at(0).isValid()) {
                const QString dest = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();
                displayCharts(dest);
            }
        }
    }

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
                for (size_t k = 0; k < graph_init.size(); ++k) {
                    try {

                        // Verify that we have the right download item
                        if (graph_init.at(k).unique_id == unique_id) {

                            // Verify that this download is of the type 'BitTorrent'
                            if (graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::Torrent ||
                                graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::TorrentMagnetLink) {

                                // Read the XML data into memory
                                std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_ti = GekkoFyre::CmnRoutines::readTorrentInfo(false);
                                std::ostringstream oss_data;
                                for (size_t i = 0; i < gk_ti.size(); ++i) {
                                    if (gk_ti.at(i).unique_id == unique_id.toStdString()) {
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

                                        cV_model = std::make_unique<QStandardItemModel>();
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
                                        ui->contentsView->setModel(cV_model.get());
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
 * @brief MainWindow::askDeleteFile poses a QMessageBox to the user, asking whether they want to delete a pre-existing download
 * before restarting the same one, to the same destination.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-01
 * @param file_dest The destination of the download on the user's local storage.
 * @return Whether the user selected 'yes', or 'no/cancel'.
 */
bool MainWindow::askDeleteFile(const QString &file_dest, const bool &noRestart)
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

                    return true;
                } catch (const std::exception &e) {
                    QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                    return false;
                }
            case QMessageBox::No:
                return false;
            case QMessageBox::Cancel:
                return false;
        }
    }

    return false;
}

/**
 * @brief MainWindow::startDownload contains the routines necessary to instantiate a new download.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-01
 * @param file_dest The destination of the download in question on the user's local storage.
 * @param resumeDl Whether we are resuming a pre-existing download or not.
 */
void MainWindow::startDownload(const QString &file_dest, const bool &resumeDl)
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            const QString url = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString();
            const QString dest = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();

            QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
            const GekkoFyre::DownloadStatus status = routines->convDlStat_StringToEnum(ui->downloadView->model()->data(index).toString());

            try {
                // TODO: QFutureWatcher<GekkoFyre::CurlMulti::CurlInfo> *verifyFileFutWatch;
                GekkoFyre::GkCurl::CurlInfo verify = GekkoFyre::CurlEasy::verifyFileExists(url);
                if (verify.response_code == 200) {
                    if (status != GekkoFyre::DownloadStatus::Downloading) {
                        double freeDiskSpace = (double)routines->freeDiskSpace(QDir(dest).absolutePath());
                        GekkoFyre::GkCurl::CurlInfoExt extended_info = GekkoFyre::CurlEasy::curlGrabInfo(url);
                        if ((unsigned long int)((extended_info.content_length * FREE_DSK_SPACE_MULTIPLIER) < freeDiskSpace)) {
                            routines->modifyDlState(dest.toStdString(), GekkoFyre::DownloadStatus::Downloading);
                            dlModel->updateCol(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Downloading), MN_STATUS_COL);

                            QObject::connect(GekkoFyre::routine_singleton::instance(), SIGNAL(sendXferStats(GekkoFyre::GkCurl::CurlProgressPtr)), this, SLOT(recvXferStats(GekkoFyre::GkCurl::CurlProgressPtr)));
                            QObject::connect(GekkoFyre::routine_singleton::instance(), SIGNAL(sendDlFinished(GekkoFyre::GkCurl::DlStatusMsg)), this, SLOT(recvDlFinished(GekkoFyre::GkCurl::DlStatusMsg)));

                            // This is required for signaling, otherwise QVariant does not know the type.
                            qRegisterMetaType<GekkoFyre::GkCurl::CurlProgressPtr>("curlProgressPtr");
                            qRegisterMetaType<GekkoFyre::GkCurl::DlStatusMsg>("DlStatusMsg");

                            // Emit the signal data necessary to initiate a download
                            emit sendStartDownload(url, dest, resumeDl);
                            return;
                        } else {
                            throw std::runtime_error(tr("Not enough free disk space!").toStdString());
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

                for (size_t k = 0; k < graph_init.size(); ++k) {
                    if (graph_init.at(k).unique_id == unique_id) {
                        if (graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::HTTP ||
                            graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::FTP) {
                            if (status != GekkoFyre::DownloadStatus::Stopped) {
                                if (status != GekkoFyre::DownloadStatus::Completed) {
                                    if (status != GekkoFyre::DownloadStatus::Failed) {
                                        if (status != GekkoFyre::DownloadStatus::Invalid) {
                                            QObject::connect(this, SIGNAL(sendStopDownload(QString)), GekkoFyre::routine_singleton::instance(), SLOT(recvStopDl(QString)));
                                            emit sendStopDownload(dest);
                                            routines->modifyDlState(dest.toStdString(), GekkoFyre::DownloadStatus::Stopped);
                                            dlModel->updateCol(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Stopped), MN_STATUS_COL);
                                            askDeleteFile(dest, true);
                                        }
                                    } else {
                                        QMessageBox::warning(this, tr("Error!"), tr("Please delete the download before re-adding the information once more!"), QMessageBox::Ok);
                                        return;
                                    }
                                }
                            }
                        } else if (graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::Torrent) {
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
            const QString dest = ui->downloadView->model()->data(
                    ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();
            fs::path dest_boost_path(dest.toStdString());
            QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
            const GekkoFyre::DownloadStatus status = routines->convDlStat_StringToEnum(
                    ui->downloadView->model()->data(index).toString());
            for (size_t k = 0; k < graph_init.size(); ++k) {
                const QString unique_id = ui->downloadView->model()->data(
                        ui->downloadView->model()->index(indexes.at(0).row(), MN_HIDDEN_UNIQUE_ID)).toString();
                if (graph_init.at(k).unique_id == unique_id) {
                    if (graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::HTTP ||
                        graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::FTP) {
                        try {
                            switch (status) {
                                case GekkoFyre::DownloadStatus::Paused:
                                    if (fs::exists(dest_boost_path) && fs::is_regular_file(dest_boost_path)) {
                                        // The file still exists, so resume downloading since it's from a paused state!
                                        startDownload(dest, true);
                                    } else {
                                        // We have NO existing file... notify the user.
                                        QMessageBox file_ask;
                                        file_ask.setIcon(QMessageBox::Information);
                                        file_ask.setWindowTitle(tr("Problem!"));
                                        file_ask.setText(tr("The file no longer exists! Should I clear the history for this item?").arg(dest));
                                        file_ask.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                                        file_ask.setDefaultButton(QMessageBox::Yes);
                                        file_ask.setModal(false);
                                        int ret = file_ask.exec();

                                        switch (ret) {
                                            case QMessageBox::Yes:
                                                removeSelRows();
                                                return;
                                            case QMessageBox::No:
                                                routines->modifyDlState(dest.toStdString(), GekkoFyre::DownloadStatus::Failed);
                                                dlModel->updateCol(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Failed), MN_STATUS_COL);
                                                return;
                                            case QMessageBox::Cancel:
                                                return;
                                            default:
                                                return;
                                        }
                                    }

                                    return;
                                case GekkoFyre::DownloadStatus::Unknown:
                                case GekkoFyre::DownloadStatus::Stopped:
                                    if (fs::exists(dest_boost_path) && fs::is_regular_file(dest_boost_path)) {
                                        // We have an existing file!
                                        QMessageBox file_ask;
                                        file_ask.setIcon(QMessageBox::Information);
                                        file_ask.setWindowTitle(tr("Pre-existing file!"));
                                        file_ask.setText(
                                                tr("A pre-existing file, \"%1\", with the same name has been detected. "
                                                           "Delete to continue? Press 'No' to deal with manually.").arg(dest));
                                        file_ask.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                                        file_ask.setDefaultButton(QMessageBox::Yes);
                                        file_ask.setModal(false);
                                        int ret = file_ask.exec();

                                        switch (ret) {
                                            case QMessageBox::Yes:
                                                if (fs::exists(dest_boost_path) &&
                                                    fs::is_regular_file(dest_boost_path)) {
                                                    sys::error_code ec;
                                                    fs::remove(dest_boost_path, ec);
                                                    if (ec != sys::errc::success) {
                                                        throw ec.category().name();
                                                    }

                                                    startDownload(dest, false);
                                                } else {
                                                    throw std::runtime_error(
                                                            tr("Error with deleting file and/or starting download!")
                                                                    .toStdString());
                                                }

                                                return;
                                            case QMessageBox::No:
                                                QMessageBox::information(this, tr("Pre-existing file"), tr("You must deal with the file "
                                                                                                                   "manually in order to "
                                                                                                                   "continue the download."),
                                                                         QMessageBox::Ok);
                                                return;
                                            case QMessageBox::Cancel:
                                                return;
                                            default:
                                                return;
                                        }
                                    } else {
                                        // We have NO existing file...
                                        startDownload(dest, false);
                                    }

                                    return;
                                default:
                                    QMessageBox::warning(this, tr("Error!"), tr("Please delete the download before re-adding the information once more!"), QMessageBox::Ok);
                                    return;
                            }
                        } catch (const std::exception &e) {
                            QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                        }
                    } else if (graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::Torrent) {
                        // Torrent
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

                for (size_t k = 0; k < graph_init.size(); ++k) {
                    if (graph_init.at(k).unique_id == unique_id) {
                        if (graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::HTTP ||
                            graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::FTP) {
                            if (status != GekkoFyre::DownloadStatus::Paused) {
                                if (status != GekkoFyre::DownloadStatus::Completed) {
                                    if (status != GekkoFyre::DownloadStatus::Stopped) {
                                        if (status != GekkoFyre::DownloadStatus::Failed) {
                                            if (status != GekkoFyre::DownloadStatus::Invalid) {
                                                QObject::connect(this, SIGNAL(sendStopDownload(QString)), GekkoFyre::routine_singleton::instance(), SLOT(recvStopDl(QString)));
                                                emit sendStopDownload(dest);
                                                routines->modifyDlState(dest.toStdString(), GekkoFyre::DownloadStatus::Paused);
                                                dlModel->updateCol(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Paused), MN_STATUS_COL);
                                            }
                                        } else {
                                            QMessageBox::warning(this, tr("Error!"), tr("Please delete the download before re-adding the information once more!"), QMessageBox::Ok);
                                            return;
                                        }
                                    }
                                }
                            }
                        } else if (graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::Torrent) {
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
            for (size_t k = 0; k < graph_init.size(); ++k) {
                if (graph_init.at(k).unique_id == unique_id) {
                    if (graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::HTTP ||
                        graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::FTP) {
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

                                                startDownload(dest, false);
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
                    } else if (graph_init.at(k).down_info.dl_type == GekkoFyre::DownloadType::Torrent) {
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
                            routines->delDownloadItem(file_dest_string);
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

void MainWindow::general_extraDetails()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            const QModelIndex url_index = dlModel->index(indexes.at(0).row(), MN_URL_COL, QModelIndex());
            const QModelIndex dest_index = dlModel->index(indexes.at(0).row(), MN_DESTINATION_COL, QModelIndex());
            const QModelIndex size_index = dlModel->index(indexes.at(0).row(), MN_FILESIZE_COL, QModelIndex());
            const QString url_string = ui->downloadView->model()->data(url_index).toString();
            const QString dest_string = ui->downloadView->model()->data(dest_index).toString();
            const QString size_string = ui->downloadView->model()->data(size_index).toString();
            ui->fileNameDataLabel->setText(QUrl(url_string).fileName());
            ui->destDataLabel->setText(QString::fromStdString(fs::path(dest_string.toStdString()).remove_filename().string()));
            ui->totSizeDataLabel->setText(size_string);
            ui->urlDataLabel->setText(url_string);

            std::vector<GekkoFyre::GkCurl::CurlDlInfo> dl_info = routines->readDownloadInfo();
            long long insert_time = 0;
            long long complt_time = 0;
            double content_length = 0;
            std::string hash_val_given = "";
            std::string hash_val_calc = "";
            QString hashType = "";
            for (size_t i = 0; i < dl_info.size(); ++i) {
                if (dl_info.at(i).file_loc == dest_string.toStdString()) {
                    insert_time = dl_info.at(i).insert_timestamp;
                    complt_time = dl_info.at(i).complt_timestamp;
                    content_length = dl_info.at(i).ext_info.content_length;
                    hash_val_given = dl_info.at(i).hash_val_given;
                    hash_val_calc = dl_info.at(i).hash_val_rtrnd;
                    hashType = routines->convHashType_toString(dl_info.at(i).hash_type);
                    break;
                }
            }

            double curr_dl_amount = 0;
            double curr_dl_speed = 0;
            if (dl_stat.size() > 0) {
                for (size_t i = 0; i < dl_stat.size(); ++i) {
                    if (dl_stat.at(i).file_dest == dest_string.toStdString()) {
                        if (dl_stat.at(i).stat.size() > 0) {
                            curr_dl_amount = (double)dl_stat.at(i).stat.back().dltotal;
                            curr_dl_speed = dl_stat.at(i).stat.back().dlnow;
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
        dl_info.cId = 0;
        dl_info.insert_timestamp = 0;
        dl_info.hash_type = hash_type;
        dl_info.hash_val_given = hash_val;
        dl_info.unique_id = unique_id;
        routines->writeDownloadItem(dl_info);
    }

    return;
}

/**
 * @brief MainWindow::recvXferStats receives the statistics regarding a download.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10-23
 * @note   <http://stackoverflow.com/questions/9086372/how-to-compare-pointers>
 *         <http://doc.qt.io/qt-5/qmutexlocker.html>
 *         <http://stackoverflow.com/questions/19981161/using-qmutexlocker-to-protect-shared-variables-when-running-function-with-qtconc>
 * @param info is the struct related to the download info.
 */
void MainWindow::recvXferStats(const GekkoFyre::GkCurl::CurlProgressPtr &info)
{
    QMutex curl_multi_mutex;
    GekkoFyre::GkCurl::CurlProgressPtr prog_temp;
    curl_multi_mutex.lock();
    prog_temp.stat = info.stat;
    curl_multi_mutex.unlock();
    prog_temp.url = info.url;
    prog_temp.file_dest = info.file_dest;
    prog_temp.content_length = info.content_length;
    prog_temp.timer_set = info.timer_set;

    bool alreadyExists = false;
    for (size_t i = 0; i < dl_stat.size(); ++i) {
        if (dl_stat.at(i).file_dest == prog_temp.file_dest) {
            alreadyExists = true;
            break;
        }
    }

    if (!alreadyExists) {
        // If it doesn't exist, push the whole of 'prog_temp' onto the private, class-global 'dl_stat' and
        // thusly updateDlStats().
        dl_stat.push_back(prog_temp);
        emit updateDlStats();
        return;
    } else {
        // If it does exist, then only update certain elements instead and thusly updateDlStats() also.
        for (size_t i = 0; i < dl_stat.size(); ++i) {
            if (dl_stat.at(i).file_dest == prog_temp.file_dest) {
                dl_stat.at(i).stat = prog_temp.stat;
                emit updateDlStats();
                return;
            }
        }
    }
    return;
}

/**
 * @brief MainWindow::manageDlStats manages the statistics regarding a HTTP/FTP download and updates the relevant columns.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10-23
 * @note   <http://www.qtcentre.org/threads/18082-Hot-to-get-a-QModelIndexList-from-a-model>
 */
void MainWindow::manageDlStats()
{
    for (int i = 0; i < dlModel->getList().size(); ++i) {
        QModelIndex find_index = dlModel->index(i, MN_DESTINATION_COL);
        for (size_t j = 0; j < dl_stat.size(); ++j) {
            if (!dl_stat.at(j).timer_set) {
                std::time(&dl_stat.at(j).timer_begin);
                dl_stat.at(j).timer_set = true;
            }

            QModelIndex unique_id_index = dlModel->index(i, MN_HIDDEN_UNIQUE_ID);
            const QString dest_string = ui->downloadView->model()->data(find_index).toString();
            const QString unique_id_string = ui->downloadView->model()->data(unique_id_index).toString();

            try {
                if (dest_string.toStdString() == dl_stat.at(j).file_dest) {
                    std::vector<GekkoFyre::GkCurl::CurlDlInfo> xml_history_info = routines->readDownloadInfo();
                    if (dl_stat.at(j).content_length == 0) {
                        for (size_t o = 0; o < xml_history_info.size(); ++o) {
                            if (xml_history_info.at(o).file_loc == dl_stat.at(j).file_dest) {
                                dl_stat.at(j).content_length = xml_history_info.at(o).ext_info.content_length;
                                break;
                            }
                        }
                    }

                    GekkoFyre::GkCurl::CurlDlStats dl_stat_element = dl_stat.at(j).stat.back();
                    std::ostringstream oss_dlnow;
                    oss_dlnow << routines->numberConverter(dl_stat_element.dlnow).toStdString() << tr("/sec").toStdString();

                    long cur_file_size = GekkoFyre::CmnRoutines::getFileSize(dl_stat.at(j).file_dest);

                    dlModel->updateCol(dlModel->index(i, MN_DOWNSPEED_COL), QString::fromStdString(oss_dlnow.str()), MN_DOWNSPEED_COL);
                    dlModel->updateCol(dlModel->index(i, MN_DOWNLOADED_COL), routines->numberConverter((double)cur_file_size), MN_DOWNLOADED_COL);
                    dlModel->updateCol(dlModel->index(i, MN_PROGRESS_COL), routines->percentDownloaded(dl_stat.at(j).content_length, (double)cur_file_size), MN_PROGRESS_COL);

                    // Update the 'download speed' spline-graph, via the file location
                    // Append our statistics to the 'graph_init' struct and thus, the graph in question
                    std::time_t cur_time;
                    std::time(&cur_time);
                    double passed_time = std::difftime(cur_time, dl_stat.at(j).timer_begin);

                    for (size_t k = 0; k < graph_init.size(); ++k) {
                        if (graph_init.at(k).unique_id == unique_id_string) {
                            graph_init.at(k).down_speed.down_speed_vals.push_back(std::make_pair(dl_stat_element.dlnow,
                                                                                                 passed_time));
                            break;
                        }
                    }

                    general_extraDetails();
                    transfer_extraDetails();
                    updateChart();
                    break;
                }
            } catch (const std::exception &e) {
                QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
            }
        }
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
                    std::vector<GekkoFyre::GkCurl::CurlDlInfo> dl_mini_info_vec = routines->readDownloadInfo(
                            CFG_HISTORY_FILE, true);
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

                    routines->modifyDlState(status.file_loc, GekkoFyre::DownloadStatus::Completed,
                                            file_hash.checksum.toStdString(),
                                            file_hash.hash_verif, file_hash.hash_type,
                                            QDateTime::currentDateTime().toTime_t());
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
