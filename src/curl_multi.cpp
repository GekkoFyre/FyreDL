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
 **   Copyright (C) 2016-2017. GekkoFyre.
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
 * @file curl_multi.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11-01
 * @brief Contains the routines for downloading (and directly managing therof) any HTTP/FTP connections, asynchronously.
 */

#include "curl_multi.hpp"
#include "cmnroutines.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include <future>
#include <cstdlib>
#include <random>
#include <ctime>
#include <memory>
#include <QMessageBox>

namespace sys = boost::system;
namespace fs = boost::filesystem;

// Global/Static variables... so, so much ugliness :(
boost::asio::io_service io_service;
boost::asio::deadline_timer timer(io_service);
std::map<curl_socket_t, boost::asio::ip::tcp::socket *> socket_map;

std::time_t lastTime = 0;
boost::ptr_unordered_map<std::string, GekkoFyre::GkCurl::CurlInit> GekkoFyre::CurlMulti::eh_vec;
std::unordered_map<std::string, GekkoFyre::GkCurl::ActiveDownloads> GekkoFyre::CurlMulti::transfer_monitoring;
GekkoFyre::GkCurl::GlobalInfo *GekkoFyre::CurlMulti::gi;
QMutex GekkoFyre::CurlMulti::mutex;
short GekkoFyre::CurlMulti::active_downloads;

GekkoFyre::CurlMulti::CurlMulti()
{
    setlocale (LC_ALL, "");
    // curl_global_init(CURL_GLOBAL_DEFAULT); // https://curl.haxx.se/libcurl/c/curl_global_init.html

    active_downloads = 0;
}

GekkoFyre::CurlMulti::~CurlMulti()
{
    // curl_global_cleanup(); // We're done with libcurl, globally, so clean it up!
    // curl_multi_cleanup(gi->multi);
    if (gi != nullptr) {
        delete gi;
    }
}

/**
 * @brief GekkoFyre::CurlMulti::fileStream
 * @note  <https://curl.haxx.se/libcurl/c/multi-single.html>
 *        <https://gist.github.com/clemensg/4960504>
 *        <https://curl.haxx.se/libcurl/c/curl_multi_info_read.html>
 *        <http://stackoverflow.com/questions/24288513/how-to-do-curl-multi-perform-asynchronously-in-c>
 * @param url
 * @param file_loc
 * @return
 */
bool GekkoFyre::CurlMulti::fileStream()
{
    CURLMcode multi_res;
    int repeats = 0;
    CURLMsg *msg; // For picking up messages with the transfer status
    int msgs_left = 0; // How many messages are left
    GekkoFyre::GkCurl::DlStatusMsg status_msg;
    try {
        if (gi->multi != nullptr) {
            multi_res = curl_multi_perform(gi->multi, &gi->still_running);
            if (multi_res != CURLM_OK) {
                mcode_or_die(tr("Download file routine: ").toStdString().c_str(), multi_res);
            }

            // You need to keep calling curl_multi_perform() to make things transfer. If you don't call
            // it, nothing gets transferred. Everytime you call it, a piece of the transfer is made (if
            // there's anything to do at the time).
            // The function returns CURLM_CALL_MULTI_PERFORM only if you should call it again before you
            // do select(), but you should always keep calling it until the transfer is done. Preferrably
            // after waiting for action with select().
            // If you want stuff to happen "in the background", you need to start a new thread and do the
            // transfer there.
            while ((gi->still_running > 0 && active_downloads > 0) || (gi->still_running != 0)) {
                CURLMcode mc; // curl_multi_wait() return code
                int numfds = 0; // Number of file descriptors

                mc = curl_multi_wait(gi->multi, NULL, 0, CURL_MAX_WAIT_MSECS, &numfds);
                if (mc != CURLM_OK) {
                    throw std::runtime_error(tr("curl_multi_wait() failed! Code: %1").arg(QString::number(mc)).toStdString());
                }

                // 'numfds' being zero means either a timeout or no file-descriptors to wait for. Try
                // timeout on first occurrence, then assume no file descriptors and no file descriptors to
                // wait for means wait for 100 milliseconds.
                if (!numfds) {
                    repeats++;

                    if (repeats > 1) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep for 100 milliseconds
                    }
                } else {
                    repeats = 0;
                }

                if (active_downloads > 0) {
                    curl_multi_perform(gi->multi, &gi->still_running);
                } else {
                    break;
                }
            };

            while ((msg = curl_multi_info_read(gi->multi, &msgs_left))) {
                if (msg->msg == CURLMSG_DONE) {
                    std::string ptr_uuid;
                    GekkoFyre::GkCurl::CurlInit *curl_struct;
                    for (auto const &entry : eh_vec) {
                        // http://www.boost.org/doc/libs/1_49_0/libs/ptr_container/doc/ptr_container.html
                        if (msg->easy_handle == entry.second->conn_info->easy) {
                            ptr_uuid = entry.first;
                            curl_struct = &eh_vec[ptr_uuid];
                            break;
                        }
                    }

                    if (ptr_uuid.empty()) {
                        // Display an error so that we do not read into uninitialized memory!
                        throw std::runtime_error(tr("Warning, 'ptr_uuid' is empty!").toStdString());
                    }

                    std::string stat_uuid;
                    for (auto const &entry : transfer_monitoring) {
                        if (QString::fromStdString(curl_struct->file_buf.file_loc) == entry.second.file_dest) {
                            stat_uuid = entry.first;
                            break;
                        }
                    }

                    if (stat_uuid.empty()) {
                        // Display an error so that we do not read into uninitialized memory!
                        throw std::runtime_error(tr("Warning, 'stat_uuid' is empty!").toStdString());
                    }

                    double content_length = 0;
                    curl_easy_getinfo(curl_struct->conn_info->easy, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
                    std::memcpy(&status_msg.content_len, &content_length, sizeof(double));

                    // The download has either completed successfully or been aborted! Either way, the handle
                    // is no longer needed.
                    status_msg.url = QString::fromStdString(curl_struct->conn_info->url);
                    status_msg.file_loc = curl_struct->prog.file_dest;
                    curl_multi_remove_handle(gi->multi, curl_struct->conn_info->easy);
                    curl_easy_cleanup(curl_struct->conn_info->easy);
                    // curl_multi_cleanup(gi->multi);

                    eh_vec.erase(ptr_uuid);
                    transfer_monitoring.erase(stat_uuid);
                    --active_downloads;

                    mutex.lock();
                    routine_singleton::instance()->sendDlFinished(status_msg);
                    mutex.unlock();
                    return true;
                }
            }
        } else {
            throw std::runtime_error(tr("multi-handle is NULL!").toStdString());
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return false;
}

/**
 * @brief GekkoFyre::CurlMulti::recvNewDl receives and manages any new HTTP(S)/FTP(S) file downloads, and proceeds with the
 * instruction of downloading these files.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @note <http://en.cppreference.com/w/cpp/thread/call_once>
 *       <http://en.cppreference.com/w/cpp/thread/once_flag>
 * @param url The effective URL of the download in question.
 * @param fileLoc The location of the download on the user's local storage.
 * @param resumeDl Whether a pre-existing download should be resumed that has previously been stopped part-way, failed
 * mid-transfer due to an internet outage, etc.
 */
void GekkoFyre::CurlMulti::recvNewDl(const QString &url, const QString &fileLoc, const bool &resumeDl)
{
    try {
        if (gi == nullptr) {
            // Global multi-handle does not exist, so create it
            gi = new GekkoFyre::GkCurl::GlobalInfo;
            gi->multi = curl_multi_init(); // Initiate the libcurl session
            curl_multi_setopt(gi->multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
            curl_multi_setopt(gi->multi, CURLMOPT_SOCKETDATA, gi);
            curl_multi_setopt(gi->multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
            curl_multi_setopt(gi->multi, CURLMOPT_TIMERDATA, gi);
        }

        std::string stat_uuid;
        GekkoFyre::GkCurl::ActiveDownloads dl_stat;
        for (auto const &entry : transfer_monitoring) {
            if (fileLoc == entry.second.file_dest) {
                stat_uuid = entry.first;
                dl_stat = transfer_monitoring[stat_uuid];
                break;
            }
        }

        if (fileLoc.isEmpty() || url.isEmpty()) {
            throw std::invalid_argument(tr("An invalid file location and/or URL has been given!").toStdString());
        }

        if (stat_uuid.empty()) {
            std::string new_uuid = createId();
            GekkoFyre::GkCurl::ActiveDownloads dl_stat_temp;
            dl_stat_temp.file_dest = fileLoc;
            dl_stat_temp.isActive = false;
            transfer_monitoring[new_uuid] = dl_stat_temp;
            dl_stat = dl_stat_temp;
        }

        if ((!stat_uuid.empty() && !dl_stat.isActive) || (stat_uuid.empty())) {
            if (resumeDl) {
                ++active_downloads;
                long byte_offset = GekkoFyre::CmnRoutines::getFileSize(fileLoc.toStdString());
                new_conn(url, fileLoc, gi, byte_offset);
            } else {
                ++active_downloads;
                new_conn(url, fileLoc, gi, 0L);
            }

            fileStream();
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return;
    }

    return;
}

void GekkoFyre::CurlMulti::mcode_or_die(const char *where, CURLMcode code)
{
    if(CURLM_OK != code) {
        const char *s;
        switch(code) {
            case CURLM_CALL_MULTI_PERFORM:
                s = "CURLM_CALL_MULTI_PERFORM";
                break;
            case CURLM_BAD_HANDLE:
                s = "CURLM_BAD_HANDLE";
                break;
            case CURLM_BAD_EASY_HANDLE:
                s = "CURLM_BAD_EASY_HANDLE";
                break;
            case CURLM_OUT_OF_MEMORY:
                s = "CURLM_OUT_OF_MEMORY";
                break;
            case CURLM_INTERNAL_ERROR:
                s = "CURLM_INTERNAL_ERROR";
                break;
            case CURLM_UNKNOWN_OPTION:
                s = "CURLM_UNKNOWN_OPTION";
                break;
            case CURLM_LAST:
                s = "CURLM_LAST";
                break;
            default:
                s = "CURLM_unknown";
                break;
            case CURLM_BAD_SOCKET:
                s = "CURLM_BAD_SOCKET";
                /* ignore this error */
                return;
        }

        throw std::runtime_error(tr("ERROR: %1 returns %2").arg(where).arg(s).toStdString());
    }

    return;
}

/**
 * @brief GekkoFyre::CurlMulti::check_multi_info checks for completed transfers, and removes their easy
 * @note  <https://curl.haxx.se/libcurl/c/asiohiper.html>
 * @param g
 */
void GekkoFyre::CurlMulti::check_multi_info(GekkoFyre::GkCurl::GlobalInfo *g)
{
    char *eff_url;
    CURLMsg *msg;
    int msgs_left;
    GekkoFyre::GkCurl::ConnInfo *conn;
    CURL *easy;
    // CURLcode res;

    std::cout << QString("REMAINING: %1\n").arg(QString::number(g->still_running)).toStdString();

    while((msg = curl_multi_info_read(g->multi, &msgs_left))) {
        if(msg->msg == CURLMSG_DONE) {
            easy = msg->easy_handle;
            // res = msg->data.result;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
            std::cerr << QString("DONE: %1 => %2\n").arg(eff_url).arg(conn->error).toStdString();
            curl_multi_remove_handle(g->multi, easy);
            conn->url.clear();
            curl_easy_cleanup(easy);
            delete conn;
        }
    }
}

/**
 * @brief GekkoFyre::CurlMulti::event_cb is called by asio when there is an action on a socket.
 * @note  <https://curl.haxx.se/libcurl/c/asiohiper.html>
 * @param g
 * @param tcp_socket
 * @param action
 */
void GekkoFyre::CurlMulti::event_cb(GekkoFyre::GkCurl::GlobalInfo *g, boost::asio::ip::tcp::socket *tcp_socket, int action)
{
    std::cout << QString("event_cb: action=%1\n").arg(QString::number(action)).toStdString();

    CURLMcode rc;
    rc = curl_multi_socket_action(g->multi, tcp_socket->native_handle(), action, &g->still_running);

    mcode_or_die("event_cb: curl_multi_socket_action", rc);
    check_multi_info(g);

    if(g->still_running <= 0) {
        std::cout << QString("last transfer done, kill timeout\n").toStdString();
        timer.cancel();
    }
}

/**
 * @brief GekkoFyre::CurlMulti::timer_cb is called by asio when our timeout expires.
 * @note  <https://curl.haxx.se/libcurl/c/asiohiper.html>
 * @param error
 * @param g
 */
void GekkoFyre::CurlMulti::timer_cb(const boost::system::error_code &error, GekkoFyre::GkCurl::GlobalInfo *g)
{
    if (!error) {
        CURLMcode rc;
        rc = curl_multi_socket_action(g->multi, CURL_SOCKET_TIMEOUT, 0, &g->still_running);

        mcode_or_die("timer_cb: curl_multi_socket_action", rc);
        check_multi_info(g);
    }
}

/**
 * @brief GekkoFyre::CurlMulti::multi_timer_cb updates the event timer after curl_multi library calls.
 * @note  <https://curl.haxx.se/libcurl/c/asiohiper.html>
 * @param multi
 * @param timeout_ms
 * @param g
 * @return
 */
int GekkoFyre::CurlMulti::multi_timer_cb(CURLM *multi, long timeout_ms, GekkoFyre::GkCurl::GlobalInfo *g)
{
    Q_UNUSED(multi);
    // std::cout << QString("multi_timer_cb: timeout_ms %1\n").arg(QString::number(timeout_ms)).toStdString();

    // Cancel running timer
    timer.cancel();

    if (timeout_ms > 0) {
        // Update timer
        timer.expires_from_now(boost::posix_time::millisec(timeout_ms));
        timer.async_wait(boost::bind(&timer_cb, _1, g));
    } else {
        // Call timeout function immediately
        boost::system::error_code error; /*success*/
        timer_cb(error, g);
    }

    return 0;
}

void GekkoFyre::CurlMulti::remsock(int *f, GekkoFyre::GkCurl::GlobalInfo *g)
{
    Q_UNUSED(g);
    if (f) {
        free (f);
    }
}

void GekkoFyre::CurlMulti::setsock(int *fdp, curl_socket_t s, CURL *e, int act, GekkoFyre::GkCurl::GlobalInfo *g)
{
    Q_UNUSED(e);
    std::cout << QString("netsock: socket=%1, act=%2\n").arg(QString::number(s)).arg(QString::number(act)).toStdString();

    std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socket_map.find(s);

    if (it == socket_map.end()) {
        std::cout << QString("socket %1 is a c-ares socket, ignoring\n").arg(QString::number(s)).toStdString();
        return;
    }

    boost::asio::ip::tcp::socket * tcp_socket = it->second;
    *fdp = act;

    if (act == CURL_POLL_IN) {
        std::cout << QString("watching for socket to become readable\n").toStdString();
        tcp_socket->async_read_some(boost::asio::null_buffers(),
                                    boost::bind(&event_cb, g, tcp_socket, act));
    } else if (act == CURL_POLL_OUT) {
        std::cout << QString("watching for socket to become writable\n").toStdString();
        tcp_socket->async_write_some(boost::asio::null_buffers(),
                                     boost::bind(&event_cb, g, tcp_socket, act));
    } else if (act == CURL_POLL_INOUT) {
        std::cout << QString("watching for socket to become readable AND writable\n").toStdString();
        tcp_socket->async_read_some(boost::asio::null_buffers(),
                                    boost::bind(&event_cb, g, tcp_socket, act));
        tcp_socket->async_write_some(boost::asio::null_buffers(),
                                     boost::bind(&event_cb, g, tcp_socket, act));
    }
}

void GekkoFyre::CurlMulti::addsock(curl_socket_t s, CURL *easy, int action, GekkoFyre::GkCurl::GlobalInfo *g)
{
    // fdp is used to store current action
    int *fdp = (int *) calloc(sizeof(int), 1);

    setsock(fdp, s, easy, action, g);
    curl_multi_assign(g->multi, s, fdp);
}

int GekkoFyre::CurlMulti::sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp)
{
    std::cout << QString("sock_cb: socket=%1, what=%2\n").arg(QString::number(s)).arg(QString::number(what)).toStdString();

    GekkoFyre::GkCurl::GlobalInfo *g = (GekkoFyre::GkCurl::GlobalInfo*) cbp;
    int *actionp = (int *) sockp;
    const char *whatstr[] = { "none", "IN", "OUT", "INOUT", "REMOVE" };

    std::cout << QString("socket callback: s=%1 what=%2\n").arg(QString::number(s)).arg(whatstr[what]).toStdString();

    if (what == CURL_POLL_REMOVE) {
        remsock(actionp, g);
    } else {
        if (!actionp) {
            std::cout << QString("Adding data: %1\n").arg(whatstr[what]).toStdString();
            addsock(s, e, what, g);
        } else {
            std::cout << QString("Changing action from %1 to %2\n").arg(whatstr[*actionp]).arg(whatstr[what]).toStdString();
            setsock(actionp, s, e, what, g);
        }
    }

    return 0;
}

/**
 * @brief GekkoFyre::CurlMulti::curl_xferinfo details the progress of the download or upload.
 * @note  <https://curl.haxx.se/libcurl/c/progressfunc.html>
 *        <https://curl.haxx.se/libcurl/c/chkspeed.html>
 * @param p
 * @param dltotal
 * @param dlnow
 * @param ult
 * @param uln
 * @return
 */
int GekkoFyre::CurlMulti::curl_xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    Q_UNUSED(dltotal);
    Q_UNUSED(ultotal);
    GekkoFyre::GkCurl::CurlProgressPtr *prog = static_cast<GekkoFyre::GkCurl::CurlProgressPtr *>(p);

    /* Under certain circumstances it may be desirable for certain functionality
       to only run every N seconds, in order to do this the transaction time can
       be used */
    std::time_t now = std::time(nullptr);
    if (now - lastTime < 1) {
        return 0;
    }

    if (active_downloads < 1) {
        return -1;
    }

    lastTime = now;
    double dlspeed = 0;
    double upspeed = 0;
    curl_easy_getinfo(prog->curl, CURLINFO_SPEED_DOWNLOAD, &dlspeed);
    curl_easy_getinfo(prog->curl, CURLINFO_SPEED_UPLOAD, &upspeed);

    GekkoFyre::GkCurl::CurlDlStats dl_stat;
    dl_stat.dlnow = dlspeed;
    dl_stat.dltotal = dlnow;
    dl_stat.upnow = upspeed;
    dl_stat.uptotal = ulnow;
    prog->stat.push_back(dl_stat);

    mutex.lock();
    routine_singleton::instance()->sendXferStats(*prog);
    mutex.unlock();

    return 0;
}

curl_socket_t GekkoFyre::CurlMulti::opensocket(void *clientp, curlsocktype purpose, curl_sockaddr *address)
{
    Q_UNUSED(clientp);
    curl_socket_t sockfd = CURL_SOCKET_BAD;

    // Restrict to IPv4
    if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET) {
        // Create a tcp socket object
        boost::asio::ip::tcp::socket *tcp_socket = new boost::asio::ip::tcp::socket(io_service);

        // Open it and get the native handle
        boost::system::error_code ec;
        tcp_socket->open(boost::asio::ip::tcp::v4(), ec);

        if (ec) {
            // An error has occured
            std::ostringstream oss;
            oss << std::endl << "Couldn't open socket [" << ec << "][" << ec.message() << "]";
            std::cerr << QString("ERROR: Returning CURL_SOCKET_BAD to signal error.\n").toStdString();
            throw std::runtime_error(oss.str());
        } else {
            sockfd = tcp_socket->native_handle();
            std::cout << QString("Opened socket %1\n").arg(QString::number(sockfd)).toStdString();

            // Save it for monitoring
            socket_map.insert(std::pair<curl_socket_t, boost::asio::ip::tcp::socket *>(sockfd, tcp_socket));
        }
    }

    return sockfd;
}

int GekkoFyre::CurlMulti::close_socket(void *clientp, curl_socket_t item)
{
    Q_UNUSED(clientp);
    std::cout << QString("close_socket: %1\n").arg(QString::number(item)).toStdString();

    std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socket_map.find(item);

    if (it != socket_map.end()) {
        delete it->second;
        socket_map.erase(it);
    }

    return 0;
}

/**
 * @brief GekkoFyre::CurlMulti::curl_write_file_callback can be used to download data into a local file
 * on the user's storage.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-11
 * @note   <https://linustechtips.com/main/topic/663949-libcurl-curlopt_writefunction-callback-function-error/>
 *         <https://www.apriorit.com/dev-blog/344-libcurl-usage-downloading-protocols>
 *         <http://fwheel.net/aio.html>
 *         <http://man7.org/linux/man-pages/man7/aio.7.html>
 *         <https://gist.github.com/rsms/771059>
 *         <http://stackoverflow.com/questions/21126950/asynchronously-writing-to-a-file-in-c-unix>
 *         <https://linux.die.net/man/7/aio>
 * @param buffer
 * @param size
 * @param nmemb
 * @param userdata
 * @return
 */
size_t GekkoFyre::CurlMulti::curl_write_file_callback(char *buffer, size_t size, size_t nmemb, void *userdata)
{
    GekkoFyre::GkCurl::FileStream *fs = static_cast<GekkoFyre::GkCurl::FileStream *>(userdata);
    size_t buf_size = (size * nmemb);
    fs->astream->write(buffer, (long)buf_size);
    fs->astream->flush();
    return buf_size;
}

/**
 * @brief GekkoFyre::CurlMulti::new_conn instantiates a new connection pertaining to the given, effective URL which it
 * then downloads and manages the downloading thereof.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10
 * @note   <https://gist.github.com/lijoantony/4098139>
 *         <https://github.com/curl/curl/blob/master/docs/examples/multi-app.c>
 *         <https://curl.haxx.se/libcurl/c/CURLOPT_RESUME_FROM_LARGE.html>
 *         <http://stackoverflow.com/questions/9409997/how-to-resume-file-download-through-ftp-using-curl-in-c>
 *         <http://stackoverflow.com/questions/10448874/download-is-not-resuming-using-curl-c-api>
 *         <http://curl-library.cool.haxx.narkive.com/7E4Ge2fM/how-to-configure-easy-handle-for-ftp-upload-resume>
 * @param url Refers to the effective URL of the download in question.
 * @param fileLoc Where the download is to be stored on the user's local storage.
 * @param global A global, static struct containing important information about the download's operations.
 * @param data_expected The expected file-size of the download in question.
 * @param file_offset An offset, measured in bytes, of where you want to start the download from. Used for resuming
 * downloads that have been paused, for example. Set to '0' to start the transfer from the beginning, and greater than
 * zero (specifically, set to the byte-index) to resume a download.
 */
std::string GekkoFyre::CurlMulti::new_conn(const QString &url, const QString &fileLoc,
                                           GekkoFyre::GkCurl::GlobalInfo *global,
                                           const curl_off_t &file_offset)
{
    std::string uuid = createId();
    GekkoFyre::GkCurl::CurlInit *ci;
    ci = new GekkoFyre::GkCurl::CurlInit;

    ci->conn_info = new GekkoFyre::GkCurl::ConnInfo;
    ci->conn_info->easy = curl_easy_init();

    if (ci->conn_info->easy == nullptr) {
        throw std::runtime_error(tr("'curl_easy_init()' failed, exiting!").toStdString());
    }

    // Maximum time, in seconds, to allow the connection phase before a timeout occurs
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_CONNECTTIMEOUT, FYREDL_CONN_TIMEOUT);

    ci->conn_info->url = url.toStdString();
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_URL, ci->conn_info->url.c_str());

    if (!fileLoc.isEmpty()) {
        ci->file_buf.file_loc = fileLoc.toStdString();
    } else {
        throw std::invalid_argument(tr("An invalid file location has been given (empty parameter)!").toStdString());
    }

    // Initialize these variables, even if they're not used... as it generates spurious errors otherwise.
    ci->mem_chunk.memory = "";
    ci->mem_chunk.size = 0;

    // http://stackoverflow.com/questions/18031357/why-the-constructor-of-stdostream-is-protected
    // TODO: Fix the memory leak hereinafter!
    ci->file_buf.astream = std::make_shared<std::ofstream>();
    if (file_offset == 0 && ci->file_buf.astream != nullptr) {
        ci->file_buf.astream->open(ci->file_buf.file_loc, std::ofstream::out | std::ios::app | std::ios::binary);
    } else if (file_offset > 0 && ci->file_buf.astream != nullptr) {
        ci->file_buf.astream->open(ci->file_buf.file_loc, std::ofstream::out | std::ios::app | std::ios::ate | std::ios::binary);
    } else {
        throw std::invalid_argument(tr("An invalid (negative) file-offset has been given! Value: %1")
                                            .arg(QString::number(file_offset)).toStdString());
    }

    // Send all data to this function, via file streaming
    // NOTE: On Windows, 'CURLOPT_WRITEFUNCTION' /must/ be set, otherwise a crash will occur!
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_WRITEFUNCTION, &curl_write_file_callback);

    // We pass our 'chunk' struct to the callback function
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_WRITEDATA, &ci->file_buf);

    if (file_offset > 0) {
        // This is for resuming transfers, when given the correct byte-offset, otherwise set to '0' for new transfers
        curl_easy_setopt(ci->conn_info->easy, CURLOPT_RESUME_FROM_LARGE, file_offset);
    }

    // We are NOT doing any uploading, but only downloading
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_UPLOAD, 0L);

    #if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_NOPROGRESS, 0L);

    ci->prog.url = url.toStdString();
    ci->prog.file_dest = fileLoc.toStdString();
    ci->prog.timer_set = false;
    ci->prog.content_length = 0;
    ci->prog.curl = ci->conn_info->easy;

    /* xferinfo was introduced in 7.32.0, no earlier libcurl versions will
       compile as they won't have the symbols around.
       If built with a newer libcurl, but running with an older libcurl:
       curl_easy_setopt() will fail in run-time trying to set the new
       callback, making the older callback get used.
       New libcurls will prefer the new callback and instead use that one even
       if both callbacks are set. */
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_XFERINFOFUNCTION, &GekkoFyre::CurlMulti::curl_xferinfo);
    // Pass the struct pointer into the xferinfo function, but note that this is an
    // alias to CURLOPT_PROGRESSDATA
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_XFERINFODATA, &ci->prog);

    #else
    #error "Libcurl needs to be of version 7.32.0 or later! Certain features are missing otherwise..."
    #endif

    // A long parameter set to 1 tells the library to follow any Location: header that the server
    // sends as part of a HTTP header in a 3xx response. The Location: header can specify a relative
    // or an absolute URL to follow.
    // https://curl.haxx.se/libcurl/c/CURLOPT_FOLLOWLOCATION.html
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_FOLLOWLOCATION, 1L);

    // Some servers don't like requests that are made without a user-agent field, so we provide one
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_USERAGENT, FYREDL_USER_AGENT);

    // The maximum redirection limit goes here
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_MAXREDIRS, 12L);

    // Enable TCP keep-alive for this transfer
    // https://curl.haxx.se/libcurl/c/CURLOPT_TCP_KEEPALIVE.html
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_TCP_KEEPIDLE, 120L); // Keep-alive idle time to 120 seconds
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_TCP_KEEPINTVL, 60L); // Interval time between keep-alive probes is 60 seconds

    curl_easy_setopt(ci->conn_info->easy, CURLOPT_VERBOSE, FYREDL_LIBCURL_VERBOSE);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_ERRORBUFFER, ci->conn_info->error);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_PRIVATE, ci->conn_info);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_LOW_SPEED_TIME, FYREDL_CONN_LOW_SPEED_TIME);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_LOW_SPEED_LIMIT, FYREDL_CONN_LOW_SPEED_CUTOUT);

    // Call this function to get a socket
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_OPENSOCKETFUNCTION, opensocket);

    // Call this function to close a socket
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_CLOSESOCKETFUNCTION, close_socket);

    std::cout << QString("Adding easy to multi (%1)\n").arg(url).toStdString();
    ci->conn_info->curl_res = curl_multi_add_handle(global->multi, ci->conn_info->easy);
    mcode_or_die("new_conn: curl_multi_add_handle", ci->conn_info->curl_res);

    std::string stat_uuid;
    for (auto const &entry : transfer_monitoring) {
        if (fileLoc == entry.second.file_dest) {
            stat_uuid = entry.first;
            break;
        }
    }

    if (stat_uuid.empty()) {
        throw std::runtime_error(tr("Warning, 'stat_uuid' is empty!").toStdString());
    }

    transfer_monitoring[stat_uuid].isActive = true;
    eh_vec[uuid] = *ci;
    return uuid;
}

/**
 * @brief GekkoFyre::CurlMulti::createUUID generates a unique UUID and returns the value.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-10-31
 * @note <http://stackoverflow.com/questions/535317/checking-value-exist-in-a-stdmap-c>
 *       <http://stackoverflow.com/questions/13445688/how-to-generate-a-random-number-in-c>
 * @return The uniquely generated UUID.
 */
std::string GekkoFyre::CurlMulti::createId()
{
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist10(0,9);
    std::ostringstream oss;

    for (size_t i = 0; i < 23; ++i) {
        oss << dist10(rng);
    }

    auto search = eh_vec.find(oss.str());
    if (search != eh_vec.end()) {
        throw std::invalid_argument(tr("A given ID already exists!").toStdString());
    }

    return oss.str();
}

/**
 * @brief GekkoFyre::CurlMulti::recvStopDl stops a download via GekkoFyre::CurlMulti::fileStream by
 * intermittently looking at a variable that contains the status information.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11-09
 * @note <https://curl.haxx.se/libcurl/c/curl_multi_remove_handle.html>
 *       <https://curl.haxx.se/docs/faq.html#How_do_I_stop_an_ongoing_transfe>
 * @param fileLoc
 */
void GekkoFyre::CurlMulti::recvStopDl(const QString &fileLoc)
{
    std::string ptr_uuid;
    GekkoFyre::GkCurl::CurlInit *curl_struct;
    for (auto const &entry : eh_vec) {
        // http://www.boost.org/doc/libs/1_49_0/libs/ptr_container/doc/ptr_container.html
        if (fileLoc.toStdString() == entry.second->file_buf.file_loc) {
            ptr_uuid = entry.first;
            curl_struct = &eh_vec[ptr_uuid];
            break;
        }
    }

    std::string stat_uuid;
    GekkoFyre::GkCurl::ActiveDownloads dl_stat;
    for (auto const &entry : transfer_monitoring) {
        if (fileLoc == entry.second.file_dest) {
            stat_uuid = entry.first;
            dl_stat = transfer_monitoring[stat_uuid];
            break;
        }
    }

    if (ptr_uuid.empty() || stat_uuid.empty()) {
        // Display an error so that we do not read into uninitialized memory!
        throw std::runtime_error(tr("Warning, either 'ptr_uuid' or 'stat_uuid' is empty!").toStdString());
    }

    if (dl_stat.isActive) {
        transfer_monitoring[stat_uuid].isActive = false;
        --active_downloads;
        // https://curl.haxx.se/libcurl/c/curl_multi_add_handle.html
        curl_multi_remove_handle(gi->multi, curl_struct->conn_info->easy);
    }

    return;
}
