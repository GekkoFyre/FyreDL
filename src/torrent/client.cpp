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
 * @file client.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-13
 * @note <http://www.rasterbar.com/products/libtorrent/reference.html>
 *       <http://www.rasterbar.com/products/libtorrent/manual.html>
 *       <http://stackoverflow.com/questions/13953086/download-specific-piece-using-libtorrent>
 * @brief Contains the routines for downloading (and directly managing therof) any torrents, asynchronously.
 */

#include "client.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>

namespace sys = boost::system;
namespace fs = boost::filesystem;
GekkoFyre::GkTorrentClient::GkTorrentClient()
{
    routines = new GekkoFyre::CmnRoutines();
    gk_torrent_session = new GekkoFyre::GkTorrentSession();

    QObject::connect(gk_torrent_session, SIGNAL(sendStats(std::string,lt::torrent_status)), this,
                     SLOT(recvRawStats(std::string,lt::torrent_status)));
}

GekkoFyre::GkTorrentClient::~GkTorrentClient()
{
    delete routines;
    delete gk_torrent_session;
}

/**
 * @brief GekkoFyre::GkTorrentClient::startTorrentDl reads the download item's Unique ID from the XML history file
 * and initiates the BitTorrent session with the appropriate parameters.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-22
 * @param unique_id The Unique ID relating to the download item in question.
 */
void GekkoFyre::GkTorrentClient::startTorrentDl(const std::string &unique_id, const std::string &xmlHistoryFile)
{
    QString torrent_error_name = tr("<N/A>");
    try {
        std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_ti = routines->readTorrentInfo(false, xmlHistoryFile);

        for (size_t i = 0; i < gk_ti.size(); ++i) {
            GekkoFyre::GkTorrent::TorrentInfo indice = gk_ti.at(i);
            if (unique_id == indice.unique_id) {
                torrent_error_name = QString::fromStdString(indice.torrent_name);
                std::string full_path = (indice.down_dest + fs::path::preferred_separator + indice.torrent_name);

                gk_torrent_session->init_session(indice.magnet_uri, unique_id, full_path);
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An issue has occured with torrent, \"%1\".\n\n%2")
                .arg(torrent_error_name).arg(e.what()), QMessageBox::Ok);
    }
}

void GekkoFyre::GkTorrentClient::recvRawStats(const std::string &unique_id, const lt::torrent_status &stats)
{
    GekkoFyre::GkTorrent::TorrentXferStats gk_xfer_stats;
    gk_xfer_stats.unique_id = unique_id;
    gk_xfer_stats.progress_ppm = stats.progress_ppm;
    gk_xfer_stats.total_done = stats.total_done;
    gk_xfer_stats.dl_payload_rate = stats.download_payload_rate;
    gk_xfer_stats.dl_state = gk_torrent_session->state(stats.state);
    emit sendProcTorrentStats(gk_xfer_stats);
}
