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
 * @file cmnroutines.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief Commonly encountered routines that are used throughout this application are defined here.
 */

#ifndef CMNROUTINES_HPP
#define CMNROUTINES_HPP

#include "default_var.hpp"
#include <libtorrent/entry.hpp>
#include <string>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <initializer_list>
#include <unordered_map>
#include <QString>
#include <QObject>
#include <QMutex>
#include <QStorageInfo>
#include <QCryptographicHash>
#include <QLayout>
#include <QMultiMap>

extern "C" {
#include <sys/stat.h>
}

namespace GekkoFyre {

class CmnRoutines : public QObject
{
    Q_OBJECT

public:
    CmnRoutines(const GekkoFyre::GkFile::FileDb &database, QObject *parent = 0);
    ~CmnRoutines();

    QString extractFilename(const QString &url);
    QString bytesToKilobytes(const double &value);
    QString bytesToMegabytes(const double &value);
    QString numberSeperators(const QVariant &value);
    QString numberConverter(const double &value);
    double percentDownloaded(const double &content_length, const double &amountDl);
    double estimatedTimeLeft(const double &content_length, const double &amountDl,
                             const double &down_speed);
    QString timeBeautify(const double &secondsToConvert);

    int convDlStat_toInt(const GekkoFyre::DownloadStatus &status);
    int convHashType_toInt(const GekkoFyre::HashType &hash_type);
    int convHashVerif_toInt(const GekkoFyre::HashVerif &hash_verif);
    int convDownType_toInt(const GekkoFyre::DownloadType &down_type);
    GekkoFyre::HashType convHashType_IntToEnum(const int &t);
    GekkoFyre::HashVerif convHashVerif_IntToEnum(const int &v);
    QCryptographicHash::Algorithm convHashType_toAlgo(const GekkoFyre::HashType &hash_type);
    static GekkoFyre::DownloadStatus convDlStat_IntToEnum(const int &s);
    QString convDlStat_toString(const GekkoFyre::DownloadStatus &status);
    GekkoFyre::DownloadStatus convDlStat_StringToEnum(const QString &status);
    GekkoFyre::HashType convHashType_StringToEnum(const QString &hashType);
    QString convHashType_toString(const GekkoFyre::HashType &hash_type);
    GekkoFyre::DownloadType convDownType_StringToEnum(const QString &down_type);
    GekkoFyre::DownloadType convDownType_IntToEnum(const int &down_int);

    static long getFileSize(const std::string &file_name);
    unsigned long int freeDiskSpace(const QString &path = QDir::rootPath());
    GekkoFyre::GkFile::FileHash cryptoFileHash(const QString &file_dest, const GekkoFyre::HashType &hash_type,
                                               const QString &given_hash_val);
    GekkoFyre::GkTorrent::TorrentInfo torrentFileInfo(const std::string &file_dest,
                                                      const int &item_limit = 500000,
                                                      const int &depth_limit = 1000);

    std::string createId(const size_t &id_length = FYREDL_UNIQUE_ID_DIGIT_COUNT);
    GekkoFyre::GkFile::FileDb openDatabase(const std::string &dbFile = CFG_HISTORY_DB_FILE);
    void leveldb_lock_remove(const std::string &dbFile = CFG_HISTORY_DB_FILE) noexcept;

    std::string leveldb_location(const std::string &dbFile = CFG_HISTORY_DB_FILE) noexcept;
    void add_item_db(const std::string download_id, const std::string &key, std::string value,
                     const GekkoFyre::GkFile::FileDb &db_struct);
    void del_item_db(const std::string download_id, const std::string &key, const GekkoFyre::GkFile::FileDb &db_struct);
    std::string read_item_db(const std::string download_id, const std::string &key, const GekkoFyre::GkFile::FileDb &db_struct);
    std::pair<std::string, bool> determine_download_id(const std::string &file_path, const GekkoFyre::GkFile::FileDb &db_struct);
    std::unordered_map<std::string, std::pair<std::string, bool>> extract_download_ids(const GekkoFyre::GkFile::FileDb &db_struct,
                                                                         const bool &torrentsOnly = false);

    std::vector<GekkoFyre::GkCurl::CurlDlInfo> readCurlItems(const bool &hashesOnly = false);
    bool addCurlItem(GekkoFyre::GkCurl::CurlDlInfo &dl_info_list);
    bool delCurlItem(const QString &file_dest, const std::string &unique_id_backup = "");
    bool modifyCurlItem(const std::string &file_loc, const GekkoFyre::DownloadStatus &status,
                        const long long &complt_timestamp = 0,
                        const std::string &hash_given = "",
                        const GekkoFyre::HashType &hash_type = GekkoFyre::HashType::None,
                        const std::string &hash_rtrnd = "",
                        const GekkoFyre::HashVerif &ret_succ_type = GekkoFyre::HashVerif::NotApplicable);
    bool modifyTorrentItem(const std::string &unique_id, const GekkoFyre::DownloadStatus &dl_status);

    bool addTorrentItem(GekkoFyre::GkTorrent::TorrentInfo &gk_ti);
    std::vector<GekkoFyre::GkTorrent::TorrentInfo> readTorrentItems(const bool &minimal_readout = false);
    bool delTorrentItem(const std::string &unique_id);

private:
    int load_file(const std::string &filename, std::vector<char> &v,
                  libtorrent::error_code &ec, int limit = 8000000);

    bool convertBool_fromInt(const int &value) noexcept;
    std::string multipart_key(const std::initializer_list<std::string> &args);
    std::string add_download_id(const std::string &file_path, const GekkoFyre::GkFile::FileDb &db_struct,
                                const bool &is_torrent = false, const std::string &override_unique_id = "");
    bool del_download_id(const std::string &unique_id, const GekkoFyre::GkFile::FileDb &db_struct,
                         const bool &is_torrent = false);

    bool write_torrent_files_addendum(std::vector<GekkoFyre::GkTorrent::TorrentFile> &to_files_vec,
                                      const std::string &download_key, const GekkoFyre::GkFile::FileDb &db_struct) noexcept;
    std::vector<GkTorrent::TorrentFile> read_torrent_files_addendum(const int &num_files, const std::string &download_key,
                                                                    const GekkoFyre::GkFile::FileDb &db_struct);
    bool write_torrent_trkrs_addendum(const std::vector<GekkoFyre::GkTorrent::TorrentTrackers> &to_trackers_vec,
                                      const std::string &download_key, const GekkoFyre::GkFile::FileDb &db_struct) noexcept;
    std::vector<GkTorrent::TorrentTrackers> read_torrent_trkrs_addendum(const int &num_trackers, const std::string &download_key,
                                                                        const GekkoFyre::GkFile::FileDb &db_struct);

    GekkoFyre::GkFile::FileDb db;
    QMutex db_mutex;
    QMutex to_info_mutex;
    QMutex mutex;
};
}

#endif // CMNROUTINES_HPP
