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
#include <libtorrent/entry.hpp>
#include <pugixml.hpp>
#include <string>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <QString>
#include <QObject>
#include <QMutex>
#include <QStorageInfo>
#include <QCryptographicHash>
#include <QLayout>
#include <QMultiMap>
#include <initializer_list>

extern "C" {
#include <sys/stat.h>
}

namespace GekkoFyre {

class CmnRoutines : public QObject
{
    Q_OBJECT

public:
    CmnRoutines();
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

    void print_exception(const std::exception &e, int level = 0);
    bool singleAppInstance_Win32();
    static std::string createId(const size_t &id_length);

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

    static std::string findCfgFile(const std::string &cfgFileName);
    static long getFileSize(const std::string &file_name);
    unsigned long int freeDiskSpace(const QString &path = QDir::rootPath());
    GekkoFyre::GkFile::FileHash cryptoFileHash(const QString &file_dest, const GekkoFyre::HashType &hash_type,
                                               const QString &given_hash_val);
    GekkoFyre::GkTorrent::TorrentInfo torrentFileInfo(const std::string &file_dest,
                                                      const int &item_limit = 500000,
                                                      const int &depth_limit = 1000);

    GekkoFyre::GkFile::FileDb openDatabase(const std::string &dbFile);
    bool batch_write_single_db(const std::string &key, const std::string &value, const std::string &unique_id,
                               const GekkoFyre::GkFile::FileDb &file_db_struct);
    GekkoFyre::GkFile::FileDbVal read_database(const std::string &key,
                                               const std::string &unique_id,
                                               const std::string &dbFileName = CFG_HISTORY_DB_FILE);
    std::vector<GekkoFyre::GkFile::FileDbVal> read_db_vec(const std::string &key, const GekkoFyre::GkFile::FileDb &file_db_struct);
    std::pair<std::string, std::string> read_db_min(const std::string &key, const GekkoFyre::GkFile::FileDb &file_db_struct);

    void clearLayout(QLayout *layout);

    std::vector<GekkoFyre::GkCurl::CurlDlInfo> readDownloadInfo(const std::string &xmlCfgFile = CFG_HISTORY_FILE, const bool &hashesOnly = false);
    bool writeDownloadItem(GekkoFyre::GkCurl::CurlDlInfo &dl_info_list);
    pugi::xml_node createNewXmlFile(const std::string &xmlCfgFile = CFG_HISTORY_FILE);
    bool delDownloadItem(const QString &file_dest, const std::string &xmlCfgFile = CFG_HISTORY_FILE);
    bool modifyDlState(const std::string &file_loc, const DownloadStatus &status,
                       const std::string &hash_checksum = "",
                       const GekkoFyre::HashVerif &ret_succ_type = GekkoFyre::HashVerif::Analyzing,
                       const GekkoFyre::HashType &hash_type = GekkoFyre::HashType::None,
                       const long long &complt_timestamp = 0,
                       const std::string &xmlCfgFile = CFG_HISTORY_FILE);

    bool writeTorrentItem(GekkoFyre::GkTorrent::TorrentInfo &gk_ti);
    std::vector<GekkoFyre::GkTorrent::TorrentInfo> readTorrentInfo(const bool &minimal_readout = false);
    bool delTorrentItem(const std::string &unique_id, const std::string &xmlCfgFile = CFG_HISTORY_FILE);

    short writeXmlSettings(const GekkoFyre::GkSettings::FyreDL &settings,
                           const std::string &xmlCfgFile = CFG_SETTINGS_FILE);
    bool modifyXmlSettings(const GekkoFyre::GkSettings::FyreDL &settings,
                           const std::string &xmlCfgFile = CFG_SETTINGS_FILE);

private:
    int load_file(const std::string &filename, std::vector<char> &v,
                  libtorrent::error_code &ec, int limit = 8000000);
    std::string multipart_key(const std::initializer_list<std::string> args);

    bool writeTorrent_extra_data(const GekkoFyre::GkTorrent::TorrentInfo &gk_ti, const GekkoFyre::GkFile::FileDb &ext_db_struct);
    void batch_write_to_file_db(const std::vector<GekkoFyre::GkTorrent::TorrentFile> &to_files_vec,
                                const GekkoFyre::GkFile::FileDb &ext_db_struct);

    QMap<std::string, std::string> process_db(std::initializer_list<std::tuple<std::string, std::string>> args);
    std::vector<GekkoFyre::GkTorrent::GeneralInfo> process_db_map(const QMap<std::string, std::string> &map,
                                                                  std::initializer_list<std::string> args);
    std::vector<GekkoFyre::GkFile::FileDbVal> process_db_xml(const std::string &xml_input);

    QMutex mutex;
};
}

#endif // CMNROUTINES_HPP
