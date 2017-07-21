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
#include <libtorrent/alert_types.hpp>
#include <libtorrent/fingerprint.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/bencode.hpp>
#include <fstream>
#include <random>
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>
#include <QMessageBox>

using clk = std::chrono::steady_clock;

GekkoFyre::GkTorrentSession::GkTorrentSession(QObject *parent) : QObject(parent)
{
    thread_terminate = false;
    QObject::connect(this, SIGNAL(finish_gk_ses_thread()), this, SLOT(finish_thread_cleanup()));
}

GekkoFyre::GkTorrentSession::GkTorrentSession(std::shared_ptr<lt::session_handle> lt_ses, QObject *parent)
{
    thread_terminate = false;
    if (lt_ses->is_valid()) {
        gk_lt_ses = lt_ses;
    }

    QObject::connect(this, SIGNAL(finish_gk_ses_thread()), this, SLOT(finish_thread_cleanup()));
}

GekkoFyre::GkTorrentSession::~GkTorrentSession()
{}

/**
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-06-29
 */
void GekkoFyre::GkTorrentSession::run_session_bckgrnd()
{
    try {
        clk::time_point last_save_resume = clk::now();

        if (gk_lt_ses->is_valid()) {
            // This is the handle we'll set once we get the notification of it being added
            for (;;) {
                reset: ;

                if (thread_terminate) { // No more torrents left, so end session!
                    goto done;
                }

                std::vector<lt::alert*> alerts;
                alerts.clear();
                gk_lt_ses->pop_alerts(&alerts);
                for (lt::alert const *a: alerts) {
                    // If we receive the finished alert or an error, we're done
                    if (auto at = lt::alert_cast<lt::torrent_finished_alert>(a)) {
                        at->handle.save_resume_data();
                        lt_to_handle.remove(at->handle.status().save_path);
                        goto reset;
                    }

                    if (lt::alert_cast<lt::torrent_error_alert>(a)) {
                        std::cout << a->message() << std::endl;
                        goto reset;
                    }

                    // When resume data is ready, save it
                    if (auto rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
                        std::ofstream of(FYREDL_TORRENT_RESUME_FILE_EXT, std::ios::binary);
                        of.unsetf(std::ios_base::skipws);
                        lt::bencode(std::ostream_iterator<char>(of), *rd->resume_data);
                    }

                    if (auto st = lt::alert_cast<lt::state_update_alert>(a)) {
                        if (st->status.empty()) continue;
                        int p = 0;
                        QMap<std::string, lt::torrent_handle>::iterator i;
                        for (i = lt_to_handle.begin(); i != lt_to_handle.end(); ++i) {
                            ++p;
                            const lt::torrent_status &s = st->status[p];
                            emit sendStats(i.value().status().save_path, s);
                        }
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                // Ask the session to post a state_update_alert, to update our state output for the torrent
                gk_lt_ses->post_torrent_updates();

                // Save resume data once every 30 seconds
                if (clk::now() - last_save_resume > std::chrono::seconds(30)) {
                    QMap<std::string, lt::torrent_handle>::iterator i;
                    for (i = lt_to_handle.begin(); i != lt_to_handle.end(); ++i) {
                        i.value().save_resume_data();
                    }

                    last_save_resume = clk::now();
                }
            }
        } else {
            throw std::runtime_error(tr("Unable to initialize a BitTorrent session! Please check your settings and try "
                                                "again.").toStdString());
        }

        done: ;
        emit finish_gk_ses_thread();
        return;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return;
    }
}

void GekkoFyre::GkTorrentSession::recv_hash_update(const std::string &save_dir, const lt::torrent_handle &lt_at)
{
    if (!lt_to_handle.contains(save_dir)) {
        lt_to_handle.insert(save_dir, lt_at);
    }
}

void GekkoFyre::GkTorrentSession::finish_thread_cleanup()
{
    thread_terminate = true;
    return;
}
