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
#include <libtorrent/session.hpp>
#include <libtorrent/fingerprint.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/bencode.hpp>
#include <utility>
#include <fstream>
#include <random>
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>
#include <QMessageBox>

using clk = std::chrono::steady_clock;

GekkoFyre::GkTorrentSession::GkTorrentSession(QObject *parent) : QObject(parent)
{}

/**
 * @brief GekkoFyre::GkTorrentSession initiates a BitTorrent session with all the required parameters.
 * @date 2017-06-24
 * @note <http://www.rasterbar.com/products/libtorrent/manual.html>
 *       <http://libtorrent.org/tutorial.html>
 *       <http://libtorrent.org/reference-Core.html#save_resume_data()>
 *       <http://libtorrent.org/reference-Core.html#session_handle>
 */
GekkoFyre::GkTorrentSession::GkTorrentSession(lt::session_handle *gk_lt_ses, QObject *parent)
{
    gk_lt_th = new QMap<std::string, lt::torrent_handle>();

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

    const int port = rand_port();
    std::string interface = std::string("0.0.0.0:" + std::to_string(port));
    pack.set_str(lt::settings_pack::listen_interfaces, interface);          // Binding to port 0 will make the operating system pick the port. The default is "0.0.0.0:6881", which binds to all interfaces on port 6881. Once/if binding the listen socket(s) succeed, listen_succeeded_alert is posted.

    lt_ses = new lt::session(pack, 0);
    gk_lt_ses = lt_ses;

    // While we wait for items to be added to the QMap object, we pause the thread until an item is
    // added and THEN process the function 'run_session_bckgrnd()'. There is also a timeout of 5 seconds.
    clk::time_point cur_time = clk::now();
    while (gk_lt_th->empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        if ((clk::now() - cur_time) > std::chrono::seconds(5)) {
            emit finish_gk_ses_thread();
            return;
        }
    }

    run_session_bckgrnd();
}

GekkoFyre::GkTorrentSession::~GkTorrentSession()
{
    gk_lt_th->clear();
    delete lt_ses;
    delete gk_lt_th;
}

void GekkoFyre::GkTorrentSession::recv_hash_update(const std::string &save_dir,
                                                   const lt::torrent_handle &lt_at)
{
    if (!gk_lt_th->contains(save_dir)) {
        gk_lt_th->insert(save_dir, lt_at);
    }
}

/**
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-06-29
 */
void GekkoFyre::GkTorrentSession::run_session_bckgrnd()
{
    try {
        clk::time_point last_save_resume = clk::now();

        // This is the handle we'll set once we get the notification of it being added
        for (;;) {
            reset: ;

            if (gk_lt_th->size() < 1) { // No more torrents left, so end session!
                goto done;
            }

            std::vector<lt::alert*> alerts;
            alerts.clear();
            lt_ses->pop_alerts(&alerts);
            for (lt::alert const *a: alerts) {
                // If we receive the finished alert or an error, we're done
                if (auto at = lt::alert_cast<lt::torrent_finished_alert>(a)) {
                    at->handle.save_resume_data();
                    gk_lt_th->remove(at->handle.status().save_path);
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
                    for (i = gk_lt_th->begin(); i != gk_lt_th->end(); ++i) {
                        ++p;
                        const lt::torrent_status &s = st->status[p];
                        emit sendStats(i.value().status().save_path, s);
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // Ask the session to post a state_update_alert, to update our state output for the torrent
            lt_ses->post_torrent_updates();

            // Save resume data once every 30 seconds
            if (clk::now() - last_save_resume > std::chrono::seconds(30)) {
                QMap<std::string, lt::torrent_handle>::iterator i;
                for (i = gk_lt_th->begin(); i != gk_lt_th->end(); ++i) {
                    i.value().save_resume_data();
                }

                last_save_resume = clk::now();
            }
        }

        done: ;
        emit finish_gk_ses_thread();
        return;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An issue has occured while BitTorrenting:\n\n%1")
                .arg(e.what()), QMessageBox::Ok);
    }

    return;
}

/**
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-06-27
 * @note <https://stackoverflow.com/questions/5008804/generating-random-integer-from-a-range>
 * @return
 */
int GekkoFyre::GkTorrentSession::rand_port() const
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(TORRENT_MIN_PORT_LISTEN, TORRENT_MAX_PORT_LISTEN);

    return uni(rng);
}
