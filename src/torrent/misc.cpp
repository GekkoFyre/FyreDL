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
 * @file misc.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-06-24
 * @brief Contains any miscellaneous BitTorrent routines.
 */

#include "misc.hpp"
#include <libtorrent/torrent_status.hpp>

GekkoFyre::GkTorrentMisc::GkTorrentMisc()
{}

GekkoFyre::GkTorrentMisc::~GkTorrentMisc()
{}

/**
 * @brief GekkoFyre::GkTorrentMisc::state returns the name of a libtorrent status enum.
 * @date 2016-12-22
 * @note <http://libtorrent.org/tutorial.html>
 * @param s is the given libtorrent status enum.
 * @return The name of a libtorrent status enum.
 */
QString GekkoFyre::GkTorrentMisc::state(lt::torrent_status::state_t s)
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
