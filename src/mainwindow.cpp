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
#include "singleton_proc.hpp"
#include "curl_easy.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
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

#ifdef _WIN32
#define _WIN32_WINNT 0x06000100
#include <SDKDDKVer.h>
#include <Windows.h>
#endif

namespace fs = boost::filesystem;
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    try {
        #ifdef _WIN32
        HANDLE  m_hStartEvent = CreateEventW(NULL, FALSE, FALSE, L"Global\\FyreDL");
        if (m_hStartEvent == NULL) {
            CloseHandle(m_hStartEvent);
            throw std::runtime_error("Unable to create handle! Not enough memory?");
        }

        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(m_hStartEvent);
            m_hStartEvent = NULL;

            // Already exists!
            // Send message from here to existing copy of application
            throw std::runtime_error("Application is already open!");
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
    QObject::connect(add_url, SIGNAL(sendDetails(std::string,double,int,double,int,int,GekkoFyre::DownloadStatus,std::string,std::string)), this, SLOT(sendDetails(std::string,double,int,double,int,int,GekkoFyre::DownloadStatus,std::string,std::string)));
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
        if (!dl_history.at(i).file_loc.empty() || dl_history.at(i).ext_info.status_ok) {
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
                              const std::string &destination)
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

    // Initialize the graphs
    initCharts(QString::fromStdString(destination));

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
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    int countRow = indexes.count();

    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            try {
                QString url = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString();

                // Now remove the row from the XML file...
                routines->delDownloadItem(url);

                // Remove the associated graph(s) from memory
                delCharts(ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString().toStdString());

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

void MainWindow::initCharts(const QString &file_dest)
{
    if (!file_dest.isEmpty()) {
        GekkoFyre::GkGraph::GraphInit graph;
        graph.down_speed.down_speed_vals.push_back(std::make_pair(0.0, 0.0));
        graph.down_speed.down_speed_init = false;
        graph.file_dest = file_dest;
        graph_init.push_back(graph);
    } else {
        throw std::invalid_argument(tr("'file_dest' is empty!").toStdString());
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
void MainWindow::displayCharts(const QString &file_dest)
{
    for (size_t i = 0; i < graph_init.size(); ++i) {
        if (ui->tabStatusWidget->currentIndex() == TAB_INDEX_GRAPH) {
            if (!graph_init.at(i).down_speed.down_speed_init) {
                // Create the needed 'series'
                graph_init.at(i).down_speed.down_speed_series = new QtCharts::QLineSeries(this);
                graph_init.at(i).down_speed.down_speed_init = true;
            }

            if (graph_init.at(i).file_dest == file_dest) {
                if (curr_shown_graphs != file_dest) {
                    // NOTE: Only clear and/or then show the QChartView once, if 'file_dest' has not changed despite this
                    // function being called.
                    QString prev_shown_graphs = curr_shown_graphs;
                    curr_shown_graphs = file_dest;

                    graph_init.at(i).down_speed.down_speed_series->setName(tr("Download speed spline %1").arg(i));
                    QFile qfile_path(file_dest);

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
                        if (graph_init.at(j).file_dest == prev_shown_graphs) {
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
            if (!graph_init.at(i).file_dest.empty() &&
                    graph_init.at(i).down_speed->down_speed_series != nullptr) {
                if (graph_init.at(i).file_dest == file_dest) {
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
 * @brief MainWindow::askDeleteFile poses a QMessageBox to the user, asking whether they want to delete a pre-existing download
 * before restarting the same one, to the same destination.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-01
 * @param file_dest The destination of the download on the user's local storage.
 * @return Whether the user selected 'yes', or 'no/cancel'.
 */
bool MainWindow::askDeleteFile(const QString &file_dest)
{
    fs::path dest_boost_path(file_dest.toStdString());
    if (fs::exists(dest_boost_path)) {
        QMessageBox file_ask;
        file_ask.setWindowTitle(tr("Pre-existing file!"));
        file_ask.setText(tr("Do you want to remove the file and then restart the download?").arg(file_dest));
        file_ask.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        file_ask.setDefaultButton(QMessageBox::No);
        file_ask.setModal(false);
        int ret = file_ask.exec();

        switch (ret) {
            case QMessageBox::Yes:
                try {
                    fs::remove(dest_boost_path);
                    return true;
                } catch (const fs::filesystem_error &e) {
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

            if (status != GekkoFyre::DownloadStatus::Completed) {
                try {
                    // TODO: QFutureWatcher<GekkoFyre::CurlMulti::CurlInfo> *verifyFileFutWatch;
                    GekkoFyre::GkCurl::CurlInfo verify = GekkoFyre::CurlEasy::verifyFileExists(url);
                    if (verify.response_code == 200) {
                        if (status != GekkoFyre::DownloadStatus::Downloading) {
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
                        }
                    }
                } catch (const std::exception &e) {
                    QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                }
            }
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

/**
 * @brief MainWindow::on_dlstartToolBtn_clicked
 * @note <https://solarianprogrammer.com/2012/10/17/cpp-11-async-tutorial/>
 *       <http://doc.qt.io/qt-5/qthreadpool.html>
 *       <http://doc.qt.io/qt-5/qthread.html>
 *       <http://en.cppreference.com/w/cpp/thread/packaged_task>
 */
void MainWindow::on_dlstartToolBtn_clicked()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            const QString dest = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();
            fs::path dest_boost_path(dest.toStdString());

            if (fs::exists(dest_boost_path)) {
                QMessageBox file_ask;
                file_ask.setWindowTitle(tr("Pre-existing file!"));
                file_ask.setText(tr("The file, \"%1\", already exists. Do you want to resume the download?").arg(dest));
                file_ask.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                file_ask.setDefaultButton(QMessageBox::No);
                file_ask.setModal(false);
                int ret = file_ask.exec();

                try {
                    switch (ret) {
                        case QMessageBox::Yes:
                            startDownload(dest, true);
                            return;
                        case QMessageBox::No:
                            askDeleteFile(dest);
                            startDownload(dest, false);
                            return;
                        case QMessageBox::Cancel:
                            askDeleteFile(dest);
                            startDownload(dest, false);
                            return;
                    }
                } catch (const std::exception &e) {
                    QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                }
            } else {
                try {
                    startDownload(dest, false);
                    return;
                } catch (const std::exception &e) {
                    QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                }
            }
        }
    }

    return;
}

void MainWindow::on_dlpauseToolBtn_clicked()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();

    if (indexes.size() > 0) {
        if (indexes.at(0).isValid()) {
            try {
                const QString url = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_URL_COL)).toString();
                const QString dest = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();

                QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
                const GekkoFyre::DownloadStatus status = routines->convDlStat_StringToEnum(ui->downloadView->model()->data(index).toString());
                if (status != GekkoFyre::DownloadStatus::Paused && status != GekkoFyre::DownloadStatus::Completed) {
                    routines->modifyDlState(dest.toStdString(), GekkoFyre::DownloadStatus::Paused);
                    dlModel->updateCol(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Paused), MN_STATUS_COL);
                }
            } catch (const std::exception &e) {
                QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
                return;
            }
        }
    }
}

void MainWindow::on_dlstopToolBtn_clicked()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
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
            const QString dest = ui->downloadView->model()->data(ui->downloadView->model()->index(indexes.at(0).row(), MN_DESTINATION_COL)).toString();

            QModelIndex index = dlModel->index(indexes.at(0).row(), MN_STATUS_COL, QModelIndex());
            const GekkoFyre::DownloadStatus status = routines->convDlStat_StringToEnum(ui->downloadView->model()->data(index).toString());
            QObject::connect(this, SIGNAL(sendStopDownload(QString)), GekkoFyre::routine_singleton::instance(), SLOT(recvStopDl(QString)));
            if (status != GekkoFyre::DownloadStatus::Stopped && status != GekkoFyre::DownloadStatus::Completed) {
                emit sendStopDownload(dest);
                routines->modifyDlState(dest.toStdString(), GekkoFyre::DownloadStatus::Stopped);
                dlModel->updateCol(index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Stopped), MN_STATUS_COL);
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

/**
 * @brief MainWindow::on_clearhistoryToolBtn_clicked will clear any completed downloads from the QTableView, 'downloadView',
 * and from the corresponding XML history file.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @note  <http://stackoverflow.com/questions/8613737/how-can-i-remove-elements-from-a-qlist-while-iterating-over-it-using-foreach>
 */
void MainWindow::on_clearhistoryToolBtn_clicked()
{
    QMessageBox file_ask;
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
                QModelIndex url_index = dlModel->index(i, MN_URL_COL);
                QModelIndex file_dest_index = dlModel->index(i, MN_DESTINATION_COL);
                const QString url_string = ui->downloadView->model()->data(url_index).toString();

                try {
                    delCharts(ui->downloadView->model()->data(file_dest_index).toString().toStdString());
                    routines->delDownloadItem(url_string);
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

void MainWindow::on_settingsToolBtn_clicked()
{
    Settings *settingsUi = new Settings(this);
    settingsUi->setAttribute(Qt::WA_DeleteOnClose, true);
    settingsUi->open();
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

void MainWindow::keyUpDlModelSlot()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    int row = indexes.at(0).row();
    --row;
    ui->downloadView->selectRow(row);

    updateChart();
}

void MainWindow::keyDownDlModelSlot()
{
    QModelIndexList indexes = ui->downloadView->selectionModel()->selectedRows();
    int row = indexes.at(0).row();
    ++row;
    ui->downloadView->selectRow(row);

    updateChart();
}

void MainWindow::sendDetails(const std::string &fileName, const double &fileSize, const int &downloaded,
                             const double &progress, const int &upSpeed, const int &downSpeed,
                             const GekkoFyre::DownloadStatus &status, const std::string &url,
                             const std::string &destination)
{
    QList<std::vector<QString>> list = dlModel->getList();
    std::vector<QString> fileNameVector;
    fileNameVector.push_back(QString::fromStdString(fileName));
    if (!list.contains(fileNameVector) && !fileNameVector.empty()) {
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
 * @brief MainWindow::manageDlStats manages the statistics regarding a download and updates the relevant columns.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10-23
 * @note   <http://www.qtcentre.org/threads/18082-Hot-to-get-a-QModelIndexList-from-a-model>
 */
void MainWindow::manageDlStats()
{
    // TODO: Have FyreDL immediately stop this routine, MainWindow::manageDlStats(), after a download is completed.
    for (int i = 0; i < dlModel->getList().size(); ++i) {
        QModelIndex find_index = dlModel->index(i, MN_DESTINATION_COL);
        for (size_t j = 0; j < dl_stat.size(); ++j) {
            if (!dl_stat.at(j).timer_set) {
                std::time(&dl_stat.at(j).timer_begin);
                dl_stat.at(j).timer_set = true;
            }

            if (ui->downloadView->model()->data(find_index).toString().toStdString() == dl_stat.at(j).file_dest && !dl_stat.at(j).dl_complete) {
                try {
                    GekkoFyre::GkCurl::CurlDlStats dl_stat_element = dl_stat.at(j).stat.back();
                    std::ostringstream oss_dlnow;
                    oss_dlnow << routines->numberConverter(dl_stat_element.dlnow).toStdString() << tr("/sec").toStdString();

                    dlModel->updateCol(dlModel->index(i, MN_DOWNSPEED_COL), QString::fromStdString(oss_dlnow.str()), MN_DOWNSPEED_COL);
                    dlModel->updateCol(dlModel->index(i, MN_DOWNLOADED_COL), routines->numberConverter(dl_stat_element.dltotal), MN_DOWNLOADED_COL);

                    // Update the 'download speed' spline-graph, via the file location
                    if (!graph_init.empty()) {
                        for (size_t k = 0; k < graph_init.size(); ++k) {

                            // Verify that the constants we are going to use are not empty/nullptr
                            if (!graph_init.at(k).file_dest.isEmpty() &&
                                    !dl_stat.at(j).file_dest.empty()) {

                                // Make sure we are using the right 'graph_init' element!
                                if (graph_init.at(k).file_dest.toStdString() == dl_stat.at(j).file_dest) {
                                    // TODO: Fix the SIGSEGV bug relating to the code immediately below! NOTE: It only occurs some of the time.
                                    // Append our statistics to the 'graph_init' struct and thus, the graph in question
                                    std::time_t cur_time;
                                    std::time(&cur_time);
                                    double passed_time = std::difftime(cur_time, dl_stat.at(j).timer_begin);
                                    graph_init.at(k).down_speed.down_speed_vals.push_back(std::make_pair(dl_stat_element.dlnow,
                                                                                                         passed_time));
                                    updateChart();
                                }
                            } else {
                                throw std::invalid_argument(tr("Invalid file destination and/or graphing pointer(s) have been given!").toStdString());
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

            if (url == status.url) {
                if (dest.toStdString() == status.file_loc) {
                    QModelIndex stat_index = dlModel->index(i, MN_STATUS_COL);
                    if (routines->convDlStat_StringToEnum(ui->downloadView->model()->data(stat_index).toString()) == GekkoFyre::DownloadStatus::Downloading) {
                        GekkoFyre::GkFile::FileHash file_hash;
                        std::vector<GekkoFyre::GkCurl::CurlDlInfo> dl_mini_info_vec = routines->readDownloadInfo(CFG_HISTORY_FILE, true);
                        for (size_t j = 0; j < dl_mini_info_vec.size(); ++j) {
                            if (dl_mini_info_vec.at(j).file_loc == status.file_loc && dl_mini_info_vec.at(j).ext_info.effective_url == status.url.toStdString()) {
                                file_hash = routines->cryptoFileHash(
                                        QString::fromStdString(status.file_loc), dl_mini_info_vec.at(j).hash_type,
                                        QString::fromStdString(dl_mini_info_vec.at(j).hash_val_given));
                                break;
                            }
                        }

                        for (size_t j = 0; j < dl_stat.size(); ++j) {
                            if (status.file_loc == dl_stat.at(j).file_dest) {
                                dl_stat.at(j).dl_complete = true;
                                break;
                            }
                        }

                        routines->modifyDlState(status.file_loc, GekkoFyre::DownloadStatus::Completed, file_hash.checksum.toStdString(),
                                                file_hash.hash_verif);
                        dlModel->updateCol(stat_index, routines->convDlStat_toString(GekkoFyre::DownloadStatus::Completed), MN_STATUS_COL);

                        // Update the 'downloaded amount' because the statistics routines are not always accurate, due to only
                        // running every few seconds at most. This causes inconsistencies.
                        dlModel->updateCol(dlModel->index(i, MN_DOWNLOADED_COL), routines->numberConverter(status.content_len), MN_DOWNLOADED_COL);
                        return;
                    }
                }

                // There is a bug where every second, third, fourth, etc. download will skip the status check to see if the
                // entry-row in question is 'Downloading' and thus not hit the return in that code bracket, thereby ending up at
                // this exception. Disabling this exception temporarily fixes this problem at the cost of performance for many file
                // downloads within the one session.
                // TODO: Fix the aforementioned problem!

                /*
                throw std::invalid_argument(tr("Error with completing the download: %1")
                                                    .arg(QString::fromStdString(oss_dest.str())).toStdString());
                                                    */
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Error!"), QString("%1\n\nAborting...").arg(e.what()), QMessageBox::Ok);
        QCoreApplication::exit(-1); // Terminate application with return code '-1'.
    }

    return;
}
