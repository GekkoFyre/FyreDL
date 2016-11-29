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
 * @file mainwindow.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief The code and functions behind the, 'mainwindow.ui', designer file.
 */

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "default_var.hpp"
#include "dl_view.hpp"
#include "cmnroutines.hpp"
#include "curl_multi.hpp"
#include "addurl.hpp"
#include <vector>
#include <QMainWindow>
#include <QString>
#include <QtConcurrent/QtConcurrent>
#include <QThread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void addDownload();
    void readFromHistoryFile();
    void modifyHistoryFile();
    void insertNewRow(const std::string &fileName, const double &fileSize, const int &downloaded,
                      const double &progress, const int &upSpeed, const int &downSpeed,
                      const GekkoFyre::DownloadStatus &status, const std::string &url,
                      const std::string &destination);
    void removeSelRows();

    void initCharts(const QString &file_dest);
    void displayCharts(const QString &file_dest);
    void delCharts(const std::string &file_dest);
    void updateChart();

    downloadModel *dlModel;
    GekkoFyre::CmnRoutines *routines;
    GekkoFyre::CurlMulti *curl_multi;
    GekkoFyre::DownloadStatus status;
    std::vector<GekkoFyre::GkCurl::CurlProgressPtr> dl_stat;
    std::vector<GekkoFyre::GkGraph::GraphInit> graph_init;
    QString curr_shown_graphs;

    // http://stackoverflow.com/questions/10121560/stdthread-naming-your-thread
    QThread *curl_multi_thread;

signals:
    void updateDlStats();
    void sendStopDownload(const QString &fileLoc);
    void sendStartDownload(const QString &url, const QString &file_loc);
    void finish_curl_multi_thread();

private slots:
    void on_action_Open_a_File_triggered();
    void on_actionEnter_URL_triggered();
    void on_actionE_xit_triggered();
    void on_action_About_triggered();
    void on_action_Documentation_triggered();
    void on_addurlToolBtn_clicked();
    void on_addfileToolBtn_clicked();
    void on_printToolBtn_clicked();
    void on_dlstartToolBtn_clicked();
    void on_dlpauseToolBtn_clicked();
    void on_dlstopToolBtn_clicked();
    void on_removeToolBtn_clicked();
    void on_clearhistoryToolBtn_clicked();
    void on_settingsToolBtn_clicked();
    void on_downloadView_customContextMenuRequested(const QPoint &pos);
    void keyUpDlModelSlot();
    void keyDownDlModelSlot();
    void sendDetails(const std::string &fileName, const double &fileSize, const int &downloaded,
                     const double &progress, const int &upSpeed, const int &downSpeed,
                     const GekkoFyre::DownloadStatus &status, const std::string &url,
                     const std::string &destination);
    void recvXferStats(const GekkoFyre::GkCurl::CurlProgressPtr &info);
    void manageDlStats();
    void recvDlFinished(const GekkoFyre::GkCurl::DlStatusMsg &status);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_HPP
