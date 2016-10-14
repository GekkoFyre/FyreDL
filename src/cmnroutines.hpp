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
        std::FILE *stream;
    };

    struct CurlInit {
        char errbuf[CURL_ERROR_SIZE];
        CURLcode curl_res;
        CURL *curl_ptr;
        MemoryStruct mem_chunk;
        FileStream file_buf;
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

    struct CurlDlInfo {
        std::string file_loc;                   // The location of the downloaded file being streamed towards
        unsigned int cId;                   // Automatically incremented Content ID for each download/file
        uint timestamp;                     // The date/time of the download/file having been inserted into the history file
        GekkoFyre::DownloadStatus dlStatus; // Status of the downloading file(s) in question
        CurlInfoExt ext_info;               // Extended info about the file(s) themselves
    };

    QString extractFilename(const QString &url);
    double bytesToKilobytes(const double &content_length);
    double percentDownloaded(const double &content_length, const double &amountDl);
    std::string findCfgFile(const std::string &cfgFileName);
    std::vector<CurlDlInfo> readDownloadInfo(const std::string &xmlCfgFile = CFG_HISTORY_FILE);
    bool writeDownloadItem(CurlDlInfo dl_info_list, const std::string &xmlCfgFile = CFG_HISTORY_FILE);
    pugi::xml_node createNewXmlFile(const std::string &xmlCfgFile = CFG_HISTORY_FILE);
    bool delDownloadItem(const QString &effec_url, const std::string &xmlCfgFile = CFG_HISTORY_FILE);
    int convDlStat_toInt(const GekkoFyre::DownloadStatus &status);
    GekkoFyre::DownloadStatus convDlStat_toEnum(const int &s);
    QString convDlStat_toString(const GekkoFyre::DownloadStatus &status);

    CurlInfo verifyFileExists(const QString &url);
    CurlInfoExt curlGrabInfo(const QString &url);
    bool fileStream(const QString &url, const QString &file_loc);
    void curlGetProgress(CURL *curl_pointer);

private:
    static size_t curl_write_memory_callback(void *ptr, size_t size, size_t nmemb, void *userp);
    static size_t curl_write_file_callback(void *buffer, size_t size, size_t nmemb, void *stream);
    CurlInit curlInit(const QString &url, const std::string &username, const std::string &password,
                      bool grabHeaderOnly = false, bool writeToMemory = false,
                      const QString &fileLoc = "");
    void curlCleanup(CurlInit curl_init);
};
}

#endif // CMNROUTINES_HPP
