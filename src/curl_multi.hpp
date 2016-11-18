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
 * @file curl_multi.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11-01
 * @brief Contains the routines for downloading (and directly managing therof) any files, asynchronously.
 */

#ifndef FYREDL_CURLMULTI_HPP
#define FYREDL_CURLMULTI_HPP

#include "default_var.hpp"
#include "singleton_emit.hpp"
#include <pugixml.hpp>
#include <boost/exception/all.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_unordered_map.hpp>
#include <string>
#include <fstream>
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <qmetatype.h>

extern "C" {
#include <curl/curl.h>
}

namespace GekkoFyre {
class CurlMulti : public QObject {
    Q_OBJECT

public:
    CurlMulti();
    ~CurlMulti();

    static bool fileStream();

public slots:
    /* 1.0) Create new easy-handle
     *   1.1) Get unique UUID
     * 2.0) Check for new transfers
     * 3.0) When transfers are done, delete the easy-handle
     */

    void recvNewDl(const QString &url, const QString &fileLoc);
    void recvStopDl(const QString &fileLoc);

signals:
    void sendXferStats(const GekkoFyre::GkCurl::CurlProgressPtr &dl_stat);
    void sendDlFinished(const GekkoFyre::GkCurl::DlStatusMsg &status);

private:
    // http://stackoverflow.com/questions/10333854/how-to-handle-a-map-with-pointers
    // https://theboostcpplibraries.com/boost.pointer_container
    static boost::ptr_unordered_map<std::string, GekkoFyre::GkCurl::CurlInit> eh_vec; // Easy handle mapped to a ID, for managing each connection
    static GekkoFyre::GkCurl::GlobalInfo *gi;
    static QMutex mutex;

    static std::string createId();

    static void mcode_or_die(const char *where, CURLMcode code);

    static void check_multi_info(GekkoFyre::GkCurl::GlobalInfo *g);
    static void event_cb(GekkoFyre::GkCurl::GlobalInfo *g, boost::asio::ip::tcp::socket *tcp_socket, int action);
    static void timer_cb(const boost::system::error_code &error, GekkoFyre::GkCurl::GlobalInfo *g);
    static int multi_timer_cb(CURLM *multi, long timeout_ms, GekkoFyre::GkCurl::GlobalInfo *g);

    static void remsock(int *f, GekkoFyre::GkCurl::GlobalInfo *g);
    static void setsock(int *fdp, curl_socket_t s, CURL *e, int act, GekkoFyre::GkCurl::GlobalInfo *g);
    static void addsock(curl_socket_t s, CURL *easy, int action, GekkoFyre::GkCurl::GlobalInfo *g);
    static int sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp); // https://curl.haxx.se/libcurl/c/CURLMOPT_SOCKETFUNCTION.html

    static int curl_xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow); // https://curl.haxx.se/libcurl/c/CURLOPT_PROGRESSFUNCTION.html
    static curl_socket_t opensocket(void *clientp, curlsocktype purpose, struct curl_sockaddr *address); // https://curl.haxx.se/libcurl/c/CURLOPT_OPENSOCKETFUNCTION.html
    static int close_socket(void *clientp, curl_socket_t item); // https://curl.haxx.se/libcurl/c/CURLOPT_CLOSESOCKETFUNCTION.html
    static size_t curl_write_file_callback(char *buffer, size_t size, size_t nmemb, void *userdata);
    static std::string new_conn(const QString &url, const QString &fileLoc, GekkoFyre::GkCurl::GlobalInfo *global);

};
    typedef SingletonEmit<CurlMulti> routine_singleton;
}

// This is required for signaling, otherwise QVariant does not know the type.
Q_DECLARE_METATYPE(GekkoFyre::GkCurl::CurlProgressPtr);
Q_DECLARE_METATYPE(GekkoFyre::GkCurl::DlStatusMsg);

#endif // FYREDL_CURLMULTI_HPP
