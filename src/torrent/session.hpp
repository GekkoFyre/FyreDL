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
#include <libtorrent/torrent_status.hpp>
#include <QObject>
#include <QString>

namespace lt = libtorrent;
namespace GekkoFyre {
class GkTorrentSession: public QObject {
    Q_OBJECT

public:
    GkTorrentSession();
    ~GkTorrentSession();

    QString state(lt::torrent_status::state_t s);
    void startTorrentDl(const std::string &unique_id, const std::string &xmlHistoryFile = CFG_HISTORY_FILE);

private:
    void init_session(const std::string &magnet_uri, const std::string &destination);
};
}

#endif // FYREDL_TORRENT_SESSION_HPP