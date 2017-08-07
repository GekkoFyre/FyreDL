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

#include "cmnroutines.hpp"
#include "default_var.hpp"
#include "../utils/fast-cpp-csv-parser/csv.h"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/cache.h>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <libtorrent/bdecode.hpp>
#include <libtorrent/announce_entry.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/hex.hpp>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <random>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QByteArray>
#include <QLocale>
#include <QApplication>
#include <QtCore/QDateTime>
#include <QMessageBox>
#include <QMutexLocker>
#include <QTextCodec>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x06000100
#include <SDKDDKVer.h>
#include <Windows.h>

extern "C" {
#include <stdlib.h>
}

#elif __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

extern "C" {
#include <sys/statvfs.h>
}

#else
#error "Platform not supported!"
#endif

using namespace libtorrent;
namespace sys = boost::system;
namespace fs = boost::filesystem;

GekkoFyre::CmnRoutines::CmnRoutines(QObject *parent) : QObject(parent)
{
    setlocale (LC_ALL, "");
}

GekkoFyre::CmnRoutines::~CmnRoutines(   )
{
    // curl_global_cleanup(); // We're done with libcurl, globally, so clean it up!
}

/**
 * @brief GekkoFyre::CmnRoutines::extractFilename
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-09
 * @note   <http://stackoverflow.com/questions/39647342/extracting-the-filename-off-a-url-with-qregexp>
 * @param url
 * @return
 */
QString GekkoFyre::CmnRoutines::extractFilename(const QString &url)
{
    QUrl url_parse = url;
    return url_parse.fileName();
}

/**
 * @brief GekkoFyre::CmnRoutines::bytesToKilobytes
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-09
 * @note   <http://stackoverflow.com/questions/7276826/c-format-number-with-commas>
 * @param content_length
 * @return
 */
QString GekkoFyre::CmnRoutines::bytesToKilobytes(const double &value)
{
    std::ostringstream oss;
    oss << numberSeperators(std::round(value / 1024)).toStdString() << " KB";
    return QString::fromStdString(oss.str());
}

QString GekkoFyre::CmnRoutines::bytesToMegabytes(const double &value)
{
    std::ostringstream oss;
    oss << numberSeperators(std::round((value / 1024) / 1024)).toStdString() << " MB";
    return QString::fromStdString(oss.str());
}

QString GekkoFyre::CmnRoutines::numberSeperators(const QVariant &value)
{
    QLocale locale;
    locale.setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    QString formattedNum = locale.toString(value.toDouble(), 'f', 0);
    return formattedNum;
}

/**
 * @brief GekkoFyre::CmnRoutines::percentDownloaded
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @param content_length
 * @param amountDl
 * @return
 */
double GekkoFyre::CmnRoutines::percentDownloaded(const double &content_length, const double &amountDl)
{
    double percent = (double)std::round((amountDl / content_length) * 100);
    if (percent >= 101) {
        std::cerr << tr("Incorrect download percentage reported!").toStdString() << std::endl;
        return 0;
    }

    return percent;
}

/**
 * @brief GekkoFyre::CmnRoutines::estimatedTimeLeft estimates the amount of time left on a transfer that is downloading.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-05
 * @param content_length The file-size of the download in question.
 * @param amountDl The current downloaded amount.
 * @param down_speed The current transfer rate on the download.
 * @return The time in seconds until the transfer is finished.
 */
double GekkoFyre::CmnRoutines::estimatedTimeLeft(const double &content_length, const double &amountDl,
                                                 const double &down_speed)
{
    double timeLeft = ((content_length - amountDl) / down_speed);
    return timeLeft;
}

QString GekkoFyre::CmnRoutines::timeBeautify(const double &secondsToConvert)
{
    std::ostringstream oss;
    double convertedTime = 0;
    if (secondsToConvert > 60) {
        // Minutes
        convertedTime = (secondsToConvert / 60);
        oss << std::setprecision(FYREDL_EST_WAIT_TIME_PRECISION) << convertedTime << " " << tr("minutes").toStdString();
    } else if (secondsToConvert > (60 * 60)) {
        // Hours
        convertedTime = (secondsToConvert / (60 * 60));
        oss << std::setprecision(FYREDL_EST_WAIT_TIME_PRECISION) << convertedTime << " " << tr("hours").toStdString();
    } else if (secondsToConvert > ((60 * 60) * 24)) {
        // Days
        convertedTime = (secondsToConvert / ((60 * 60) * 24));
        oss << std::setprecision(FYREDL_EST_WAIT_TIME_PRECISION) << convertedTime << " " << tr("days").toStdString();
    }

    return QString::fromStdString(oss.str());
}

/**
 * @brief GekkoFyre::CmnRoutines::print_exception_qmsgbox recursively prints out all the exceptions from the
 * immediate cause.
 * @date 2016-11-12
 * @note <http://en.cppreference.com/w/cpp/error/throw_with_nested>
 * @param e
 * @param level
 */
void GekkoFyre::CmnRoutines::print_exception(const std::exception &e, int level)
{
    std::cerr << std::string((unsigned long)level, ' ') << tr("exception: ").toStdString() << e.what() << std::endl;
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &e) {
        print_exception(e, (level + 1));
    } catch (...) {}
}

/**
 * @brief GekkoFyre::CmnRoutines::createId generates a unique ID and returns the value.
 * @date 2016-12-12
 * @note <http://stackoverflow.com/questions/13445688/how-to-generate-a-random-number-in-c>
 * @return The uniquely generated ID.
 */
std::string GekkoFyre::CmnRoutines::createId(const size_t &id_length) noexcept
{
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist10(0,9);
    std::ostringstream oss;

    for (size_t i = 0; i < (id_length - 1); ++i) {
        oss << dist10(rng);
    }

    return oss.str();
}

/**
 * @brief GekkoFyre::CmnRoutines::openDatabase
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-08
 * @note <http://www.lmdb.tech/doc/>
 *       <http://doc.qt.io/qt-5/qdir.html>
 * @param dbFileName
 * @return
 */
GekkoFyre::GkFile::FileDb GekkoFyre::CmnRoutines::openDatabase(const std::string &dbFileName)
{
    leveldb::Status s;
    GekkoFyre::GkFile::FileDb db_struct;
    db_struct.options.create_if_missing = true;
    std::shared_ptr<leveldb::Cache>(db_struct.options.block_cache).reset(leveldb::NewLRUCache(LEVELDB_CFG_CACHE_SIZE));
    db_struct.options.compression = leveldb::CompressionType::kSnappyCompression;
    if (!dbFileName.empty()) {
        std::string db_location = leveldb_location(dbFileName);
        sys::error_code ec;
        bool doesExist;
        doesExist = !fs::exists(db_location, ec) ? false : true;

        leveldb::DB *raw_db_ptr;
        s = leveldb::DB::Open(db_struct.options, db_location, &raw_db_ptr);
        db_struct.db.reset(raw_db_ptr);
        if (!s.ok()) {
            throw std::runtime_error(tr("Unable to open/create database! %1").arg(QString::fromStdString(s.ToString())).toStdString());
        }

        if (fs::exists(db_location, ec) && fs::is_directory(db_location) && !doesExist) {
            std::cout << tr("Database object created. Status: ").toStdString() << s.ToString() << std::endl;
        }
    }

    return db_struct;
}

/**
 * @brief GekkoFyre::CmnRoutines::leveldb_location determines the home directory and where to put the database files,
 * depending on whether this is a Linux or Microsoft Windows operating system that FyreDL is running on.
 * @param dbFile
 * @return
 */
std::string GekkoFyre::CmnRoutines::leveldb_location(const std::string &dbFile) noexcept
{
    fs::path home_dir(QDir::homePath().toStdString());
    std::ostringstream oss_db_dir;
    std::ostringstream oss_db_file;
    if (fs::is_directory(home_dir)) {
        #ifdef __linux__
        oss_db_dir << home_dir.string() << fs::path::preferred_separator << CFG_FILES_DIR_LINUX;
        #elif _WIN32
        oss_db_dir << home_dir.string() << fs::path::preferred_separator << CFG_FILES_DIR_WNDWS;
        #endif

        if (!fs::is_directory(oss_db_dir.str())) {
            bool succ_dir = fs::create_directory(oss_db_dir.str());
            if (!succ_dir) {
                QMessageBox::warning(nullptr, tr("Error!"), tr("Unable to create directory, \"%1\".\n\nPermissions problem?")
                        .arg(QString::fromStdString(oss_db_dir.str())));
                return "";
            }
        }

        oss_db_file << oss_db_dir.str() << fs::path::preferred_separator << dbFile;
    } else {
        QMessageBox::warning(nullptr, tr("Error!"), tr("Unable to find home directory!"), QMessageBox::Ok);
        return "";
    }

    return oss_db_file.str();
}

/**
 * @brief GekkoFyre::CmnRoutines::leveldb_lock_remove will detect if a database lock is in place and pose a question to
 * the user, asking if they want to remove it.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-22
 * @param dbFile The location of the database on the user's local storage.
 */
void GekkoFyre::CmnRoutines::leveldb_lock_remove(const std::string &dbFile) noexcept
{
    std::string db_location = leveldb_location(dbFile);
    std::ostringstream oss_lock;
    oss_lock << db_location << fs::path::preferred_separator << LEVELDB_CFG_LOCK_FILE_NAME;
    if (fs::exists(oss_lock.str()) && fs::is_regular_file(oss_lock.str())) {
        QMessageBox msg_box;
        msg_box.setText(tr("A lock file is in-place for the LevelDB database! You must delete it before "
                           "proceeding to use FyreDL. Continue with this action?"));
        msg_box.setWindowTitle(tr("Warning!"));
        msg_box.setStandardButtons(QMessageBox::Apply | QMessageBox::Abort | QMessageBox::Cancel);
        msg_box.setDefaultButton(QMessageBox::Apply);
        int ret = msg_box.exec();
        switch (ret) {
            case QMessageBox::Apply:
            {
                bool rem_ret = fs::remove(oss_lock.str());
                if (!rem_ret) {
                    QMessageBox::warning(nullptr, tr("Error!"), tr("Removal of the database lock proved unsuccessful. Aborting..."),
                                         QMessageBox::Ok);
                    QCoreApplication::exit(-1);
                }
            }
                break;
            case QMessageBox::Abort:
            {
                QCoreApplication::exit(-1);
            }
                break;
            case QMessageBox::Cancel:
            {
                QCoreApplication::exit(-1);
            }
                break;
            default:
            {
                QCoreApplication::exit(-1);
            }
                break;
        }
    }

    return;
}

bool GekkoFyre::CmnRoutines::convertBool_fromInt(const int &value) {
    bool bool_convert;
    switch (value) {
        case 0:
            bool_convert = false;
        case 1:
            bool_convert = true;
        default:
            throw std::invalid_argument(tr("An invalid integer was provided to convert from to a boolean value.").toStdString());
    }

    return bool_convert;
}

/**
 * @brief GekkoFyre::CmnRoutines::add_download_id adds a Unique ID (key) to the Google LevelDB database as a key-value store, along
 * with the file-path of the download's location (value) on the user's local storage.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-03
 * @param file_path The value to be stored.
 * @param db_struct The database connection object.
 * @return The Unique ID that was created as a result of this function's action.
 */
std::string GekkoFyre::CmnRoutines::add_download_id(const std::string &file_path, const GekkoFyre::GkFile::FileDb &db_struct,
                                                    const bool &is_torrent, const std::string &override_unique_id)
{
    leveldb::ReadOptions read_opt;
    leveldb::Status s;
    read_opt.verify_checksums = true;
    std::string key;

    if (!override_unique_id.empty()) {
        key = override_unique_id;
    } else {
        key = createId(FYREDL_UNIQUE_ID_DIGIT_COUNT);
    }

    std::string csv_read_data;
    s = db_struct.db->Get(read_opt, LEVELDB_STORE_UNIQUE_ID, &csv_read_data);
    if (!s.ok()) {
        throw std::runtime_error(s.ToString());
    }

    std::stringstream csv_out;
    if (!csv_read_data.empty() && csv_read_data.size() > CFG_XML_MIN_PARSE_SIZE) {
        QMap<std::string, std::pair<std::string, bool>> cache;
        io::CSVReader<LEVELDB_CSV_UNIQUE_ID_COLS> csv_in(csv_read_data);
        csv_in.read_header(io::ignore_no_column, LEVELDB_CSV_UID_KEY, LEVELDB_CSV_UID_VALUE1, LEVELDB_CSV_UID_VALUE2); // If a column with a name is not in the file but is in the argument list, then read_row will not modify the corresponding variable.
        if (!csv_in.has_column(LEVELDB_CSV_UID_KEY) || !csv_in.has_column(LEVELDB_CSV_UID_VALUE1) ||
                !csv_in.has_column(LEVELDB_CSV_UID_VALUE2)) {
            throw std::invalid_argument(tr("Information provided from database is invalid!").toStdString());
        }

        std::string unique_id, path;
        int is_torrent_csv;
        while (csv_in.read_row(unique_id, path, is_torrent_csv)) {
            if (!cache.contains(unique_id)) {
                cache.insert(unique_id, std::make_pair(path, convertBool_fromInt(is_torrent_csv)));
            } else {
                std::cerr << tr("Unique ID already exists in database! Creating new Unique ID...").toStdString() << std::endl;
                return add_download_id(file_path, db_struct, is_torrent, override_unique_id);
            }
        }

        csv_out << LEVELDB_CSV_UID_KEY << "," << LEVELDB_CSV_UID_VALUE1 << std::endl;
        for (const auto &item: cache.toStdMap()) {
            csv_out << item.first << ",";
            csv_out << item.second.first << ",";
            csv_out << item.second.second << std::endl;
        }
    }

    csv_out << key << "," << file_path << "," << is_torrent <<std::endl;

    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::WriteBatch batch;
    batch.Delete(LEVELDB_STORE_UNIQUE_ID);
    batch.Put(LEVELDB_STORE_UNIQUE_ID, csv_out.str());
    s = db_struct.db->Write(write_options, &batch);
    if (!s.ok()) {
        throw std::runtime_error(s.ToString());
    }

    return key;
}

bool GekkoFyre::CmnRoutines::del_download_id(const std::string &unique_id, const GekkoFyre::GkFile::FileDb &db_struct,
                                             const bool &is_torrent) noexcept
{
    auto download_ids = extract_download_ids(db_struct, is_torrent);
    std::ostringstream csv_data;
    for (const auto &id: download_ids.toStdMap()) {
        if (id.first != unique_id) {
            csv_data << id.first << ",";
            csv_data << id.second.first << ",";
            csv_data << id.second.second << std::endl;
        }
    }

    leveldb::Status s;
    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::WriteBatch batch;
    batch.Delete(LEVELDB_STORE_UNIQUE_ID);
    batch.Put(LEVELDB_STORE_UNIQUE_ID, csv_data.str());
    s = db_struct.db->Write(write_options, &batch);
    if (!s.ok()) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("There was an issue while deleting Unique ID, \"%1\", from the "
                                                               "database. See below.\n\n%2")
                .arg(QString::fromStdString(unique_id)).arg(QString::fromStdString(s.ToString())), QMessageBox::Ok);
        return false;
    }

    return true;
}

void GekkoFyre::CmnRoutines::add_item_db(const std::string download_id, const std::string &key, const std::string &value,
                                         const GekkoFyre::GkFile::FileDb &db_struct)
{
    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::WriteBatch batch;
    std::string key_joined = std::string(download_id + "_" + key);
    batch.Delete(key_joined);
    batch.Put(key_joined, value);
    leveldb::Status s;
    s = db_struct.db->Write(write_options, &batch);
    if (!s.ok()) {
        throw std::runtime_error(s.ToString());
    }

    return;
}

void GekkoFyre::CmnRoutines::del_item_db(const std::string download_id, const std::string &key,
                                         const GekkoFyre::GkFile::FileDb &db_struct)
{
    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::WriteBatch batch;
    std::string key_joined = std::string(download_id + "_" + key);
    batch.Delete(key_joined);
    leveldb::Status s;
    s = db_struct.db->Write(write_options, &batch);
    if (!s.ok()) {
        throw std::runtime_error(s.ToString());
    }

    return;
}

std::string GekkoFyre::CmnRoutines::read_item_db(const std::string download_id, const GekkoFyre::GkFile::FileDb &db_struct)
{
    leveldb::ReadOptions read_opt;
    leveldb::Status s;
    read_opt.verify_checksums = true;
    std::string read_data;
    s = db_struct.db->Get(read_opt, download_id, &read_data);
    if (!s.ok()) {
        throw std::runtime_error(s.ToString());
    }

    return read_data;
}

/**
 * @brief GekkoFyre::CmnRoutines::determine_download_id determines the Unique ID for a given path of a downloadable item's
 * location on the user's local storage.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-03
 * @param file_path The file path to use in extrapolating the Unique ID.
 * @param db_struct The database object to use in connecting to the Google LevelDB database.
 * @return The found Unique ID along with the type of download in question (i.e. HTTP/FTP or BitTorrent).
 */
std::pair<std::string, bool> GekkoFyre::CmnRoutines::determine_download_id(const std::string &file_path,
                                                          const GekkoFyre::GkFile::FileDb &db_struct)
{
    leveldb::ReadOptions read_opt;
    leveldb::Status s;
    read_opt.verify_checksums = true;

    std::string key = createId(FYREDL_UNIQUE_ID_DIGIT_COUNT);
    std::string csv_read_xml;
    s = db_struct.db->Get(read_opt, LEVELDB_STORE_UNIQUE_ID, &csv_read_xml);
    if (!s.ok()) {
        throw std::runtime_error(s.ToString());
    }

    if (!csv_read_xml.empty() && csv_read_xml.size() > CFG_XML_MIN_PARSE_SIZE) {
        QMap<std::string, std::pair<std::string, bool>> cache;
        io::CSVReader<LEVELDB_CSV_UNIQUE_ID_COLS> csv_in(csv_read_xml);
        csv_in.read_header(io::ignore_no_column, LEVELDB_CSV_UID_KEY, LEVELDB_CSV_UID_VALUE1, LEVELDB_CSV_UID_VALUE2); // If a column with a name is not in the file but is in the argument list, then read_row will not modify the corresponding variable.
        if (!csv_in.has_column(LEVELDB_CSV_UID_KEY) || !csv_in.has_column(LEVELDB_CSV_UID_VALUE1) ||
                !csv_in.has_column(LEVELDB_CSV_UID_VALUE2)) {
            throw std::invalid_argument(tr("Information provided from database is invalid!").toStdString());
        }

        std::string unique_id, path;
        int is_torrent_csv;
        while (csv_in.read_row(unique_id, path, is_torrent_csv)) {
            cache.insert(unique_id, std::make_pair(path, convertBool_fromInt(is_torrent_csv)));
        }

        for (const auto &item: cache.toStdMap()) {
            if (item.second.first == file_path) {
                return std::make_pair(item.first, item.second.second);
            }
        }
    }

    std::cerr << tr("Unable to determine the Unique ID from the given storage path, \"%1\"")
            .arg(QString::fromStdString(file_path)).toStdString() << std::endl;
    return std::make_pair("", false);
}

/**
 * @brief GekkoFyre::CmnRoutines::extract_download_ids extracts all the downloadable Unique IDs from the Google LevelDB database
 * and presents them as a QMap object, containing as the value the path on the local storage to the item and whether
 * it's a HTTP/FTP or BitTorrent downloadable.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-03
 * @param db_struct The database object used to connect to the Google LevelDB database on the user's local storage.
 * @param torrentsOnly Whether you want to only extract BitTorrent Unique IDs or not. If false, will only extract downloadables
 * of the libcurl variant.
 * @return The key is the Unique ID itself, while the value is a pair consisting of the storage path and whether it's a
 * BitTorrent item or not.
 */
QMap<std::string, std::pair<std::string, bool>> GekkoFyre::CmnRoutines::extract_download_ids(const GekkoFyre::GkFile::FileDb &db_struct,
                                                                                             const bool &torrentsOnly)
{
    leveldb::ReadOptions read_opt;
    leveldb::Status s;
    read_opt.verify_checksums = true;

    std::string csv_read_data;
    s = db_struct.db->Get(read_opt, LEVELDB_STORE_UNIQUE_ID, &csv_read_data);
    if (!s.ok()) {
        throw std::runtime_error(s.ToString());
    }

    QMap<std::string, std::pair<std::string, bool>> cache;
    std::stringstream csv_out;
    if (!csv_read_data.empty() && csv_read_data.size() > CFG_XML_MIN_PARSE_SIZE) {
        io::CSVReader<LEVELDB_CSV_UNIQUE_ID_COLS> csv_in(csv_read_data);
        csv_in.read_header(io::ignore_missing_column, LEVELDB_CSV_UID_KEY, LEVELDB_CSV_UID_VALUE1,
                           LEVELDB_CSV_UID_VALUE2); // If a column with a name is not in the file but is in the argument list, then read_row will not modify the corresponding variable.
        if (!csv_in.has_column(LEVELDB_CSV_UID_KEY) || !csv_in.has_column(LEVELDB_CSV_UID_VALUE1) ||
            !csv_in.has_column(LEVELDB_CSV_UID_VALUE2)) {
            throw std::invalid_argument(tr("Information provided from database is invalid!").toStdString());
        }

        std::string unique_id, path;
        int is_torrent_csv;
        while (csv_in.read_row(unique_id, path, is_torrent_csv)) {
            if (torrentsOnly) {
                // Only accept download items marked as 'BitTorrent'
                if (is_torrent_csv) {
                    cache.insert(unique_id, std::make_pair(path, convertBool_fromInt(is_torrent_csv)));
                }
            }

            if (!torrentsOnly) {
                // Only accept download items marked as 'HTTP/FTP'
                if (!is_torrent_csv) {
                    cache.insert(unique_id, std::make_pair(path, convertBool_fromInt(is_torrent_csv)));
                }
            }
        }
    }

    return cache;
}

/**
 * @brief GekkoFyre::CmnRoutines::getFileSize finds the size of a file /without/ having to 'open' it. Portable
 * under both Linux, Apple Macintosh and Microsoft Windows.
 * @note <http://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c>
 *       <http://stackoverflow.com/questions/8991192/check-filesize-without-opening-file-in-c>
 * @param file_name The path of the file that you want to determine the size of.
 * @return The size of the file in question, in bytes.
 */
long GekkoFyre::CmnRoutines::getFileSize(const std::string &file_name)
{
    #ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesEx(file_name.c_str(), GetFileExInfoStandard, &fad)) {
        return -1; // error condition, could call GetLastError to find out more
    }

    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    return (long)size.QuadPart;
    #elif __linux__
    struct stat64 stat_buf;
    int rc = stat64(file_name.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
    #else
    #error "Platform not supported!"
    #endif
}

/**
 * @brief GekkoFyre::CmnRoutines::freeDiskSpace() gives you the free disk-space available to the current user
 * of the system at the given path.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @note <https://msdn.microsoft.com/en-us/library/windows/desktop/aa364937(v=vs.85).aspx>
 *       <http://forums.codeguru.com/showthread.php?424302-disk-space&p=1578378#post1578378>
 * @param path The path to determine the free disk space of.
 * @return A (presumably 64-bit) value determining free disk-space.
 */
unsigned long int GekkoFyre::CmnRoutines::freeDiskSpace(const QString &path)
{
    #ifdef _WIN32
    ULARGE_INTEGER liFreeBytesAvailable;
    ULARGE_INTEGER liTotalNumberOfBytes;
    ULARGE_INTEGER liTotalNumberOfFreeBytes;
    LPCSTR liDirectoryName(path.toStdString().c_str());
    GetDiskFreeSpaceEx(liDirectoryName, &liFreeBytesAvailable, &liTotalNumberOfBytes, &liTotalNumberOfFreeBytes);
    return (unsigned long int)(LONGLONG)liTotalNumberOfFreeBytes.QuadPart;
    #elif __linux__
    struct statvfs64 fiData;
    fs::path boost_path(path.toStdString());
    statvfs64(boost_path.remove_filename().c_str(), &fiData);
    __fsblkcnt64_t freeSpace = (fiData.f_bavail * fiData.f_bsize);
    return (unsigned long int)freeSpace;
    #else
    #error "Platform not supported!"
    #endif
}

/**
 * @brief GekkoFyre::CmnRoutines::cryptoFileHash() will find the given hash (or if unknown, search for it) of a file
 * and return said hash in a hexadecimal format to be used in string-comparisons.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11-19
 * @note <http://doc.qt.io/qt-5/qcryptographichash.html>
 *       <http://doc.qt.io/qt-5/qiodevice.html#OpenModeFlag-enum>
 *       <http://en.cppreference.com/w/cpp/thread/call_once>
 *       <http://en.cppreference.com/w/cpp/thread/once_flag>
 * @param file_dest The destination of the file you wish to hash.
 * @param hash_type The crypto type you wish to use, or if given GekkoFyre::HashType::Analyzing, attempt a search of the
 * correct crypto used.
 * @return The checksum, in hexadecimal format for portability reasons.
 */
GekkoFyre::GkFile::FileHash GekkoFyre::CmnRoutines::cryptoFileHash(const QString &file_dest, const GekkoFyre::HashType &hash_type,
                                                                   const QString &given_hash_val)
{
    GekkoFyre::GkFile::FileHash info;
    QFile f(file_dest);
    fs::path boost_file_path(file_dest.toStdString());
    if (f.exists() && !fs::is_directory(boost_file_path)) {
        if (!f.open(QIODevice::ReadOnly)) {
            // The file is already opened by another application on the system! Throw exception...
            throw std::runtime_error(tr("Unable to continue! The file, \"%1\", is already opened by another application.")
                                             .arg(file_dest).toStdString());
        }

        QByteArray result;
        info.hash_type = hash_type;
        if (hash_type == GekkoFyre::HashType::MD5 || hash_type == GekkoFyre::HashType::SHA1 ||
             hash_type == GekkoFyre::HashType::SHA256||  hash_type == GekkoFyre::HashType::SHA512 ||
             hash_type == GekkoFyre::HashType::SHA3_256 ||  hash_type == GekkoFyre::HashType::SHA3_512) {
            QCryptographicHash hash(convHashType_toAlgo(hash_type));
            if (hash.addData(&f)) {
                result = hash.result();
                info.checksum = result.toHex();
                if (!given_hash_val.isEmpty()) {
                    // There is a given hash to compare against!
                    if (info.checksum == given_hash_val) {
                        // The calculated checksum MATCHES the given hash!
                        info.hash_verif = GekkoFyre::HashVerif::Verified;
                        return info;
                    } else {
                        // The calculated checksum does NOT match the given hash!
                        info.hash_verif = GekkoFyre::HashVerif::Corrupt;
                        return info;
                    }
                } else {
                    // There is no given hash to compare against
                    info.hash_verif = GekkoFyre::HashVerif::NotApplicable;
                    return info;
                }
            }
        } else if (hash_type == GekkoFyre::HashType::None) {
            info.hash_type = GekkoFyre::HashType::None;
            info.hash_verif = GekkoFyre::HashVerif::NotApplicable;
            info.checksum = "";
            return info;
        } else {
            // We need to find the hash-type!
            std::vector<QString> vec_hash_val;
            std::vector<GekkoFyre::HashType> vec_hash_type;
            vec_hash_type.push_back(GekkoFyre::HashType::MD5);
            vec_hash_type.push_back(GekkoFyre::HashType::SHA1);
            vec_hash_type.push_back(GekkoFyre::HashType::SHA256);
            vec_hash_type.push_back(GekkoFyre::HashType::SHA512);
            vec_hash_type.push_back(GekkoFyre::HashType::SHA3_256);
            vec_hash_type.push_back(GekkoFyre::HashType::SHA3_512);
            for (size_t i = 0; i < vec_hash_type.size(); ++i) {
                QCryptographicHash hash(convHashType_toAlgo(vec_hash_type.at(i)));
                if (hash.addData(&f)) {
                    result = hash.result();
                    vec_hash_val.push_back(result.toHex());
                }
            }

            for (size_t i = 0; i < vec_hash_val.size(); ++i) {
                info.checksum = vec_hash_val.at(i);
                info.hash_type = GekkoFyre::HashType::CannotDetermine;
                if (!given_hash_val.isEmpty()) {
                    if (vec_hash_val.at(i) == given_hash_val) {
                        info.hash_verif = GekkoFyre::HashVerif::Verified;
                        return info;
                    }
                }
            }

            info.hash_verif = GekkoFyre::HashVerif::NotApplicable;
            info.hash_type = GekkoFyre::HashType::CannotDetermine;
            info.checksum = "";
            return info;
        }
    }

    info.hash_verif = GekkoFyre::HashVerif::NotApplicable;
    info.hash_type = GekkoFyre::HashType::None;
    info.checksum = "";
    return info;
}

int GekkoFyre::CmnRoutines::load_file(const std::string &filename, std::vector<char> &v,
                                      libtorrent::error_code &ec, int limit)
{
    ec.clear();
    FILE* f = fopen(filename.c_str(), "rb");
    if (f == NULL)
    {
        ec.assign(errno, boost::system::system_category());
        return -1;
    }

    int r = fseek(f, 0, SEEK_END);
    if (r != 0)
    {
        ec.assign(errno, boost::system::system_category());
        fclose(f);
        return -1;
    }
    long s = ftell(f);
    if (s < 0)
    {
        ec.assign(errno, boost::system::system_category());
        fclose(f);
        return -1;
    }

    if (s > limit)
    {
        fclose(f);
        return -2;
    }

    r = fseek(f, 0, SEEK_SET);
    if (r != 0)
    {
        ec.assign(errno, boost::system::system_category());
        fclose(f);
        return -1;
    }

    v.resize(s);
    if (s == 0)
    {
        fclose(f);
        return 0;
    }

    r = fread(&v[0], 1, v.size(), f);
    if (r < 0)
    {
        ec.assign(errno, boost::system::system_category());
        fclose(f);
        return -1;
    }

    fclose(f);

    if (r != s) return -3;

    return 0;
}

/**
 * @note <http://www.rasterbar.com/products/libtorrent/examples.html>
 * @param file_dest
 * @param item_limit
 * @param depth_limit
 * @return
 */
GekkoFyre::GkTorrent::TorrentInfo GekkoFyre::CmnRoutines::torrentFileInfo(const std::string &file_dest,
                                                                          const int &item_limit,
                                                                          const int &depth_limit) noexcept
{
    GekkoFyre::GkTorrent::TorrentInfo gk_torrent_struct;
    gk_torrent_struct.general.comment = "";
    gk_torrent_struct.general.complt_timestamp = 0;
    gk_torrent_struct.general.creatn_timestamp = 0;
    gk_torrent_struct.general.creator = "";
    gk_torrent_struct.general.dlStatus = GekkoFyre::DownloadStatus::Failed;
    gk_torrent_struct.general.down_dest = "";
    gk_torrent_struct.general.insert_timestamp = 0;
    gk_torrent_struct.general.magnet_uri = "";
    gk_torrent_struct.general.num_files = 0;
    gk_torrent_struct.general.num_pieces = 0;
    gk_torrent_struct.general.piece_length = 0;

    std::vector<char> buf;
    error_code ec;
    int ret = load_file(file_dest, buf, ec, 40 * 1000000);
    if (ret == -1) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("File too big, aborting..."), QMessageBox::Ok);
        return gk_torrent_struct;
    }

    if (ret != 0) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("Failed to load file: %1").arg(QString::fromStdString(file_dest)), QMessageBox::Ok);
        return gk_torrent_struct;
    }

    bdecode_node e;
    int pos = -1;
    std::cout << tr("Decoding! Recursion limit: %1. Total item count limit: %2.")
            .arg(item_limit).arg(depth_limit).toStdString() << std::endl;
    ret = bdecode(&buf[0], &buf[0] + buf.size(), e, ec, &pos, depth_limit, item_limit);

    if (ret != 0) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("Failed to decode: '%1' at character: %2")
                .arg(QString::fromStdString(ec.message())).arg(QString::number(pos)), QMessageBox::Ok);
        return gk_torrent_struct;
    }

    torrent_info t(e, ec);
    if (ec) {
        QMessageBox::warning(nullptr, tr("Error!"), QString("%1").arg(QString::fromStdString(ec.message())), QMessageBox::Ok);
        return gk_torrent_struct;
    }

    e.clear();
    std::vector<char>().swap(buf);

    //
    // Translate info about torrent
    //
    typedef std::vector<std::pair<std::string, int>> node_vec;
    const node_vec &nodes = t.nodes();

    // Nodes
    for (node_vec::const_iterator i = nodes.begin(), end(nodes.end()); i != end; ++i) {
        gk_torrent_struct.nodes.push_back(std::make_pair(i->first, i->second));
    }

    // Trackers
    for (std::vector<announce_entry>::const_iterator i = t.trackers().begin(); i != t.trackers().end(); ++i) {
        GekkoFyre::GkTorrent::TorrentTrackers gk_torrent_tracker;
        gk_torrent_tracker.tier = i->tier;
        gk_torrent_tracker.url = i->url;
        gk_torrent_tracker.enabled = true;
        gk_torrent_struct.trackers.push_back(gk_torrent_tracker);
    }

    // std::ostringstream ih;
    // to_hex((char const*)&t.info_hash()[0], 20, ih);
    // ih << std::hex << t.info_hash();

    std::string unique_id = createId(FYREDL_UNIQUE_ID_DIGIT_COUNT);
    gk_torrent_struct.general.num_pieces = t.num_pieces();
    gk_torrent_struct.general.piece_length = t.piece_length();
    gk_torrent_struct.general.comment = t.comment();
    gk_torrent_struct.general.creator = t.creator();
    gk_torrent_struct.general.magnet_uri = make_magnet_uri(t);
    gk_torrent_struct.general.num_files = t.num_files();
    // gk_torrent_struct.creatn_timestamp = t.creation_date().get();
    gk_torrent_struct.general.torrent_name = t.name();
    gk_torrent_struct.general.unique_id = unique_id;

    const file_storage &st = t.files();
    for (int i = 0; i < st.num_files(); ++i) {
        const int first = st.map_file(i, 0, 0).piece;
        const int last = st.map_file(i, (std::max)(boost::int64_t(st.file_size(i))-1, boost::int64_t(0)), 0).piece;
        const int flags = st.file_flags(i);
        const int64_t file_offset = st.file_offset(i);
        const int64_t file_size = st.file_size(i);
        uint32_t mod_time = (uint32_t)st.mtime(i);

        GekkoFyre::GkTorrent::TorrentFile gk_tf;
        std::ostringstream sha1_hex;
        sha1_hex << std::hex << st.hash(i).to_string();
        gk_tf.unique_id = unique_id;
        gk_tf.sha1_hash_hex = sha1_hex.str();
        gk_tf.file_path = st.file_path(i);
        gk_tf.file_offset = file_offset;
        gk_tf.content_length = file_size;
        gk_tf.mtime = mod_time;
        gk_tf.map_file_piece = std::make_pair(first, last);
        gk_tf.flags = flags;
        gk_tf.downloaded = false;
        gk_torrent_struct.files_vec.push_back(gk_tf);
    }

    return gk_torrent_struct;
}

/**
 * @brief GekkoFyre::CmnRoutines::readCurlItems extracts the history information from 'CFG_HISTORY_FILE' relating to
 * HTTP(S) or FTP(S) downloads that have been added to FyreDL. This excludes BitTorrent downloads, which are handled by
 * different functions.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10
 * @param xmlCfgFile is the XML history file in question.
 * @param hashesOnly excludes all the 'extended' information by not loading it into memory.
 * @return A STL standard container holding a struct pertaining to all the needed CURL information is returned.
 */
std::vector<GekkoFyre::GkCurl::CurlDlInfo> GekkoFyre::CmnRoutines::readCurlItems(const bool &hashesOnly)
{
    try {
        std::vector<GekkoFyre::GkCurl::CurlDlInfo> output;
        GekkoFyre::GkFile::FileDb db_struct = openDatabase(CFG_HISTORY_DB_FILE);
        auto download_ids = extract_download_ids(db_struct, false);
        for (auto const &id: download_ids.toStdMap()) {
            if (!id.second.second) { // Therefore not a BitTorrent item
                std::string joined_key = id.first;
                size_t pos = 0;
                std::string delimeter = "_";
                std::string token;
                std::vector<std::string> split_key;
                while ((pos = joined_key.find(delimeter)) != std::string::npos) {
                    token = joined_key.substr(0, pos);
                    split_key.push_back(token);
                    joined_key.erase(0, (pos + delimeter.length()));
                }

                split_key.push_back(joined_key);
                auto db_read = read_item_db(joined_key, db_struct);

                for (const auto &key: split_key) {
                    GekkoFyre::GkCurl::CurlDlInfo dl_info;
                    if (key == LEVELDB_KEY_CURL_STAT) {
                        dl_info.dlStatus = convDlStat_StringToEnum(QString::fromStdString(id.second.first));
                    } else if (key == LEVELDB_KEY_CURL_INSERT_DATE) {
                        dl_info.insert_timestamp = std::atoll(id.second.first.c_str());
                    } else if (key == LEVELDB_KEY_CURL_COMPLT_DATE) {
                        dl_info.complt_timestamp = std::atoll(id.second.first.c_str());
                    } else if (key == LEVELDB_KEY_CURL_STATMSG) {
                        dl_info.ext_info.status_msg = id.second.first;
                    } else if (key == LEVELDB_KEY_CURL_EFFEC_URL) {
                        dl_info.ext_info.effective_url = id.second.first;
                    } else if (key == LEVELDB_KEY_CURL_RESP_CODE) {
                        dl_info.ext_info.response_code = std::atoll(id.second.first.c_str());
                    } else if (key == LEVELDB_KEY_CURL_CONT_LNGTH) {
                        dl_info.ext_info.content_length = std::atof(id.second.first.c_str());
                    } else if (key == LEVELDB_KEY_CURL_HASH_TYPE) {
                        dl_info.hash_type = convHashType_StringToEnum(QString::fromStdString(id.second.first));
                    } else if (key == LEVELDB_KEY_CURL_HASH_VAL_GIVEN) {
                        dl_info.hash_val_given = id.second.first;
                    } else if (key == LEVELDB_KEY_CURL_HASH_VAL_RTRND) {
                        dl_info.hash_val_rtrnd = id.second.first;
                    } else if (key == LEVELDB_KEY_CURL_HASH_SUCC_TYPE) {
                        // TODO: Fix this so it actually reads from the database!
                        dl_info.hash_succ_type = GekkoFyre::HashVerif::NotApplicable;
                    } else {
                        // TODO: Make sure this actually reads as the file-location!
                        dl_info.file_loc = id.second.first;
                    }

                    output.push_back(dl_info);
                }
            }
        }

        return output;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return std::vector<GekkoFyre::GkCurl::CurlDlInfo>();
}

/**
 * @brief GekkoFyre::CmnRoutines::addCurlItem writes libcurl related information to a Google LevelDB database on the local
 * disk of the user's system, within the home directory. The information is stored in an XML format.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2017-07-18
 * @param dl_info The download information to add to the database.
 * @return Whether the write operations were successful or not.
 */
bool GekkoFyre::CmnRoutines::addCurlItem(GekkoFyre::GkCurl::CurlDlInfo &dl_info)
{
    if (dl_info.ext_info.status_ok) {
        if (dl_info.dlStatus == GekkoFyre::DownloadStatus::Stopped || dl_info.dlStatus == GekkoFyre::DownloadStatus::Invalid ||
            dl_info.dlStatus == GekkoFyre::DownloadStatus::Unknown) {
            try {
                dl_info.dlStatus = GekkoFyre::DownloadStatus::Unknown;
                QDateTime now = QDateTime::currentDateTime();
                dl_info.insert_timestamp = now.toTime_t();
                dl_info.complt_timestamp = 0;
                dl_info.ext_info.status_msg = "";

                std::string unique_id = dl_info.unique_id;
                GekkoFyre::GkFile::FileDb db_struct = openDatabase(CFG_HISTORY_DB_FILE);
                std::string download_key = add_download_id(dl_info.file_loc, db_struct, false, dl_info.unique_id);
                add_item_db(download_key, LEVELDB_KEY_CURL_STAT, std::to_string(convDlStat_toInt(dl_info.dlStatus)), db_struct);
                add_item_db(download_key, LEVELDB_KEY_CURL_INSERT_DATE, std::to_string(dl_info.insert_timestamp), db_struct);
                add_item_db(download_key, LEVELDB_KEY_CURL_COMPLT_DATE, std::to_string(dl_info.complt_timestamp), db_struct);
                add_item_db(download_key, LEVELDB_KEY_CURL_STATMSG, dl_info.ext_info.status_msg, db_struct);
                add_item_db(download_key, LEVELDB_KEY_CURL_EFFEC_URL, dl_info.ext_info.effective_url, db_struct);
                add_item_db(download_key, LEVELDB_KEY_CURL_RESP_CODE, std::to_string(dl_info.ext_info.response_code), db_struct);
                add_item_db(download_key, LEVELDB_KEY_CURL_CONT_LNGTH, std::to_string(dl_info.ext_info.content_length), db_struct);
                add_item_db(download_key, LEVELDB_KEY_CURL_HASH_TYPE, std::to_string(dl_info.hash_type), db_struct);
                add_item_db(download_key, LEVELDB_KEY_CURL_HASH_VAL_GIVEN, dl_info.hash_val_given, db_struct);
                add_item_db(download_key, LEVELDB_KEY_CURL_HASH_VAL_RTRND, dl_info.hash_val_rtrnd, db_struct);
                add_item_db(download_key, LEVELDB_KEY_CURL_HASH_SUCC_TYPE, std::to_string(dl_info.hash_succ_type), db_struct);
                return true;
            } catch (const std::exception &e) {
                QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
                return false;
            }
        }
    }

    return false;
}

/**
 * @brief GekkoFyre::CmnRoutines::delCurlItem deletes the given downloadable item from the Google LevelDB database.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-21
 * @param file_dest The location of the downloadable item on the user's local storage, along with the folder name of the
 * download. This is used to deduce the Unique ID of the download in question to modify.
 * @param unique_id_backup If the 'file_dest' param happens to be empty but the unique identifier provided instead (i.e. in the
 * event of database corruption), then this may be used as a last resort to delete an item from the database.
 * @return Whether the series of operations to delete the database object proceeded okay or not.
 */
bool GekkoFyre::CmnRoutines::delCurlItem(const QString &file_dest, const std::string &unique_id_backup)
{
    try {
        GekkoFyre::GkFile::FileDb db_struct = openDatabase(CFG_HISTORY_DB_FILE);
        std::string download_id;
        if (unique_id_backup.empty() && !file_dest.isEmpty()) {
            auto identifier = determine_download_id(file_dest.toStdString(), db_struct);
            download_id = identifier.first;
        } else {
            // TODO: An empty 'unique_id_backup' can still possibly be provided, as-is dealt with by the exception below!
            download_id = unique_id_backup;
        }

        if (download_id.empty()) {
            throw std::invalid_argument(tr("An invalid Unique ID has been provided. Unable to delete download item with "
                                                   "storage path, \"%1\".").arg(file_dest).toStdString());
        }

        del_item_db(download_id, LEVELDB_KEY_CURL_STAT, db_struct);
        del_item_db(download_id, LEVELDB_KEY_CURL_INSERT_DATE, db_struct);
        del_item_db(download_id, LEVELDB_KEY_CURL_COMPLT_DATE, db_struct);
        del_item_db(download_id, LEVELDB_KEY_CURL_STATMSG, db_struct);
        del_item_db(download_id, LEVELDB_KEY_CURL_EFFEC_URL, db_struct);
        del_item_db(download_id, LEVELDB_KEY_CURL_RESP_CODE, db_struct);
        del_item_db(download_id, LEVELDB_KEY_CURL_CONT_LNGTH, db_struct);
        del_item_db(download_id, LEVELDB_KEY_CURL_HASH_TYPE, db_struct);
        del_item_db(download_id, LEVELDB_KEY_CURL_HASH_VAL_GIVEN, db_struct);
        del_item_db(download_id, LEVELDB_KEY_CURL_HASH_VAL_RTRND, db_struct);
        del_item_db(download_id, LEVELDB_KEY_CURL_HASH_SUCC_TYPE, db_struct);
        bool ret = del_download_id(download_id, db_struct, false);
        return ret;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return false;
    }

    return false;
}

/**
 * @brief GekkoFyre::CmnRoutines::modifyCurlItem allows the modification of the download state, and will update
 * the Google LevelDB database relevantly.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-21
 * @param file_loc The location of the downloadable item on the user's local storage, along with the folder name of the
 * download. This is used to deduce the Unique ID of the download in question to modify.
 * @param status The status of the download, whether it be 'Paused', 'Stopped', 'Downloading', or something else entirely.
 * @param complt_timestamp The timestamp of when the downloadable item has completed its task of downloading itself.
 * @param hash_given The checksum hash that was given with the downloadable item when it was added to FyreDL.
 * @param hash_type The type of checksum hash that was given with the downloadable item when it was added to FyreDL.
 * @param hash_rtrnd The returned checksum hash that was calculated after the downloadable item had finished downloading.
 * @param ret_succ_type Whether the two checksum hashes, the given and the returned, match at all, therefore marking it as
 * a valid and non-corrupted download.
 * @return Whether the series of operations to modify the database object(s) proceeded okay or not.
 */
bool GekkoFyre::CmnRoutines::modifyCurlItem(const std::string &file_loc, const GekkoFyre::DownloadStatus &status,
                                            const long long &complt_timestamp, const std::string &hash_given,
                                            const GekkoFyre::HashType &hash_type, const std::string &hash_rtrnd,
                                            const GekkoFyre::HashVerif &ret_succ_type)
{
    try {
        GekkoFyre::GkFile::FileDb db_struct = openDatabase(CFG_HISTORY_DB_FILE);
        auto identifier = determine_download_id(file_loc, db_struct);
        if (!identifier.first.empty()) {
            std::string dl_id = identifier.first;

            //
            // General
            add_item_db(dl_id, LEVELDB_KEY_CURL_STAT, std::to_string(convDlStat_toInt(status)), db_struct);
            if (complt_timestamp > 0) {
                add_item_db(dl_id, LEVELDB_KEY_CURL_COMPLT_DATE, std::to_string(complt_timestamp), db_struct);
            }

            //
            // Hash Values
            if (ret_succ_type != GekkoFyre::HashVerif::NotApplicable) {
                add_item_db(dl_id, LEVELDB_KEY_CURL_HASH_TYPE, std::to_string(convHashType_toInt(hash_type)), db_struct);
                add_item_db(dl_id, LEVELDB_KEY_CURL_HASH_VAL_GIVEN, hash_given, db_struct);
                add_item_db(dl_id, LEVELDB_KEY_CURL_HASH_VAL_RTRND, hash_rtrnd, db_struct);
                add_item_db(dl_id, LEVELDB_KEY_CURL_HASH_SUCC_TYPE, std::to_string(convHashVerif_toInt(ret_succ_type)), db_struct);
            }

            return true;
        } else {
            throw std::invalid_argument(tr("An invalid Unique ID has been provided. Unable to modify download item with "
                                                   "storage path, \"%1\".").arg(QString::fromStdString(file_loc)).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return false;
    }

    return false;
}

bool GekkoFyre::CmnRoutines::modifyTorrentItem(const std::string &unique_id, const GekkoFyre::DownloadStatus &dl_status)
{
    try {
        if (!unique_id.empty()) {
            return true;
        }

        return false;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return false;
    }
}

/**
 * @brief GekkoFyre::CmnRoutines::writeTorrentItem writes BitTorrent related information to a Google LevelDB database that is kept on
 * the user's local storage within the home directory. This information is stored in a readibly parsable XML format.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07
 * @param gk_ti The BitTorrent related information to write to the database.
 * @return Whether the write operation proceeded successfully or not.
 */
bool GekkoFyre::CmnRoutines::writeTorrentItem(GekkoFyre::GkTorrent::TorrentInfo &gk_ti)
{
    try {
        GekkoFyre::GkFile::FileDb db_struct = openDatabase(CFG_HISTORY_DB_FILE);
        leveldb::Status s;
        if (!gk_ti.general.down_dest.empty()) {
            if (s.ok()) {
                // Proceed, the database status is 'okay'!
                if (gk_ti.general.dlStatus == GekkoFyre::DownloadStatus::Stopped || gk_ti.general.dlStatus == GekkoFyre::DownloadStatus::Invalid ||
                    gk_ti.general.dlStatus == GekkoFyre::DownloadStatus::Unknown) {
                    QDateTime now = QDateTime::currentDateTime();
                    gk_ti.general.insert_timestamp = now.toTime_t();

                    std::string download_key = add_download_id(gk_ti.general.down_dest, db_struct, true, gk_ti.general.unique_id);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_INSERT_DATE, std::to_string(gk_ti.general.insert_timestamp), db_struct);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_COMPLT_DATE, std::to_string(gk_ti.general.complt_timestamp), db_struct);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_CREATN_DATE, std::to_string(gk_ti.general.creatn_timestamp), db_struct);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_DLSTATUS, convDlStat_toString(gk_ti.general.dlStatus).toStdString(), db_struct);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_TORRNT_COMMENT, gk_ti.general.comment, db_struct);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_TORRNT_CREATOR, gk_ti.general.creator, db_struct);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_MAGNET_URI, gk_ti.general.magnet_uri, db_struct);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_TORRNT_NAME, gk_ti.general.torrent_name, db_struct);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_NUM_FILES, std::to_string(gk_ti.general.num_files), db_struct);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_TORRNT_PIECES, std::to_string(gk_ti.general.num_pieces), db_struct);
                    add_item_db(download_key, LEVELDB_KEY_TORRENT_TORRNT_PIECE_LENGTH, std::to_string(gk_ti.general.piece_length), db_struct);

                    //
                    // Files and Trackers
                    //
                    return true;
                }
            } else {
                // There is a problem with the database!
                throw std::runtime_error(tr("There is an issue with the database! Error: %1").arg(QString::fromStdString(s.ToString())).toStdString());
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return false;
    }

    return false;
}

/**
 * @brief GekkoFyre::CmnRoutines::readTorrentItem extracts all of the users stored history relating to BitTorrent from a
 * Google LevelDB database and parses the XML therein, caching whatever it can in memory to speed up the process.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07
 * @param minimal_readout only extracts the most vital history information, thus (potentially) saving CPU time and
 * memory.
 * @return A STL standard container holding a struct pertaining to all the needed BitTorrent information is returned.
 */
std::vector<GekkoFyre::GkTorrent::TorrentInfo> GekkoFyre::CmnRoutines::readTorrentItem(const bool &minimal_readout)
{
    return std::vector<GekkoFyre::GkTorrent::TorrentInfo>();
}

bool GekkoFyre::CmnRoutines::delTorrentItem(const std::string &unique_id)
{
    return false;
}

int GekkoFyre::CmnRoutines::convDlStat_toInt(const GekkoFyre::DownloadStatus &status)
{
    switch (status) {
    case GekkoFyre::DownloadStatus::Downloading:
        return 0;
    case GekkoFyre::DownloadStatus::Completed:
        return 1;
    case GekkoFyre::DownloadStatus::Failed:
        return 2;
    case GekkoFyre::DownloadStatus::Paused:
        return 3;
    case GekkoFyre::DownloadStatus::Stopped:
        return 4;
    case GekkoFyre::DownloadStatus::Unknown:
        return 5;
    case GekkoFyre::DownloadStatus::Invalid:
        return 6;
    default:
        return -1;
    }
}

int GekkoFyre::CmnRoutines::convHashType_toInt(const GekkoFyre::HashType &hash_type)
{
    switch (hash_type) {
        case GekkoFyre::HashType::MD5:
            return 1;
        case GekkoFyre::HashType::SHA1:
            return 2;
        case GekkoFyre::HashType::SHA256:
            return 3;
        case GekkoFyre::HashType::SHA512:
            return 4;
        case GekkoFyre::HashType::SHA3_256:
            return 5;
        case GekkoFyre::HashType::SHA3_512:
            return 6;
        case GekkoFyre::HashType::None:
            return 0;
        case GekkoFyre::HashType::CannotDetermine:
            return -1;
        default:
            return -1;
    }
}

int GekkoFyre::CmnRoutines::convHashVerif_toInt(const GekkoFyre::HashVerif &hash_verif)
{
    switch (hash_verif) {
        case GekkoFyre::HashVerif::Analyzing:
            return ENUM_GEKKOFYRE_HASH_VERIF_ANALYZING;
        case GekkoFyre::HashVerif::NotApplicable:
            return ENUM_GEKKOFYRE_HASH_VERIF_NOT_APPLIC;
        case GekkoFyre::HashVerif::Verified:
            return ENUM_GEKKOFYRE_HASH_VERIF_VERIFIED;
        case GekkoFyre::HashVerif::Corrupt:
            return ENUM_GEKKOFYRE_HASH_VERIF_CORRUPT;
        default:
            return ENUM_GEKKOFYRE_HASH_VERIF_NOT_APPLIC;
    }
}

int GekkoFyre::CmnRoutines::convDownType_toInt(const GekkoFyre::DownloadType &down_type)
{
    switch (down_type) {
        case GekkoFyre::DownloadType::HTTP:
            return ENUM_GEKKOFYRE_DOWN_TYPE_HTTP;
        case GekkoFyre::DownloadType::FTP:
            return ENUM_GEKKOFYRE_DOWN_TYPE_FTP;
        case GekkoFyre::DownloadType::Torrent:
            return ENUM_GEKKOFYRE_DOWN_TYPE_TORRENT;
        case GekkoFyre::DownloadType::TorrentMagnetLink:
            return ENUM_GEKKOFYRE_DOWN_TYPE_MAGNET_LINK;
        default:
            return ENUM_GEKKOFYRE_DOWN_TYPE_HTTP;
    }
}

GekkoFyre::HashType GekkoFyre::CmnRoutines::convHashType_IntToEnum(const int &t)
{
    switch (t) {
        case 1:
            return GekkoFyre::HashType::MD5;
        case 2:
            return GekkoFyre::HashType::SHA1;
        case 3:
            return GekkoFyre::HashType::SHA256;
        case 4:
            return GekkoFyre::HashType::SHA512;
        case 5:
            return GekkoFyre::HashType::SHA3_256;
        case 6:
            return GekkoFyre::HashType::SHA3_512;
        case 0:
            return GekkoFyre::HashType::None;
        case -1:
            return GekkoFyre::HashType::CannotDetermine;
        default:
            return GekkoFyre::HashType::CannotDetermine;
    }
}

GekkoFyre::HashVerif GekkoFyre::CmnRoutines::convHashVerif_IntToEnum(const int &v)
{
    switch (v) {
        case ENUM_GEKKOFYRE_HASH_VERIF_ANALYZING:
            return GekkoFyre::HashVerif::Analyzing;
        case ENUM_GEKKOFYRE_HASH_VERIF_NOT_APPLIC:
            return GekkoFyre::HashVerif::NotApplicable;
        case ENUM_GEKKOFYRE_HASH_VERIF_VERIFIED:
            return GekkoFyre::HashVerif::Verified;
        case ENUM_GEKKOFYRE_HASH_VERIF_CORRUPT:
            return GekkoFyre::HashVerif::Corrupt;
        default:
            return GekkoFyre::HashVerif::NotApplicable;
    }
}

QCryptographicHash::Algorithm GekkoFyre::CmnRoutines::convHashType_toAlgo(const GekkoFyre::HashType &hash_type)
{
    switch (hash_type) {
        case GekkoFyre::HashType::MD5:
            return QCryptographicHash::Algorithm::Md5;
        case GekkoFyre::HashType::SHA1:
            return QCryptographicHash::Algorithm::Sha1;
        case GekkoFyre::HashType::SHA256:
            return QCryptographicHash::Algorithm::Sha256;
        case GekkoFyre::HashType::SHA512:
            return QCryptographicHash::Algorithm::Sha512;
        case GekkoFyre::HashType::SHA3_256:
            return QCryptographicHash::Algorithm::Sha3_256;
        case GekkoFyre::HashType::SHA3_512:
            return QCryptographicHash::Algorithm::Sha3_512;
        default:
            return QCryptographicHash::Algorithm::Md4; // This hash is /never/ used, so it's a good default!
    }
}

GekkoFyre::DownloadStatus GekkoFyre::CmnRoutines::convDlStat_IntToEnum(const int &s)
{
    GekkoFyre::DownloadStatus ds_enum;
    switch (s) {
    case 0:
        ds_enum = GekkoFyre::DownloadStatus::Downloading;
        return ds_enum;
    case 1:
        ds_enum = GekkoFyre::DownloadStatus::Completed;
        return ds_enum;
    case 2:
        ds_enum = GekkoFyre::DownloadStatus::Failed;
        return ds_enum;
    case 3:
        ds_enum = GekkoFyre::DownloadStatus::Paused;
        return ds_enum;
    case 4:
        ds_enum = GekkoFyre::DownloadStatus::Stopped;
        return ds_enum;
    case 5:
        ds_enum = GekkoFyre::DownloadStatus::Unknown;
        return ds_enum;
    case 6:
        ds_enum = GekkoFyre::DownloadStatus::Invalid;
        return ds_enum;
    default:
        ds_enum = GekkoFyre::DownloadStatus::Unknown;
        return ds_enum;
    }
}

QString GekkoFyre::CmnRoutines::convDlStat_toString(const GekkoFyre::DownloadStatus &status)
{
    switch (status) {
    case GekkoFyre::DownloadStatus::Downloading:
        return tr("Downloading");
    case GekkoFyre::DownloadStatus::Completed:
        return tr("Completed");
    case GekkoFyre::DownloadStatus::Failed:
        return tr("Failed");
    case GekkoFyre::DownloadStatus::Paused:
        return tr("Paused");
    case GekkoFyre::DownloadStatus::Stopped:
        return tr("Stopped");
    case GekkoFyre::DownloadStatus::Unknown:
        return tr("Unknown");
    case GekkoFyre::DownloadStatus::Invalid:
        return tr("Invalid");
    default:
        return tr("Unknown");
    }
}

GekkoFyre::DownloadStatus GekkoFyre::CmnRoutines::convDlStat_StringToEnum(const QString &status)
{
    if (status == tr("Downloading")) {
        return GekkoFyre::DownloadStatus::Downloading;
    } else if (status == tr("Completed")) {
        return GekkoFyre::DownloadStatus::Completed;
    } else if (status == tr("Failed")) {
        return GekkoFyre::DownloadStatus::Failed;
    } else if (status == tr("Paused")) {
        return GekkoFyre::DownloadStatus::Paused;
    } else if (status == tr("Stopped")) {
        return GekkoFyre::DownloadStatus::Stopped;
    } else if (status == tr("Unknown")) {
        return GekkoFyre::DownloadStatus::Unknown;
    } else {
        return GekkoFyre::DownloadStatus::Unknown;
    }
}

GekkoFyre::HashType GekkoFyre::CmnRoutines::convHashType_StringToEnum(const QString &hashType)
{
    if (hashType == QString("MD5")) {
        return GekkoFyre::HashType::MD5;
    } else if (hashType == QString("SHA-1")) {
        return GekkoFyre::HashType::SHA1;
    } else if (hashType == QString("SHA-256")) {
        return GekkoFyre::HashType::SHA256;
    } else if (hashType == QString("SHA-512")) {
        return GekkoFyre::HashType::SHA512;
    } else if (hashType == QString("SHA3-256")) {
        return GekkoFyre::HashType::SHA3_256;
    } else if (hashType == QString("SHA3-512")) {
        return GekkoFyre::HashType::SHA3_512;
    } else if (hashType == tr("None")) {
        return GekkoFyre::HashType::None;
    } else if (hashType == tr("Unknown")) {
        return GekkoFyre::HashType::CannotDetermine;
    } else {
        return GekkoFyre::HashType::CannotDetermine;
    }
}

QString GekkoFyre::CmnRoutines::convHashType_toString(const GekkoFyre::HashType &hash_type)
{
    switch (hash_type) {
        case GekkoFyre::HashType::MD5:
            return QString("MD5");
        case GekkoFyre::HashType::SHA1:
            return QString("SHA-1");
        case GekkoFyre::HashType::SHA256:
            return QString("SHA-256");
        case GekkoFyre::HashType::SHA512:
            return QString("SHA-512");
        case GekkoFyre::HashType::SHA3_256:
            return QString("SHA3-512");
        case GekkoFyre::HashType::SHA3_512:
            return QString("SHA3-512");
        case GekkoFyre::HashType::None:
            return tr("None");
        case GekkoFyre::HashType::CannotDetermine:
            return tr("Unknown");
        default:
            return tr("Unknown");
    }
}

GekkoFyre::DownloadType GekkoFyre::CmnRoutines::convDownType_StringToEnum(const QString &down_type)
{
    if (down_type == QString("HTTP(S)")) {
        return GekkoFyre::DownloadType::HTTP;
    } else if (down_type == QString("FTP(S)")) {
        return GekkoFyre::DownloadType::FTP;
    } else if (down_type == QString(".torrent")) {
        return GekkoFyre::DownloadType::Torrent;
    } else if (down_type == QString("Magnet link (torrent)")) {
        return GekkoFyre::DownloadType::TorrentMagnetLink;
    } else {
        return GekkoFyre::DownloadType::HTTP;
    }
}

GekkoFyre::DownloadType GekkoFyre::CmnRoutines::convDownType_IntToEnum(const int &down_int)
{
    if (ENUM_GEKKOFYRE_DOWN_TYPE_HTTP) {
        return GekkoFyre::DownloadType::HTTP;
    } else if (ENUM_GEKKOFYRE_DOWN_TYPE_FTP) {
        return GekkoFyre::DownloadType::FTP;
    } else if (ENUM_GEKKOFYRE_DOWN_TYPE_TORRENT) {
        return GekkoFyre::DownloadType::Torrent;
    } else if (ENUM_GEKKOFYRE_DOWN_TYPE_MAGNET_LINK) {
        return GekkoFyre::DownloadType::TorrentMagnetLink;
    }

    return GekkoFyre::DownloadType::HTTP;
}

QString GekkoFyre::CmnRoutines::numberConverter(const double &value)
{
    QMutexLocker locker(&mutex);
    locker.relock(); // Should lock automatically though
    if (value < (1024 * 1024)) {
        QString conv = bytesToKilobytes(value);
        return conv;
    } else if (value > (2048 * 1024)) {
        QString conv = bytesToMegabytes(value);
        return conv;
    } else {
        QString conv = bytesToKilobytes(value);
        return conv;
    }
}
