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
#include <iostream>
#include <memory>
#include <vector>
#include <ctime>
#include <QString>
#include <QtCharts>
#include <QLineSeries>

extern "C" {
#include <curl/curl.h>
}

#ifdef _WIN32
#define _WIN32_WINNT 0x06000100
#include <SDKDDKVer.h>
#include <Windows.h>

#elif __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

extern "C" {
}

#else
#error "Platform not supported!"
#endif

#define FYREDL_PROG_VERS "0.0.1"          // The application version
#define FYREDL_USER_AGENT "FyreDL/0.0.1"  // The user-agent displayed externally by FyreDL, along with the application version.
#define CFG_HISTORY_FILE "fyredl.xml"     // The configuration/history file-name used by FyreDL. Location is set within the application's GUI.
#define ENBL_GUI_CHARTS false             // Whether to enable charts/graphs within the GUI, to chart the progress of downloads.
#define FYREDL_LIBCURL_VERBOSE 1L         // Set to '1L' if you want libcurl to tell you what it's up to!
#define FYREDL_CONN_TIMEOUT 60L           // The duration, in seconds, until a timeout occurs when attempting to make a connection.
#define FYREDL_CONN_LOW_SPEED_CUTOUT 512L // The average transfer speed in bytes per second to be considered below before connection cut-off.
#define FYREDL_CONN_LOW_SPEED_TIME 10L    // The number of seconds that the transfer speed should be below 'FYREDL_CONN_LOW_SPEED_CUTOUT' before connection cut-off.

//
// ###################################
//   DO NOT MODIFY BELOW THIS LINE!
// Unless you know what you are doing
// ###################################
//

// Download and File I/O
#define WRITE_BUFFER_SIZE (CURL_MAX_WRITE_SIZE * 8) // Measured in bytes, see <https://curl.haxx.se/libcurl/c/CURLOPT_BUFFERSIZE.html>
#define CURL_MAX_WAIT_MSECS (15 * 1000) // Measured in milliseconds

// XML configuration
#define XML_PARENT_NODE "fyredl-db"
#define XML_CHILD_NODE_FILE "file"
#define XML_CHILD_ITEM_FILE "item"
#define XML_ITEM_ATTR_FILE_CID "content-id"                // The unique, content integer ID of the download in the XML history file
#define XML_ITEM_ATTR_FILE_FLOC "file-loc"                 // Location of the file on user's storage disk
#define XML_ITEM_ATTR_FILE_STAT "status"                   // The download status (i.e., downloading, completed, unknown, etc.)
#define XML_ITEM_ATTR_FILE_INSERT_DATE "insert-date"       // Date upon which the download was added to the XML history file
#define XML_ITEM_ATTR_FILE_STATMSG "status-msg"            // Status message returned by the libcurl library
#define XML_ITEM_ATTR_FILE_EFFEC_URL "effec-url"           // Given, proper URL returned from the (source) web-server
#define XML_ITEM_ATTR_FILE_RESP_CODE "resp-code"           // Given return-code returned from the (source) web-server
#define XML_ITEM_ATTR_FILE_CONT_LNGTH "content-length"     // The size of the download returned from the (source) web-server
#define XML_ITEM_ATTR_FILE_HASH_TYPE "hash-type"           // The type of hash employed (i.e, SHA1, SHA3-256/512, MD5, etc.)
#define XML_ITEM_ATTR_FILE_HASH_VAL_GIVEN "hash-val-given" // The hash-value that was given by the user
#define XML_ITEM_ATTR_FILE_HASH_VAL_RTRND "hash-val-rtrnd" // The hash-value that was calculated after the download (presumably) succeeded
#define XML_ITEM_ATTR_FILE_HASH_SUCC_TYPE "hash-succ-type" // Whether the calculated hash of the download matched the given hash or not

// These determine the columns used in QTableView
#define MN_FILENAME_COL 0
#define MN_FILESIZE_COL 1
#define MN_DOWNLOADED_COL 2
#define MN_PROGRESS_COL 3
#define MN_UPSPEED_COL 4
#define MN_DOWNSPEED_COL 5
#define MN_STATUS_COL 6
#define MN_DESTINATION_COL 7
#define MN_URL_COL 8

// This determines the currently selected tab at the bottom of the main window
#define TAB_INDEX_GENERAL 0
#define TAB_INDEX_TRANSFER 1
#define TAB_INDEX_CONTENTS 2
#define TAB_INDEX_GRAPH 3
#define TAB_INDEX_LOG 4

// Comma Separated Value related information
#define CSV_NUM_COLS 3
#define CSV_FIELD_URL "url"
#define CSV_FIELD_DEST "destination"
#define CSV_FIELD_HASH "hash"

namespace GekkoFyre {
    enum DownloadStatus {
        Downloading,
        Completed,
        Failed,
        Paused,
        Stopped,
        Unknown,
        Invalid
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

    enum HashType {
        SHA1,
        SHA256,
        SHA512,
        SHA3_256,
        SHA3_512,
        MD5,
        CannotDetermine,
        None,
    };

    enum HashVerif {
        Analyzing,
        Verified,
        Corrupt,
        NotApplicable
    };

    namespace GkFile {
        struct FileHash {
            GekkoFyre::HashType hash_type;
            GekkoFyre::HashVerif hash_verif;
            QString checksum;
        };
    }

    namespace GkCurl {
        // http://stackoverflow.com/questions/18031357/why-the-constructor-of-stdostream-is-protected
        struct FileStream {
            std::string file_loc;   // Name to store file as if download /and/ disk writing is successful
            std::ofstream *astream; // I/O stream
        };

        struct curl_multi_destructor {
            void operator()(CURLM *multi) {
                if (multi != nullptr) {
                    curl_multi_cleanup(multi);
                }
            }
        };

        // Global information, common to all connections
        struct GlobalInfo {
            std::shared_ptr<CURLM> multi;
            int still_running;
        };

        struct MemoryStruct {
            std::string memory;
            size_t size;
        };

        // Information associated with a specific easy handle
        struct ConnInfo {
            CURL *easy;                  // libcurl easy-interface pointer
            std::string url;             // The effective URL of the download in question
            char error[CURL_ERROR_SIZE]; // Any reportable errors go here
            CURLMcode curl_res;          // The return code from the easy-interface
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
            curl_off_t dltotal;   // Total amount downloaded
            curl_off_t uptotal;   // Total amount uploaded
            double dlnow;         // Current download speed
            double upnow;         // Current upload speed
            std::time_t cur_time; // Time since epoch at which these statistics were gathered (for charting facilities)
        };

        struct DlStatusMsg {
            bool dl_compl_succ;   // Whether the download was completed successfully or aborted with an error
            double content_len;   // The content-length of the finished download
            QString url;          // The URL of the download in question
            std::string file_loc; // The full location of where the file is being saved to disk
        };

        struct CurlProgressPtr {
            CURL *curl;                    // Easy interface pointer
            std::vector<CurlDlStats> stat; // Download statistics struct
            std::string url;               // The URL in question
            std::string file_dest;         // The destination of where the download is being saved to disk
            std::time_t timer_begin;       // The time since epoch at which the timer begun (for charting facilities)
            bool timer_set;                // Whether the timer, 'timer_begin' has been set for this object or not
            bool dl_complete;              // Whether the download has completed or not. Used for correctly displaying the statistics in the GUI.
        };

        struct CurlDlInfo {
            std::string file_loc;                // The location of the downloaded file being streamed towards
            unsigned int cId;                    // Automatically incremented Content ID for each download/file
            uint timestamp;                      // The date/time of the download/file having been inserted into the history file
            GekkoFyre::DownloadStatus dlStatus;  // Status of the downloading file(s) in question
            CurlInfoExt ext_info;                // Extended info about the file(s) themselves
            GekkoFyre::HashType hash_type;       // The actual type of hash used (e.g., CRC32/MD5/SHA1/SHA256/SHA512)
            std::string hash_val_given;          // The value of the hash, if a type is specified in 'CurlDlInfo::hash_type', given by the user
            std::string hash_val_rtrnd;          // Same as above, but calculated from the local file when, presumably, successfully downloaded
            GekkoFyre::HashVerif hash_succ_type; // Whether the calculated hash matched the given hash or not
        };

        struct CurlInit {
            std::shared_ptr<ConnInfo> conn_info;
            MemoryStruct mem_chunk;
            FileStream file_buf;
            CurlProgressPtr prog;
            std::string uuid;
        };
    }

    namespace GkGraph {
        struct DownSpeedGraph {
            QtCharts::QLineSeries *down_speed_series;
            bool down_speed_init;
            std::vector<std::pair<double, double>> down_speed_vals;
        };

        struct GraphInit {
            DownSpeedGraph down_speed;
            QString file_dest;
            bool currShown;
        };
    }
}

#endif // DEFVAR_HPP
