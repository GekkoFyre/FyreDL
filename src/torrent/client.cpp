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
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <QString>
#include <random>
#include <thread>

namespace sys = boost::system;
namespace fs = boost::filesystem;

/**
 * @note <http://www.rasterbar.com/products/libtorrent/manual.html>
 *       <http://libtorrent.org/tutorial.html>
 *       <http://libtorrent.org/reference-Core.html#save_resume_data()>
 *       <http://libtorrent.org/reference-Core.html#session_handle>
 */
GekkoFyre::GkTorrentClient::GkTorrentClient()
{
    routines = std::make_unique<GekkoFyre::CmnRoutines>();

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

    std::string interface = std::string("0.0.0.0:" + std::to_string(rand_port()));
    pack.set_str(lt::settings_pack::listen_interfaces, interface);          // Binding to port 0 will make the operating system pick the port. The default is "0.0.0.0:6881", which binds to all interfaces on port 6881. Once/if binding the listen socket(s) succeed, listen_succeeded_alert is posted.

    lt_ses = std::make_shared<lt::session>(pack, 0);

    gk_ses_thread = new QThread;
    gk_to_ses = new GekkoFyre::GkTorrentSession(lt_ses); // Create a session handler
    gk_to_ses->moveToThread(gk_ses_thread);

    QObject::connect(gk_ses_thread, SIGNAL(started()), gk_to_ses, SLOT(run_session_bckgrnd()));
    QObject::connect(this, SIGNAL(update_ses_hash(std::string,lt::torrent_handle)), gk_to_ses, SLOT(recv_hash_update(std::string,lt::torrent_handle)));
    QObject::connect(gk_to_ses, SIGNAL(sendStats(std::string,lt::torrent_status)), this, SLOT(recv_proc_to_stats(std::string,lt::torrent_status)));
    QObject::connect(gk_to_ses, SIGNAL(finish_gk_ses_thread()), gk_ses_thread, SLOT(quit()));
    QObject::connect(gk_to_ses, SIGNAL(finish_gk_ses_thread()), gk_to_ses, SLOT(deleteLater()));
    QObject::connect(gk_ses_thread, SIGNAL(finished()), gk_ses_thread, SLOT(deleteLater()));
    gk_ses_thread->start();
}

GekkoFyre::GkTorrentClient::~GkTorrentClient()
{}

/**
 * @brief GekkoFyre::GkTorrentClient::startTorrentDl reads the download item's Unique ID from the XML history file
 * and initiates the BitTorrent session with the appropriate parameters.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-22
 * @param unique_id The Unique ID relating to the download item in question.
 */
void GekkoFyre::GkTorrentClient::startTorrentDl(const GekkoFyre::GkTorrent::TorrentInfo &item)
{
    QString torrent_error_name = QString::fromStdString(item.general.torrent_name);

    try {
        std::string full_path = item.general.down_dest + fs::path::preferred_separator + item.general.torrent_name;
        lt::add_torrent_params atp; // http://libtorrent.org/reference-Core.html#add-torrent-params

        // Load resume data from disk and pass it in as we add the magnet link
        std::ifstream ifs(FYREDL_TORRENT_RESUME_FILE_EXT, std::ios::binary);
        ifs.unsetf(std::ios::skipws);
        atp.resume_data.assign(std::istream_iterator<char>(ifs), std::istream_iterator<char>());
        atp.url = item.general.magnet_uri;
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

/**
 * @brief GekkoFyre::GkTorrentClient::rand_port
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-06-27
 * @note <https://stackoverflow.com/questions/5008804/generating-random-integer-from-a-range>
 * @return
 */
int GekkoFyre::GkTorrentClient::rand_port() const
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(TORRENT_MIN_PORT_LISTEN, TORRENT_MAX_PORT_LISTEN);

    return uni(rng);
}
