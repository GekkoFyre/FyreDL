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
 * @file session.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-22
 * @brief Contains the routines for managing BitTorrent sessions.
 */

#ifndef FYREDL_TORRENT_SESSION_HPP
#define FYREDL_TORRENT_SESSION_HPP

#include "./../default_var.hpp"
#include <libtorrent/session_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <string>
#include <memory>
#include <QObject>
#include <QMap>

namespace lt = libtorrent;
namespace GekkoFyre {
class GkTorrentSession: public QObject {
    Q_OBJECT

public:
    GkTorrentSession(QObject *parent = 0);
    GkTorrentSession(std::shared_ptr<lt::session_handle> lt_ses, QObject *parent = 0);
    ~GkTorrentSession();

private:
    QMap<std::string, lt::torrent_handle> lt_to_handle;
    std::shared_ptr<lt::session_handle> gk_lt_ses;
    bool thread_terminate;

public slots:
    void run_session_bckgrnd();
    void recv_hash_update(const std::string &save_dir, const lt::torrent_handle &lt_at);

private slots:
    void finish_thread_cleanup();

signals:
    void sendStats(const std::string &save_path, const lt::torrent_status &stats);
    void finish_gk_ses_thread();
};
}

#endif // FYREDL_TORRENT_SESSION_HPP