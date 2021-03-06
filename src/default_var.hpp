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

#ifndef GK_DEFVAR_HPP
#define GK_DEFVAR_HPP

#include <leveldb/db.h>
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_handle.hpp>
#include <locale>
#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <ctime>
#include <cassert>
#include <cstdlib>
#include <tuple>
#include <QString>
#include <QtCharts>
#include <QLineSeries>
#include <QVariant>
#include <QPointer>

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

#define FYREDL_PROG_VERS "0.0.1"                         // The application version
#define FYREDL_USER_AGENT "FyreDL/0.0.1"                 // The user-agent displayed externally by FyreDL, along with the application version.
#define FYREDL_FINGERPRINT "FyreDL"                      // Fingerprint for the client. Has to be no longer than 20-bytes or will be truncated otherwise.
#define FYREDL_TORRENT_RESUME_FILE_EXT ".fyredl"         // The file extension used for 'resume data' by the BitTorrent side of the FyreDL application
#define CFG_HISTORY_DB_FILE "history.db"
#define CFG_FILES_DIR_LINUX ".fyredl"                    // The name of the settings directory under Linux systems. This can be found in the users home directory.
#define CFG_FILES_DIR_WNDWS "FyreDL"                     // The name of the settings directory under Microsoft Windows. This can be found in the users home directory.
#define CFG_CSV_MIN_PARSE_SIZE 12                        // DO NOT MODIFY! Unless you specifically know what you are doing!
#define ENBL_GUI_CHARTS false                            // Whether to enable charts/graphs within the GUI, to chart the progress of downloads.
#define ENBL_GUI_CONTENTS_VIEW true                      // Whether to enable the contents view of inside BitTorrents (located at the bottom) within the GUI.
#define FYREDL_LIBCURL_VERBOSE 1L                        // Set to '1L' if you want libcurl to tell you what it's up to!
#define FYREDL_CONN_TIMEOUT 60L                          // The duration, in seconds, until a timeout occurs when attempting to make a connection.
#define FYREDL_CONN_LOW_SPEED_CUTOUT 512L                // The average transfer speed in bytes per second to be considered below before connection cut-off.
#define FYREDL_CONN_LOW_SPEED_TIME 10L                   // The number of seconds that the transfer speed should be below 'FYREDL_CONN_LOW_SPEED_CUTOUT' before connection cut-off.
#define FYREDL_EST_WAIT_TIME_PRECISION 3                 // The significant digit precision of the estimated wait time counter for each active transfer
#define FYREDL_UNIQUE_ID_DIGIT_COUNT 32                  // The 'unique identifier' serial number that is given to each download item. This determines how many digits are allocated to this identifier and thus, how much RAM is used for storage thereof.
#define FYREDL_DEFAULT_RESOLUTION_WIDTH 1920.0
#define FYREDL_DEFAULT_UI_TABLE_PIXEL_PADDING 3
#define FYREDL_DEFAULT_UI_TABLE_FONT_PIXEL_SIZE 14
#define FYREDL_DEFAULT_UI_TABLE_FONT_FAMILY "Sans Serif"

//
// ######################################
// #   DO NOT MODIFY BELOW THIS LINE!   #
// # Unless you know what you are doing #
// ######################################
//

// BitTorrent
#define TORRENT_MIN_PORT_LISTEN 10240
#define TORRENT_MAX_PORT_LISTEN 64512

// Download and File I/O
#define WRITE_BUFFER_SIZE (CURL_MAX_WRITE_SIZE * 8) // Measured in bytes, see <https://curl.haxx.se/libcurl/c/CURLOPT_BUFFERSIZE.html>
#define CURL_MAX_WAIT_MSECS (15 * 1000) // Measured in milliseconds
#define FREE_DSK_SPACE_MULTIPLIER 3

// LevelDB configuration
#define LEVELDB_CFG_CACHE_SIZE 32UL * 1024UL * 1024UL
#define LEVELDB_CFG_LOCK_FILE_NAME "LOCK"

#define LEVELDB_STORE_UNIQUE_ID "store-unique-id"

#define LEVELDB_KEY_CURL_STAT "curl-stat"                     // The download status (i.e., downloading, completed, unknown, etc.)
#define LEVELDB_KEY_CURL_INSERT_DATE "curl-insrt-date"        // Date and time upon which the download was added to the XML history file
#define LEVELDB_KEY_CURL_COMPLT_DATE "curl-complt-date"       // Date and time upon which the download was completed
#define LEVELDB_KEY_CURL_STATMSG "curl-statmsg"               // Status message returned by the libcurl library
#define LEVELDB_KEY_CURL_EFFEC_URL "curl-effec-url"           // Given, proper URL returned from the (source) web-server
#define LEVELDB_KEY_CURL_RESP_CODE "curl-resp-code"           // Given return-code returned from the (source) web-server
#define LEVELDB_KEY_CURL_CONT_LNGTH "curl-cont-lngth"         // The size of the download returned from the (source) web-server
#define LEVELDB_KEY_CURL_HASH_TYPE "curl-hash-type"           // The type of hash employed (i.e, SHA1, SHA3-256/512, MD5, etc.)
#define LEVELDB_KEY_CURL_HASH_VAL_GIVEN "curl-hash-val-given" // The hash-value that was given by the user
#define LEVELDB_KEY_CURL_HASH_VAL_RTRND "curl-hash-val-rtrnd" // The hash-value that was calculated after the download (presumably) succeeded
#define LEVELDB_KEY_CURL_HASH_SUCC_TYPE "curl-hash-succ-type" // Whether the calculated hash of the download matched the given hash or not

#define LEVELDB_KEY_TORRENT_INSERT_DATE "to-insert-date"
#define LEVELDB_KEY_TORRENT_COMPLT_DATE "to-complt-date"
#define LEVELDB_KEY_TORRENT_CREATN_DATE "to-creatn-date"
#define LEVELDB_KEY_TORRENT_DLSTATUS "to-status"
#define LEVELDB_KEY_TORRENT_TORRNT_COMMENT "to-comment"
#define LEVELDB_KEY_TORRENT_TORRNT_CREATOR "to-creator"
#define LEVELDB_KEY_TORRENT_MAGNET_URI "to-magnet-uri"
#define LEVELDB_KEY_TORRENT_TORRNT_NAME "to-name"
#define LEVELDB_KEY_TORRENT_NUM_FILES "num-files"
#define LEVELDB_KEY_TORRENT_NUM_TRACKERS "num-trackers"
#define LEVELDB_KEY_TORRENT_TORRNT_PIECES "num-pieces"
#define LEVELDB_KEY_TORRENT_TORRNT_PIECE_LENGTH "piece-length"
#define LEVELDB_KEY_TORRENT_TORRENT_FILES "to-files"
#define LEVELDB_CHILD_NODE_TORRENT_FILES_MAPFLEPCE "map-file-piece"
#define LEVELDB_KEY_TORRENT_TRACKERS "to-extra-trackers"

// XML configuration
#define XML_CHILD_NODE_SETTINGS "settings"
#define XML_CHILD_ITEM_SETTINGS "user"
#define XML_ITEM_ATTR_SETTINGS_WIN_Y "main-win-y"
#define XML_ITEM_ATTR_SETTINGS_WIN_X "main-win-x"

// These determine the columns used in QTableView
#define MN_COL_COUNT 12

#define MN_FILENAME_COL 0       // The name of just the file or torrent being downloaded
#define MN_FILESIZE_COL 1       // The content-length of the download, in kilobytes/megabytes/gigabytes/etc.
#define MN_DOWNLOADED_COL 2     // How much has been downloaded, in kilobytes/megabytes/gigabytes/etc.
#define MN_PROGRESS_COL 3       // The progress towards completion on the download, on a scale of 0-100 percent
#define MN_UPSPEED_COL 4        // The upward transfer speed
#define MN_DOWNSPEED_COL 5      // The downward transfer speed
#define MN_SEEDERS_COL 6        // Related to BitTorrent items; the amount of seeders on a particular download
#define MN_LEECHERS_COL 7       // Related to BitTorrent items; the amount of leechers on a particular download
#define MN_STATUS_COL 8         // Whether the download is paused, stopped, actively transferring, etc.
#define MN_DESTINATION_COL 9    // The destination of the download on local storage
#define MN_URL_COL 10           // The URL column
#define MN_HIDDEN_UNIQUE_ID 11  // Stores the 'unique identifier' serial number for each download. Kind of a hack/cheat :)

// This determines the currently selected tab at the bottom of the main window
#define TAB_INDEX_GENERAL 0
#define TAB_INDEX_TRANSFER 1
#define TAB_INDEX_CONTENTS 2
#define TAB_INDEX_GRAPH 3
#define TAB_INDEX_LOG 4

// Comma Separated Value related information
#define URL_ADD_CSV_NUM_COLS 3
#define URL_ADD_CSV_FIELD_URL "url"
#define URL_ADD_CSV_FIELD_DEST "destination"
#define URL_ADD_CSV_FIELD_HASH "hash"

#define LEVELDB_CSV_UNIQUE_ID_COLS 3
#define LEVELDB_CSV_UID_KEY "unique-id"
#define LEVELDB_CSV_UID_VALUE1 "file-loc"
#define LEVELDB_CSV_UID_VALUE2 "is-torrent"

#define LEVELDB_CSV_TORRENT_FILE_COLS 8
#define LEVELDB_CSV_TORRENT_FILE_PATH "file-path"
#define LEVELDB_CSV_TORRENT_FILE_SHA1 "sha1-hash"
#define LEVELDB_CSV_TORRENT_FILE_FLAGS "flags"
#define LEVELDB_CSV_TORRENT_FILE_CONTENT_LENGTH "content-length"
#define LEVELDB_CSV_TORRENT_FILE_FILE_OFFSET "file-offset"
#define LEVELDB_CSV_TORRENT_FILE_MTIME "mod-time"
#define LEVELDB_CSV_TORRENT_FILE_MAPFLEPCE_KEY "mapflepce-key"
#define LEVELDB_CSV_TORRENT_FILE_BOOL_DLED "downloaded-bool"

#define LEVELDB_CSV_TORRENT_MAPFLEPCE_COLS 2
#define LEVELDB_CSV_TORRENT_MAPFLEPCE_1 "mapflepce-1"
#define LEVELDB_CSV_TORRENT_MAPFLEPCE_2 "mapflepce-2"

#define LEVELDB_CSV_TORRENT_TRACKER_COLS 3
#define LEVELDB_CSV_TORRENT_TRACKER_TIER "tier"
#define LEVELDB_CSV_TORRENT_TRACKER_URL "url"
#define LEVELDB_CSV_TORRENT_TRACKER_BOOL_ENABLED "bool-enabled"

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

        struct FileDb {
            std::shared_ptr<leveldb::DB> db;
            leveldb::Options options;
        };
    }

    namespace GkTorrent {
        struct TorrentXferStats {
            int dl_rate;                     // The total rates for all peers for this torrent. The rates are given as the number of bytes per second.
            int ul_rate;                     // The total rates for all peers for this torrent. The rates are given as the number of bytes per second.
            int num_pieces_downloaded;       // The number of pieces that has been downloaded. This can be used to see if anything has updated since last time if you want to keep a graph of the pieces up to date.
            int progress_ppm;                // Reflects progress, but instead [0, 1000000] (ppm = parts per million).
            std::time_t cur_time;
        };

        struct TorrentResumeInfo {
            std::string unique_id;           // A unique identifier for this torrent
            std::string save_path;
            QString dl_state;
            boost::int64_t total_uploaded;   // These are saved in and restored from resume data to maintain totals across sessions.
            boost::int64_t total_downloaded; // These are saved in and restored from resume data to maintain totals across sessions.
            std::time_t last_seen_cmplte;    // The time when we, or one of our peers, last saw a complete copy of this torrent.
            std::time_t last_scrape;         // The number of seconds since this torrent acquired scrape data. If it has never done that, this value is -1.
            std::time_t time_started;
            boost::optional<GkTorrent::TorrentXferStats> xfer_stats;
        };

        struct TorrentFile {
            std::string unique_id;              // A unique identifier for this torrent
            std::string file_path;              // The internal path of the file within the torrent
            std::string sha1_hash_hex;          // The SHA-1 hash of the file, if available, in hexadecimal
            int flags;
            int64_t content_length;             // The content length of this particular file
            int64_t file_offset;                // The internal offset of the file within the torrent
            uint32_t mtime;                     // Modification time? Not sure...
            std::pair<int, int> map_file_piece; // <first, last> How the pieces are mapped within the torrent for this particular file
            bool downloaded;                    // Whether this particular file is downloaded or not
        };

        struct TorrentTrackers {
            std::string unique_id;                  // A unique identifier for this torrent
            int tier;                               // The tier number of the tracker in question
            std::string url;                        // The <URL:port> of the tracker in question
            bool enabled;                           // Whether this tracker is active for transfers or not
        };

        struct GeneralInfo {
            std::string unique_id;                  // A unique identifier for this torrent
            std::string down_dest;                  // The location of where the download is being streamed towards
            long long insert_timestamp;             // The date/time of the download/file having been inserted into the history file
            long long complt_timestamp;             // The date/time of the download/file having completed transfer
            long creatn_timestamp;                  // The date/time that the torrent file was authored
            GekkoFyre::DownloadStatus dlStatus;     // Status of the downloading file(s) in question
            std::string comment;                    // Any comments left by the author of the torrent file in question
            std::string creator;                    // The author of the torrent file in question
            std::string magnet_uri;                 // The 'BitTorrent Magnet Link' URI
            std::string torrent_name;               // Name of the torrent
            int num_files;                          // How many files are contained within this torrent
            int num_trackers;                       // How many trackers are contained within this torrent
            int num_pieces;                         // How many pieces are contained within this torrent
            int piece_length;                       // The length of each piece
        };

        struct TorrentInfo {
            GeneralInfo general;
            boost::optional<GekkoFyre::GkTorrent::TorrentResumeInfo> to_resume_info;
            std::vector<std::pair<std::string, int>> nodes;
            std::vector<TorrentTrackers> trackers;
            std::vector<TorrentFile> files_vec;
        };
    }

    namespace GkCurl {
        // http://stackoverflow.com/questions/18031357/why-the-constructor-of-stdostream-is-protected
        struct FileStream {
            std::string file_loc;   // Name to store file as if download /and/ disk writing is successful
            std::shared_ptr<std::ofstream> astream; // I/O stream
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

        struct [[deprecated("use 'Global::DownloadInfo' instead, which is more universal")]] CurlProgressPtr {
            CURL *curl;                    // Easy interface pointer
            std::vector<CurlDlStats> stat; // Download statistics struct
            std::string url;               // The URL in question
            std::string file_dest;         // The destination of where the download is being saved to disk
            bool timer_set;                // Whether the timer, 'timer_begin' has been set for this object or not
            double content_length;         // The file size of the download, as given by the web-server
        };

        struct CurlDlInfo {
            std::string file_loc;                // The location of the downloaded file being streamed towards
            long long insert_timestamp;          // The date/time of the download/file having been inserted into the history file
            long long complt_timestamp;          // The date/time of the download/file having completed transfer
            GekkoFyre::DownloadStatus dlStatus;  // Status of the downloading file(s) in question
            CurlInfoExt ext_info;                // Extended info about the file(s) themselves
            std::string unique_id;               // A unique identifier for this download
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
        };
    }

    namespace GkGraph {
        struct DownSpeedGraph {
            QPointer<QtCharts::QLineSeries> down_speed_series;
            bool down_speed_init;
            std::vector<std::pair<double, double>> down_speed_vals;
        };

        struct GkXferStats {
            boost::optional<double> upload_rate;   // The current upload transfer rate, in bytes per second.
            double download_rate;                  // The current download transfer rate, in bytes per second.
            long progress_ppm;                     // Reflects progress, but instead [0, 1000000] (ppm = parts per million).
            long download_total;                   // The total amount that has been downloaded, in bytes.
            boost::optional<long> upload_total;    // The total amount that has been uploaded, in bytes.
            boost::optional<int> num_pieces_dled;  // The number of BitTorrent pieces that have been downloaded. Can be used to see if the BitTorrent download has updated.
            boost::optional<std::time_t> cur_time; // The current time these statistics were 'snapshotted'.
        };

        struct GkDlStats {
            GekkoFyre::DownloadStatus dl_state;  // The state of the download, i.e. whether it's paused or actively transferring data
            std::time_t timer_begin;             // The time since epoch at which the timer begun (for charting facilities)
            double content_length;               // The file size of the download, as given by the web-server
            std::vector<GkXferStats> xfer_stats; // The all important transfer statistics of the download in question
        };
    }

    namespace Global {
        struct DownloadInfo {
            DownloadType dl_type;                            // The type of download in question, whether HTTP(S)/FTP(S) or BitTorrent
            QString dl_dest;                                 // The location of the download on local storage
            QString unique_id;                               // Unique ID, if any, for the download in question
            QString url;                                     // The URL of the download in question, whether it be the magnet link or a HTTP URL
            GkGraph::GkDlStats stats;                        // Statistics relating to the download in question
            GkGraph::DownSpeedGraph xfer_graph;              // The 'download speed' graph
            boost::optional<GkTorrent::TorrentInfo> to_info; // Information relating to BitTorrent downloads
            boost::optional<GkCurl::CurlDlInfo> curl_info;   // Information relating to HTTP(S) or FTP(S) downloads
        };
    }
}

#endif // GK_DEFVAR_HPP
