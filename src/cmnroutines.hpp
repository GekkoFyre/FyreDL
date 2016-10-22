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
 * @file cmnroutines.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief Commonly encountered routines that are used throughout this application are defined here.
 */

#ifndef CMNROUTINES_HPP
#define CMNROUTINES_HPP

#include "default_var.hpp"
#include <pugixml.hpp>
#include <boost/exception/all.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <QDateTime>
#include <string>
#include <cstdio>
#include <QString>
#include <QObject>

extern "C" {
#include <curl/curl.h>
}

namespace GekkoFyre {
class CmnRoutines : public QObject
{
    Q_OBJECT

private:
    struct MemoryStruct {
        char *memory;
        size_t size;
    };

    struct FileStream {
        const char *file_loc; // Name to store file as if download /and/ disk writing is successful
        std::FILE *stream;    // File object stream
    };

    // Global information, common to all connections
    struct GlobalInfo {
        CURLM *multi;
        int still_running;
    };

    // Information associated with a specific easy handle
    struct ConnInfo {
        CURL *easy;
        char *url;
        char error[CURL_ERROR_SIZE];
        CURLMcode curlm_res;
    };

    struct CurlProgressPtr {
        double last_runtime;
        CURL *curl;
    };

    struct CurlInit {
        ConnInfo *conn_info;
        GlobalInfo glob_info;
        MemoryStruct mem_chunk;
        FileStream file_buf;
        CurlProgressPtr prog;
    };

public:
    CmnRoutines();
    ~CmnRoutines();

    struct CurlInfo {
        long response_code;        // The HTTP/FTP response code
        std::string effective_url; // In cases when you've asked libcurl to follow redirects, it may very well not be the same value you set with 'CURLOPT_URL'
    };

    struct CurlInfoExt {
        bool status_ok;            // Whether 'CURLE_OK' was returned or not
        std::string status_msg;    // The status message, if any, returned by the libcurl functions
        long long response_code;   // The HTTP/FTP response code
        double elapsed;            // Total time in seconds for the previous transfer (including name resolving, TCP connect, etc.)
        std::string effective_url; // In cases when you've asked libcurl to follow redirects, it may very well not be the same value you set with 'CURLOPT_URL'
        double content_length;     // The size of the download, i.e. content length
    };

    struct CurlDlStats {
        curl_off_t dltotal; // Total downloaded
        curl_off_t uptotal; // Total uploaded
        curl_off_t dlnow;   // Current download
        curl_off_t upnow;   // Current upload
        double cur_time;
        std::string url;    // The URL in question
        CURL *easy;
    };

    struct CurlDlInfo {
        std::string file_loc;               // The location of the downloaded file being streamed towards
        unsigned int cId;                   // Automatically incremented Content ID for each download/file
        uint timestamp;                     // The date/time of the download/file having been inserted into the history file
        GekkoFyre::DownloadStatus dlStatus; // Status of the downloading file(s) in question
        CurlInfoExt ext_info;               // Extended info about the file(s) themselves
    };

    struct CurlDlPtr {
        CURL *ptr;
        std::string url;
    };

    QString extractFilename(const QString &url);
    QString bytesToKilobytes(const double &content_length);
    QString numberSeperators(const QVariant &value);
    double percentDownloaded(const double &content_length, const double &amountDl);

    int convDlStat_toInt(const GekkoFyre::DownloadStatus &status);
    GekkoFyre::DownloadStatus convDlStat_IntToEnum(const int &s);
    QString convDlStat_toString(const GekkoFyre::DownloadStatus &status);
    GekkoFyre::DownloadStatus convDlStat_StringToEnum(const QString &status);

    std::string findCfgFile(const std::string &cfgFileName);
    std::vector<CurlDlInfo> readDownloadInfo(const std::string &xmlCfgFile = CFG_HISTORY_FILE);
    bool writeDownloadItem(CurlDlInfo dl_info_list, const std::string &xmlCfgFile = CFG_HISTORY_FILE);
    pugi::xml_node createNewXmlFile(const std::string &xmlCfgFile = CFG_HISTORY_FILE);
    bool delDownloadItem(const QString &effec_url, const std::string &xmlCfgFile = CFG_HISTORY_FILE);
    bool modifyDlState(const QString &effec_url, const DownloadStatus &status,
                       const std::string &xmlCfgFile = CFG_HISTORY_FILE);

    CurlInfo verifyFileExists(const QString &url);
    CurlInfoExt curlGrabInfo(const QString &url);
    bool fileStream(const QString &url, const QString &file_loc);

signals:
    void sendXferStats(CurlDlStats);
    void sendXferPtr(CurlDlPtr);

private:
    static void mcode_or_die(const char *where, CURLMcode code);

    static void check_multi_info(GlobalInfo *g);
    static void event_cb(GlobalInfo *g, boost::asio::ip::tcp::socket *tcp_socket, int action);
    static void timer_cb(const boost::system::error_code &error, GlobalInfo *g);
    static int multi_timer_cb(CURLM *multi, long timeout_ms, GlobalInfo *g);

    static void remsock(int *f, GlobalInfo *g);
    static void setsock(int *fdp, curl_socket_t s, CURL *e, int act, GlobalInfo *g);
    static void addsock(curl_socket_t s, CURL *easy, int action, GlobalInfo *g);
    static int sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp); // https://curl.haxx.se/libcurl/c/CURLMOPT_SOCKETFUNCTION.html

    int curl_xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow); // https://curl.haxx.se/libcurl/c/CURLOPT_PROGRESSFUNCTION.html
    static curl_socket_t opensocket(void *clientp, curlsocktype purpose, struct curl_sockaddr *address); // https://curl.haxx.se/libcurl/c/CURLOPT_OPENSOCKETFUNCTION.html
    static int close_socket(void *clientp, curl_socket_t item); // https://curl.haxx.se/libcurl/c/CURLOPT_CLOSESOCKETFUNCTION.html
    static size_t curl_write_memory_callback(void *ptr, size_t size, size_t nmemb, void *userp);
    static size_t curl_write_file_callback(void *buffer, size_t size, size_t nmemb, void *stream);
    static CurlInit new_conn(const QString &url, bool grabHeaderOnly = false, bool writeToMemory = false,
                             const QString &fileLoc = "", bool grabStats = false);
};
}

#endif // CMNROUTINES_HPP
