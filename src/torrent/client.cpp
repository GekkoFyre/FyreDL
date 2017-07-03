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
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <iosfwd>
#include <QString>
#include <QThread>

namespace sys = boost::system;
namespace fs = boost::filesystem;

GekkoFyre::GkTorrentClient::GkTorrentClient()
{
    routines = new GekkoFyre::CmnRoutines();

    gk_ses_thread = new QThread;
    gk_to_ses = new GekkoFyre::GkTorrentSession(lt_ses, this); // Create a session handler
    gk_to_ses->moveToThread(gk_ses_thread);

    QObject::connect(this, SIGNAL(update_ses_hash(std::string,lt::torrent_handle)), gk_to_ses, SLOT(recv_hash_update(std::string,lt::torrent_handle)));
    QObject::connect(gk_to_ses, SIGNAL(sendStats(std::string,lt::torrent_status)), this, SLOT(recv_proc_to_stats(std::string,lt::torrent_status)));
    QObject::connect(gk_to_ses, SIGNAL(finish_gk_ses_thread()), gk_ses_thread, SLOT(quit()));
    QObject::connect(gk_to_ses, SIGNAL(finish_gk_ses_thread()), gk_to_ses, SLOT(deleteLater()));
    QObject::connect(gk_ses_thread, SIGNAL(finished()), gk_ses_thread, SLOT(deleteLater()));
}

GekkoFyre::GkTorrentClient::~GkTorrentClient()
{
    delete routines;
    delete gk_to_ses;
}

/**
 * @brief GekkoFyre::GkTorrentClient::startTorrentDl reads the download item's Unique ID from the XML history file
 * and initiates the BitTorrent session with the appropriate parameters.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-22
 * @param unique_id The Unique ID relating to the download item in question.
 */
void GekkoFyre::GkTorrentClient::startTorrentDl(const GekkoFyre::GkTorrent::TorrentInfo &item)
{
    QString torrent_error_name = QString::fromStdString(item.torrent_name);

    try {
        std::string full_path = item.down_dest + fs::path::preferred_separator + item.torrent_name;
        lt::add_torrent_params atp; // http://libtorrent.org/reference-Core.html#add-torrent-params

        // Load resume data from disk and pass it in as we add the magnet link
        std::ifstream ifs(FYREDL_TORRENT_RESUME_FILE_EXT, std::ios::binary);
        ifs.unsetf(std::ios::skipws);
        atp.resume_data.assign(std::istream_iterator<char>(ifs), std::istream_iterator<char>());
        atp.url = item.magnet_uri;
        atp.save_path = full_path;
        lt_ses->async_add_torrent(atp);

        std::vector<lt::alert*> alerts;
        lt_ses->pop_alerts(&alerts);
        while (alerts.size() > 0) {
            for (lt::alert const *a: alerts) {
                if (auto at = lt::alert_cast<lt::add_torrent_alert>(a)) {
                    if (!gk_ses_thread->isRunning()) {
                        gk_ses_thread->start();
                    }

                    emit update_ses_hash(at->params.save_path, at->handle);
                    goto terminate;
                }
            }
        }

        terminate: ;
        return;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An issue has occured with torrent, \"%1\".\n\n%2")
                .arg(torrent_error_name).arg(e.what()), QMessageBox::Ok);
    }

    return;
}

void GekkoFyre::GkTorrentClient::recv_proc_to_stats(const std::string &save_path,
                                                    const lt::torrent_status &stats)
{
    GekkoFyre::GkTorrentMisc gk_to_misc;
    GekkoFyre::GkTorrent::TorrentResumeInfo to_xfer_stats;
    to_xfer_stats.save_path = save_path;
    to_xfer_stats.dl_state = gk_to_misc.state(stats.state);
    to_xfer_stats.xfer_stats.get().progress_ppm = stats.progress_ppm;
    to_xfer_stats.xfer_stats.get().dl_rate = stats.upload_rate;
    to_xfer_stats.xfer_stats.get().ul_rate = stats.download_rate;
    to_xfer_stats.xfer_stats.get().num_pieces_downloaded = stats.num_pieces;
    to_xfer_stats.last_seen_cmplte = stats.last_seen_complete;
    to_xfer_stats.last_scrape = stats.last_scrape;
    to_xfer_stats.total_downloaded = stats.all_time_download;
    to_xfer_stats.total_uploaded = stats.all_time_upload;

    emit xfer_torrent_info(to_xfer_stats);
    return;
}
