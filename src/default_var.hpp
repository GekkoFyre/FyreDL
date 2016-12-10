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

#include <boost/optional.hpp>
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
#elif __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

extern "C" {
}

#else
#error "Platform not supported!"
#endif

#define FYREDL_PROG_VERS "0.0.1"                // The application version
#define FYREDL_USER_AGENT "FyreDL/0.0.1"        // The user-agent displayed externally by FyreDL, along with the application version.
#define CFG_HISTORY_FILE "fyredl_history.xml"   // The history file-name used by FyreDL. Location is also set within the application's GUI.
#define CFG_SETTINGS_FILE "fyredl_settings.xml" // The configuration file-name used by FyreDL. Location is also set within the application's GUI.
#define ENBL_GUI_CHARTS false                   // Whether to enable charts/graphs within the GUI, to chart the progress of downloads.
#define FYREDL_LIBCURL_VERBOSE 1L               // Set to '1L' if you want libcurl to tell you what it's up to!
#define FYREDL_CONN_TIMEOUT 60L                 // The duration, in seconds, until a timeout occurs when attempting to make a connection.
#define FYREDL_CONN_LOW_SPEED_CUTOUT 512L       // The average transfer speed in bytes per second to be considered below before connection cut-off.
#define FYREDL_CONN_LOW_SPEED_TIME 10L          // The number of seconds that the transfer speed should be below 'FYREDL_CONN_LOW_SPEED_CUTOUT' before connection cut-off.
#define FYREDL_EST_WAIT_TIME_PRECISION 3        // The significant digit precision of the estimated wait time counter for each active transfer

//
// ###################################
//   DO NOT MODIFY BELOW THIS LINE!
// Unless you know what you are doing
// ###################################
//

// Download and File I/O
#define WRITE_BUFFER_SIZE (CURL_MAX_WRITE_SIZE * 8) // Measured in bytes, see <https://curl.haxx.se/libcurl/c/CURLOPT_BUFFERSIZE.html>
#define CURL_MAX_WAIT_MSECS (15 * 1000) // Measured in milliseconds
#define FREE_DSK_SPACE_MULTIPLIER 3

// XML configuration
#define XML_PARENT_NODE "fyredl-db"

#define XML_CHILD_NODE_VERS "fyredl-xml"
#define XML_CHILD_ITEM_VERS "version"
#define XML_ITEM_ATTR_VERS_NO "supported"                 // The supported XML file version for this particular FyreDL build

#define XML_CHILD_NODE_FILE "http"
#define XML_CHILD_ITEM_FILE "item"
#define XML_ITEM_ATTR_FILE_CID "content-id"                // The unique, content integer ID of the download in the XML history file
#define XML_ITEM_ATTR_FILE_FLOC "file-loc"                 // Location of the file on user's storage disk
#define XML_ITEM_ATTR_FILE_STAT "status"                   // The download status (i.e., downloading, completed, unknown, etc.)
#define XML_ITEM_ATTR_FILE_INSERT_DATE "insert-date"       // Date and time upon which the download was added to the XML history file
#define XML_ITEM_ATTR_FILE_COMPLT_DATE "complt-date"       // Date and time upon which the download was completed
#define XML_ITEM_ATTR_FILE_STATMSG "status-msg"            // Status message returned by the libcurl library
#define XML_ITEM_ATTR_FILE_EFFEC_URL "effec-url"           // Given, proper URL returned from the (source) web-server
#define XML_ITEM_ATTR_FILE_RESP_CODE "resp-code"           // Given return-code returned from the (source) web-server
#define XML_ITEM_ATTR_FILE_CONT_LNGTH "content-length"     // The size of the download returned from the (source) web-server
#define XML_ITEM_ATTR_FILE_HASH_TYPE "hash-type"           // The type of hash employed (i.e, SHA1, SHA3-256/512, MD5, etc.)
#define XML_ITEM_ATTR_FILE_HASH_VAL_GIVEN "hash-val-given" // The hash-value that was given by the user
#define XML_ITEM_ATTR_FILE_HASH_VAL_RTRND "hash-val-rtrnd" // The hash-value that was calculated after the download (presumably) succeeded
#define XML_ITEM_ATTR_FILE_HASH_SUCC_TYPE "hash-succ-type" // Whether the calculated hash of the download matched the given hash or not

#define XML_CHILD_NODE_TORRENT "torrent"
#define XML_CHILD_NODE_TORRENT_NODES "nodes"
#define XML_CHILD_NODE_TORRENT_FILES "files"
#define XML_CHILD_NODE_TORRENT_TRACKERS "trackers"
#define XML_CHILD_ITEM_TORRENT "item"
#define XML_CHILD_NODES_NAMES_TORRENT "name"
#define XML_CHILD_NODES_NUMBR_TORRENT "number"
#define XML_CHILD_FILES_PATH_TORRENT "path"
#define XML_CHILD_FILES_HASH_TORRENT "hash"
#define XML_CHILD_TRACKERS_URL_TORRENT "url"
#define XML_CHILD_TRACKERS_TIER_TORRENT "tier"
#define XML_ITEM_ATTR_TORRENT_CID "cid"
#define XML_ITEM_ATTR_TORRENT_FLOC "destination"
#define XML_ITEM_ATTR_TORRENT_INSERT_DATE "insert-date"
#define XML_ITEM_ATTR_TORRENT_COMPLT_DATE "complt-date"
#define XML_ITEM_ATTR_TORRENT_CREATN_DATE "creatn-date"
#define XML_ITEM_ATTR_TORRENT_DLSTATUS "dl-status"
#define XML_ITEM_ATTR_TORRENT_TORRNT_COMMENT "torrnt-comment"
#define XML_ITEM_ATTR_TORRENT_TORRNT_CREATOR "torrnt-creator"
#define XML_ITEM_ATTR_TORRENT_MAGNET_URI "magnet-uri"
#define XML_ITEM_ATTR_TORRENT_TORRNT_NAME "torrnt-name"
#define XML_ITEM_ATTR_TORRENT_NUM_FILES "num-files"
#define XML_ITEM_ATTR_TORRENT_TORRNT_PIECES "torrnt-pieces"
#define XML_ITEM_ATTR_TORRENT_TORRNT_PIECE_LENGTH "torrnt-piece-length"

#define XML_CHILD_NODE_SETTINGS "settings"
#define XML_CHILD_ITEM_SETTINGS "user"
#define XML_ITEM_ATTR_SETTINGS_WIN_Y "main-win-y"
#define XML_ITEM_ATTR_SETTINGS_WIN_X "main-win-x"

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

// [ Enum values ]
// Download Types
#define ENUM_GEKKOFYRE_DOWN_TYPE_HTTP 0
#define ENUM_GEKKOFYRE_DOWN_TYPE_FTP 1
#define ENUM_GEKKOFYRE_DOWN_TYPE_TORRENT 2
#define ENUM_GEKKOFYRE_DOWN_TYPE_MAGNET_LINK 3

// Hash verification
#define ENUM_GEKKOFYRE_HASH_VERIF_ANALYZING 1
#define ENUM_GEKKOFYRE_HASH_VERIF_NOT_APPLIC 0
#define ENUM_GEKKOFYRE_HASH_VERIF_VERIFIED 2
#define ENUM_GEKKOFYRE_HASH_VERIF_CORRUPT -1

namespace GekkoFyre {
    // * If status is paused, and paused is unclicked, or start is clicked, then the download resumes.
    //      * So, pause does just that. It lets you resume from where you left off. Pause the connection to the server/
    //             destination, but don't terminate it. Leave it for as long as naturally possible.
    // * Stop terminates the connection to the server and poses a prompt to to the user asking if he/she wants to delete
    //              the file.
    //      * Ask the user if he/she wants to be prompted about file deletion next time, and whether to automatically
    //              delete if no prompts are chosen. NOTE: Resume does not come into this at all.
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
        FTP,              // Separate handler needed for if there are authentication issues
        Torrent,
        TorrentMagnetLink
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

    namespace GkTorrent {
        struct TorrentFile {
            std::string sha1_hash_hex;
            std::string file_path;
        };

        struct TorrentInfo {
            std::string down_dest;                  // The location of where the download is being streamed towards
            unsigned int cId;                       // Automatically incremented Content ID for each download/file
            long long insert_timestamp;             // The date/time of the download/file having been inserted into the history file
            long long complt_timestamp;             // The date/time of the download/file having completed transfer
            long creatn_timestamp; // The date/time that the torrent file was authored
            GekkoFyre::DownloadStatus dlStatus;     // Status of the downloading file(s) in question
            std::string comment;                    // Any comments left by the author of the torrent file in question
            std::string creator;                    // The author of the torrent file in question
            std::string magnet_uri;                 // The 'BitTorrent Magnet Link' URI
            std::string torrent_name;               // Name of the torrent
            int num_files;                          // How many files are contained within this torrent
            int num_pieces;                         // How many pieces are contained within this torrent
            int piece_length;                       // The length of each piece
            std::vector<std::pair<std::string, int>> nodes;
            std::vector<std::pair<int, std::string>> trackers;
            std::vector<TorrentFile> files_vec;
        };
    }

    namespace Global {
        struct DownloadInfo {
            GekkoFyre::DownloadType dl_type;
        };
    }

    namespace GkCurl {
        // http://stackoverflow.com/questions/18031357/why-the-constructor-of-stdostream-is-protected
        struct FileStream {
            std::string file_loc;   // Name to store file as if download /and/ disk writing is successful
            std::ofstream *astream; // I/O stream
        };

        // Global information, common to all connections
        struct GlobalInfo {
            CURLM *multi;
            int still_running;
        };

        // Monitors which downloads are actively transferring data. A hack to get things working correctly with regard
        // to being able to halt/pause downloads in libcurl.
        struct ActiveDownloads {
            QString file_dest; // The file destination of the download on local storage
            bool isActive;     // Whether the download is actively transferring data to local storage or not
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
            double content_length;         // The file size of the download, as given by the web-server
        };

        struct CurlDlInfo {
            std::string file_loc;                // The location of the downloaded file being streamed towards
            unsigned int cId;                    // Automatically incremented Content ID for each download/file
            long long insert_timestamp;          // The date/time of the download/file having been inserted into the history file
            long long complt_timestamp;          // The date/time of the download/file having completed transfer
            GekkoFyre::DownloadStatus dlStatus;  // Status of the downloading file(s) in question
            CurlInfoExt ext_info;                // Extended info about the file(s) themselves
            GekkoFyre::HashType hash_type;       // The actual type of hash used (e.g., CRC32/MD5/SHA1/SHA256/SHA512)
            std::string hash_val_given;          // The value of the hash, if a type is specified in 'CurlDlInfo::hash_type', given by the user
            std::string hash_val_rtrnd;          // Same as above, but calculated from the local file when, presumably, successfully downloaded
            GekkoFyre::HashVerif hash_succ_type; // Whether the calculated hash matched the given hash or not
        };

        struct CurlInit {
            ConnInfo *conn_info;
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
            GekkoFyre::Global::DownloadInfo down_info; // Hijacking 'GraphInit' for the purpose of using this
            DownSpeedGraph down_speed;                 // The 'download speed' graph
            QString file_dest;                         // File destination of the download on user's local storage
            bool currShown;                            // Whether the graph for the given 'file_dest' is actually displayed or not
        };
    }

    namespace GkSettings {
        struct FyreDL {
            int main_win_x;
            int main_win_y;
        };
    }
}

#endif // DEFVAR_HPP
