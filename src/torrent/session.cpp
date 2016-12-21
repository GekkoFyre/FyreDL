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
 * @file session.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-22
 * @brief Contains the routines for managing BitTorrent sessions.
 */

#include "session.hpp"
#include "./../cmnroutines.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bencode.hpp>
#include <chrono>
#include <fstream>
#include <thread>
#include <iostream>

using clk = std::chrono::steady_clock;
namespace sys = boost::system;
namespace fs = boost::filesystem;
GekkoFyre::GkTorrentSession::GkTorrentSession()
{}

GekkoFyre::GkTorrentSession::~GkTorrentSession()
{}

/**
 * @brief GekkoFyre::GkTorrentSession::state returns the name of a libtorrent status enum.
 * @date 2016-12-22
 * @note <http://libtorrent.org/tutorial.html>
 * @param s is the given libtorrent status enum.
 * @return The name of a libtorrent status enum.
 */
QString GekkoFyre::GkTorrentSession::state(lt::torrent_status::state_t s)
{
    switch (s) {
        case lt::torrent_status::checking_files:
            return tr("Checking");
        case lt::torrent_status::downloading_metadata:
            return tr("Downloading Metadata");
        case lt::torrent_status::downloading:
            return tr("Downloading");
        case lt::torrent_status::finished:
            return tr("Finished");
        case lt::torrent_status::seeding:
            return tr("Seeding");
        case lt::torrent_status::allocating:
            return tr("Allocating");
        case lt::torrent_status::checking_resume_data:
            return tr("Checking Resume Data");
        default:
            return QString("<N/A>");
    }
}

/**
 * @brief GekkoFyre::GkTorrentSession::startTorrentDl reads the download item's Unique ID from the XML history file
 * and initiates the BitTorrent session with the appropriate parameters.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-22
 * @param unique_id The Unique ID relating to the download item in question.
 */
void GekkoFyre::GkTorrentSession::startTorrentDl(const std::string &unique_id, const std::string &xmlHistoryFile)
{
    QString torrent_error_name = tr("<N/A>");
    try {
        std::unique_ptr<GekkoFyre::CmnRoutines> routines;
        routines = std::make_unique<GekkoFyre::CmnRoutines>();
        std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_ti = routines->readTorrentInfo(false, xmlHistoryFile);

        for (size_t i = 0; i < gk_ti.size(); ++i) {
            GekkoFyre::GkTorrent::TorrentInfo indice = gk_ti.at(i);
            if (unique_id == indice.unique_id) {
                torrent_error_name = QString::fromStdString(indice.torrent_name);
                std::string full_path = (indice.down_dest + fs::path::preferred_separator + indice.torrent_name);
                init_session(indice.magnet_uri, full_path);
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An issue has occured with torrent, \"%1\".\n\n%2")
                .arg(torrent_error_name).arg(e.what()), QMessageBox::Ok);
    }
}

/**
 * @brief GekkoFyre::GkTorrentSession::init_session initiates a BitTorrent session with all the required parameters.
 * @date 2016-12-22
 * @note <http://libtorrent.org/tutorial.html>
 * @param magnet_uri is the 'Magnet URL' of the BitTorrent in question that's being downloaded.
 */
void GekkoFyre::GkTorrentSession::init_session(const std::string &magnet_uri, const std::string &destination)
{
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::alert_mask, lt::alert::error_notification | lt::alert::storage_notification |
            lt::alert::status_notification);

    lt::session ses(pack);
    lt::add_torrent_params atp; // http://libtorrent.org/reference-Core.html#add-torrent-params
    clk::time_point last_save_resume = clk::now();

    // Load resume data from disk and pass it in as we add the magnet link
    std::ifstream ifs(FYREDL_TORRENT_RESUME_FILE_EXT, std::ios::binary);
    ifs.unsetf(std::ios::skipws);
    atp.resume_data.assign(std::istream_iterator<char>(ifs), std::istream_iterator<char>());
    atp.url = magnet_uri;
    atp.save_path = destination;
    ses.async_add_torrent(atp);

    // This is the handle we'll set once we get the notification of it being added
    lt::torrent_handle h;
    for (;;) {
        std::vector<lt::alert*> alerts;
        ses.pop_alerts(&alerts);

        for (lt::alert const *a: alerts) {
            if (auto at = lt::alert_cast<lt::add_torrent_alert>(a)) {
                h = at->handle;
            }

            // If we receive the finished alert or an error, we're done
            if (lt::alert_cast<lt::torrent_finished_alert>(a)) {
                h.save_resume_data();
                goto done;
            }

            if (lt::alert_cast<lt::torrent_error_alert>(a)) {
                std::cout << a->message() << std::endl;
                goto done;
            }

            // When resume data is ready, save it
            if (auto rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
                std::ofstream of(FYREDL_TORRENT_RESUME_FILE_EXT, std::ios::binary);
                of.unsetf(std::ios_base::skipws);
                lt::bencode(std::ostream_iterator<char>(of), *rd->resume_data);
            }

            if (auto st = lt::alert_cast<lt::state_update_alert>(a)) {
                if (st->status.empty()) continue;

                // We only have a single torrent, so we know which one the status is for
                // TODO: Enter status code here
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Ask the session to post a state_update_alert, to update our state output for the torrent
        ses.post_torrent_updates();

        // Save resume data once every 30 seconds
        if (clk::now() - last_save_resume > std::chrono::seconds(30)) {
            h.save_resume_data();
            last_save_resume = clk::now();
        }
    }

    // TODO: Ideally we should save the resume data here

    done: ;
}
