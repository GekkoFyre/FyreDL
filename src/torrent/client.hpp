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
 * @file client.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-13
 * @note <http://www.rasterbar.com/products/libtorrent/reference.html>
 *       <http://www.rasterbar.com/products/libtorrent/manual.html>
 *       <http://stackoverflow.com/questions/13953086/download-specific-piece-using-libtorrent>
 * @brief Contains the routines for downloading (and directly managing therof) any torrents, asynchronously.
 */

#ifndef FYREDL_TORRENT_CLIENT_HPP
#define FYREDL_TORRENT_CLIENT_HPP

#include "./../default_var.hpp"
#include "./../cmnroutines.hpp"
#include "misc.hpp"
#include <libtorrent/session_handle.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <string>
#include <memory>
#include <future>
#include <QObject>
#include <QPointer>
#include <QMap>

namespace GekkoFyre {
class GkTorrentClient: public QObject {
    Q_OBJECT

public:
    GkTorrentClient(const GekkoFyre::GkFile::FileDb &database, QObject *parent = 0);
    ~GkTorrentClient();

    void startTorrentDl(const GekkoFyre::GkTorrent::TorrentInfo &item);

private:
    void run_session_bckgrnd();

    std::shared_ptr<GekkoFyre::CmnRoutines> routines;
    lt::session_handle *lt_ses;
    QMap<std::string, lt::torrent_handle> lt_to_handle;
    QMap<std::string, std::string> unique_id_cache;
    std::future<void> async_ses;
    GekkoFyre::GkFile::FileDb db_struct;

private slots:
    void recv_proc_to_stats(const std::string &save_path, const lt::torrent_status &stats);

signals:
    void xfer_torrent_info(const GekkoFyre::GkTorrent::TorrentResumeInfo &xfer_stats);
    void xfer_internal_stats(const std::string &save_path, const lt::torrent_status &stats);
};
}

// This is required for signaling, otherwise QVariant does not know the type.
Q_DECLARE_METATYPE(lt::torrent_status);

#endif // FYREDL_TORRENT_CLIENT_HPP