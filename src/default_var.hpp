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
 * @file default_var.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief Default variables used throughout the program that the user may change to suit their preferences
 */

#ifndef DEFVAR_HPP
#define DEFVAR_HPP

#include <locale>
#include <string>
#include <fstream>
#include <QString>

extern "C" {
#include <curl/curl.h>
}

#define CFG_HISTORY_FILE "fyredl_history.xml"
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL 3
#define MAX_WAIT_MSECS (30 * 1000) /* Wait max. 30 seconds */

#define MN_FILENAME_COL 0
#define MN_FILESIZE_COL 1
#define MN_DOWNLOADED_COL 2
#define MN_PROGRESS_COL 3
#define MN_UPSPEED_COL 4
#define MN_DOWNSPEED_COL 5
#define MN_STATUS_COL 6
#define MN_DESTINATION_COL 7
#define MN_URL_COL 8

#define CURL_MAX_WAIT_MSECS 15000

namespace GekkoFyre {
    enum DownloadStatus {
        Downloading,
        Completed,
        Failed,
        Paused,
        Stopped,
        Unknown
    };

    enum DownloadType {
        HTTP,
        FTP,
        Torrent
    };

    enum CurlGrabMethod {
        Header,
        File
    };

    namespace GkCurl {
        struct FileStream {
            std::string file_loc;  // Name to store file as if download /and/ disk writing is successful
            std::ofstream *stream; // File object stream
        };

        // Global information, common to all connections
        struct GlobalInfo {
            CURLM *multi;
            int still_running;
        };

        struct MemoryStruct {
            std::string memory;
            size_t size;
        };

        // Information associated with a specific easy handle
        struct ConnInfo {
            CURL *easy;
            std::string url;
            char error[CURL_ERROR_SIZE];
            CURLMcode curl_res;
        };

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
            double dlnow;       // Current download
            curl_off_t upnow;   // Current upload
            double cur_time;
            std::string url;    // The URL in question
        };

        struct DlStatusMsg {
            bool dl_compl_succ; // Whether the download was completed successfully or aborted with an error
            QString url;        // The URL of the download in question
        };

        struct CurlProgressPtr {
            double lastruntime;
            CURL *curl;
            CurlDlStats stat;
        };

        struct CurlDlInfo {
            std::string file_loc;               // The location of the downloaded file being streamed towards
            unsigned int cId;                   // Automatically incremented Content ID for each download/file
            uint timestamp;                     // The date/time of the download/file having been inserted into the history file
            GekkoFyre::DownloadStatus dlStatus; // Status of the downloading file(s) in question
            CurlInfoExt ext_info;               // Extended info about the file(s) themselves
        };

        struct CurlInit {
            ConnInfo *conn_info;
            MemoryStruct mem_chunk;
            FileStream file_buf;
            CurlProgressPtr prog;
            std::string uuid;
        };
    }
}

#endif // DEFVAR_HPP
