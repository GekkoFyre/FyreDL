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
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bencode.hpp>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <QString>
#include <random>

namespace sys = boost::system;
namespace fs = boost::filesystem;

using clk = std::chrono::steady_clock;

/**
 * @note <http://www.rasterbar.com/products/libtorrent/manual.html>
 *       <http://libtorrent.org/tutorial.html>
 *       <http://libtorrent.org/reference-Core.html#save_resume_data()>
 *       <http://libtorrent.org/reference-Core.html#session_handle>
 */
GekkoFyre::GkTorrentClient::GkTorrentClient(QObject *parent) : QObject(parent)
{
    routines = std::make_shared<GekkoFyre::CmnRoutines>();
    async_active = false;

    // http://libtorrent.org/reference-Settings.html#settings_pack
    // http://www.libtorrent.org/include/libtorrent/session_settings.hpp
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::alert_mask, lt::alert::error_notification | lt::alert::storage_notification |
                                                lt::alert::status_notification);
    pack.set_str(lt::settings_pack::user_agent, FYREDL_USER_AGENT);         // This is the client identification to the tracker.
    pack.set_str(lt::settings_pack::peer_fingerprint, FYREDL_FINGERPRINT);  // This is the fingerprint for the client. It will be used as the prefix to the peer_id. If this is 20 bytes (or longer) it will be truncated at 20 bytes and used as the entire peer-id.
    pack.set_str(lt::settings_pack::handshake_client_version, "");          // This is the client name and version identifier sent to peers in the handshake message. If this is an empty string, the user_agent is used instead.

    pack.set_bool(lt::settings_pack::rate_limit_ip_overhead, true);         // If set to true, the estimated TCP/IP overhead is drained from the rate limiters, to avoid exceeding the limits with the total traffic.
    pack.set_bool(lt::settings_pack::prefer_udp_trackers, true);            // It means that trackers may be rearranged in a way that udp trackers are always tried before http trackers for the same hostname.
    pack.set_bool(lt::settings_pack::announce_crypto_support, true);        // When this is true, and incoming encrypted connections are enabled, &supportcrypt=1 is included in http tracker announces.
    pack.set_bool(lt::settings_pack::enable_upnp, true);                    // Starts and stops the UPnP service.
    pack.set_bool(lt::settings_pack::enable_natpmp, true);                  // Starts and stops the NAT-PMP service.
    pack.set_bool(lt::settings_pack::enable_dht, true);                     // Starts the dht node and makes the trackerless service available to torrents.
    pack.set_bool(lt::settings_pack::prefer_rc4, true);                     // If the allowed encryption level is both, setting this to true will prefer rc4 if both methods are offered, plaintext otherwise.

    pack.set_int(lt::settings_pack::connection_speed, 10);                  // The number of connection attempts that are made per second.
    pack.set_int(lt::settings_pack::handshake_timeout, 30);                 // The number of seconds to wait for a handshake response from a peer.
    pack.set_int(lt::settings_pack::download_rate_limit, 0);                // Sets the session-global limits of upload and download rate limits, in bytes per second. By default peers on the local network are not rate limited.
    pack.set_int(lt::settings_pack::upload_rate_limit, 0);                  // Sets the session-global limits of upload and download rate limits, in bytes per second. By default peers on the local network are not rate limited.
    pack.set_int(lt::settings_pack::dht_upload_rate_limit, 4000);           // Sets the rate limit on the DHT. This is specified in bytes per second and defaults to 4000. For busy boxes with lots of torrents that requires more DHT traffic, this should be raised.
    pack.set_int(lt::settings_pack::connections_limit, 500);                // Sets a global limit on the number of connections opened.
    pack.set_int(lt::settings_pack::connections_slack, 10);                 // The number of incoming connections exceeding the connection limit to accept in order to potentially replace existing ones.
    pack.set_int(lt::settings_pack::half_open_limit, -1);
    pack.set_int(lt::settings_pack::ssl_listen, 4433);                      // Sets the listen port for SSL connections. This setting is only taken into account when opening the regular listen port, and won't re-open the listen socket simply by changing this setting.
    std::string interface = std::string("0.0.0.0:" + 0);
    pack.set_str(lt::settings_pack::listen_interfaces, interface);          // Binding to port 0 will make the operating system pick the port. The default is "0.0.0.0:6881", which binds to all interfaces on port 6881. Once/if binding the listen socket(s) succeed, listen_succeeded_alert is posted.

    lt_ses = new libtorrent::session(pack);

    // http://doc.qt.io/qt-4.8/threads-qobject.html#signals-and-slots-across-threads
    // http://wiki.qt.io/Threads_Events_QObjects
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<lt::torrent_status>("lt::torrent_status");
    QObject::connect(this, SIGNAL(xfer_internal_stats(std::string,lt::torrent_status)), this,
                     SLOT(recv_proc_to_stats(std::string,lt::torrent_status)), Qt::AutoConnection);
}

GekkoFyre::GkTorrentClient::~GkTorrentClient()
{}

/**
 * @brief GekkoFyre::GkTorrentClient::startTorrentDl reads the given download destination to the user's local storage from
 * the Google LevelDB database and initiates a BitTorrent download of the item in question.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-22
 * @param item The downloadable item in question to get from the BitTorrent P2P network.
 */
void GekkoFyre::GkTorrentClient::startTorrentDl(const GekkoFyre::GkTorrent::TorrentInfo &item)
{
    QString torrent_error_name = QString::fromStdString(item.general.torrent_name);

    try {
        fs::path tor_dest = item.general.down_dest;
        lt::add_torrent_params atp; // http://libtorrent.org/reference-Core.html#add-torrent-params

        // Load resume data from disk and pass it in as we add the magnet link
        std::string resume_file = std::string(tor_dest.string() + item.general.torrent_name + FYREDL_TORRENT_RESUME_FILE_EXT);
        std::ifstream ifs(resume_file, std::ios::binary);
        ifs.unsetf(std::ios::skipws);

        atp.resume_data.assign(std::istream_iterator<char>(ifs), std::istream_iterator<char>());
        atp.url = item.general.magnet_uri;
        if (!fs::exists(tor_dest)) {
            if (!fs::create_directory(tor_dest)) {
                throw std::runtime_error(tr("Unable to create director, \"%1\".")
                                                 .arg(QString::fromStdString(tor_dest.string())).toStdString());
            }
        }

        atp.save_path = tor_dest.string();
        lt_ses->async_add_torrent(atp);

        std::vector<lt::alert*> alerts;
        do {
            lt_ses->pop_alerts(&alerts);
            for (lt::alert const *a: alerts) {
                std::cout << a->message() << std::endl;
                if (auto at = lt::alert_cast<lt::add_torrent_alert>(a)) {
                    std::string handle_file_path = at->handle.save_path();
                    if (!lt_to_handle.contains(handle_file_path) && !unique_id_cache.contains(handle_file_path)) {
                        lt_to_handle.insert(handle_file_path, at->handle);

                        GekkoFyre::GkFile::FileDb db_struct = routines->openDatabase(CFG_HISTORY_DB_FILE);
                        std::vector<GekkoFyre::GkFile::FileDbVal> file_loc_vec = routines->read_db_vec(LEVELDB_KEY_TORRENT_FLOC, db_struct);
                        std::string handle_unique_id = routines->determine_unique_id(file_loc_vec, handle_file_path, db_struct);
                        unique_id_cache.insert(handle_file_path, handle_unique_id);
                    }

                    if (!async_active) {
                        async_ses = std::async(std::launch::async, &GkTorrentClient::run_session_bckgrnd, this);
                        async_active = true;
                        goto terminate;
                    }
                }

                if (lt::alert_cast<lt::torrent_error_alert>(a)) {
                    std::cerr << a->message() << std::endl;
                    goto terminate;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        } while (alerts.size() > 0);

        terminate: ;
        return;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An issue has occured with torrent, \"%1\".\n\n%2")
                .arg(torrent_error_name).arg(e.what()), QMessageBox::Ok);
        return;
    }
}

/**
 * @brief GekkoFyre::GkTorrentClient::recv_proc_to_stats is an intermediate layer in processing the upload/download and other
 * miscellaneous transfer statistics of the BitTorrent item in question.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07
 * @param save_path
 * @param stats
 * @see MainWindow::recvBitTorrent_XferStats(), MainWindow::manageDlStats()
 */
void GekkoFyre::GkTorrentClient::recv_proc_to_stats(const std::string &save_path, const lt::torrent_status &stats)
{
    GekkoFyre::GkTorrentMisc gk_to_misc;
    GekkoFyre::GkTorrent::TorrentResumeInfo to_xfer_stats;
    to_xfer_stats.save_path = save_path;
    to_xfer_stats.dl_state = gk_to_misc.state(stats.state);

    if (unique_id_cache.contains(save_path)) {
        to_xfer_stats.unique_id = unique_id_cache.value(save_path);
    } else {
        QMessageBox::warning(nullptr, tr("Error!"), tr("Unable to find Unique ID property for BitTorrent item, \"%1\".")
                .arg(QString::fromStdString(save_path)), QMessageBox::Ok);
        return;
    }

    GkTorrent::TorrentXferStats xfer_stats;
    xfer_stats.progress_ppm = stats.progress_ppm;
    xfer_stats.ul_rate = stats.upload_rate;
    xfer_stats.dl_rate = stats.download_rate;
    xfer_stats.num_pieces_downloaded = stats.num_pieces;
    to_xfer_stats.xfer_stats = xfer_stats;

    to_xfer_stats.last_seen_cmplte = stats.last_seen_complete;
    to_xfer_stats.last_scrape = stats.last_scrape;
    to_xfer_stats.total_downloaded = stats.all_time_download;
    to_xfer_stats.total_uploaded = stats.all_time_upload;

    emit xfer_torrent_info(to_xfer_stats);
    return;
}

/**
 * @brief GekkoFyre::GkTorrentClient::rand_port does as the name says; it will create a random port number for you!
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-06-27
 * @return A random port number in the range of TORRENT_MIN_PORT_LISTEN <= TORRENT_MAX_PORT_LISTEN
 */
int GekkoFyre::GkTorrentClient::rand_port() const
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(TORRENT_MIN_PORT_LISTEN, TORRENT_MAX_PORT_LISTEN);

    return uni(rng);
}

/**
 * @brief GekkoFyre::GkTorrentClient::run_session_bckgrnd runs in the background on its own, dedicated thread, separate to that of
 * the GUI and any others. It processes all the relevant helper functions needed to maintain BitTorrent downloads which are
 * operating at a asynchronous level.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07
 */
void GekkoFyre::GkTorrentClient::run_session_bckgrnd()
{
    try {
        clk::time_point last_save_resume = clk::now();

        if (lt_ses->is_valid()) {
            // This is the handle we'll set once we get the notification of it being added
            for (;;) {
                reset: ;

                std::vector<lt::alert*> alerts;
                alerts.clear();
                lt_ses->pop_alerts(&alerts);
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
                        std::string resume_file = std::string(rd->handle.save_path() + rd->torrent_name() +
                                                              FYREDL_TORRENT_RESUME_FILE_EXT);
                        std::ofstream of(resume_file, std::ios::binary);
                        of.unsetf(std::ios_base::skipws);
                        lt::bencode(std::ostream_iterator<char>(of), *rd->resume_data);
                    }

                    if (auto st = lt::alert_cast<lt::state_update_alert>(a)) {
                        if (st->status.empty()) continue;
                        int p = -1;
                        QMap<std::string, lt::torrent_handle>::iterator i;
                        for (i = lt_to_handle.begin(); i != lt_to_handle.end(); ++i) {
                            ++p;
                            const lt::torrent_status &s = st->status[p];
                            emit xfer_internal_stats(i.value().status().save_path, s);
                        }
                    }

                    if (auto msg = lt::alert_cast<lt::torrent_finished_alert>(a)) {
                        if (msg->message().empty()) continue;
                        // Process state change
                        std::cout << msg->message() << std::endl;
                        goto done;
                    }

                    if (auto msg = lt::alert_cast<lt::torrent_removed_alert>(a)) {
                        if (msg->message().empty()) continue;
                        // Process state change
                        std::cout << msg->message() << std::endl;
                        goto done;
                    }

                    if (auto msg = lt::alert_cast<lt::torrent_deleted_alert>(a)) {
                        if (msg->message().empty()) continue;
                        // Process state change
                        std::cout << msg->message() << std::endl;
                        goto done;
                    }

                    if (auto msg = lt::alert_cast<lt::torrent_paused_alert>(a)) {
                        if (msg->message().empty()) continue;
                        // Process state change
                        std::cout << msg->message() << std::endl;
                        goto done;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                // Ask the session to post a state_update_alert, to update our state output for the torrent
                lt_ses->post_torrent_updates();

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
        return;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return;
    }
}
