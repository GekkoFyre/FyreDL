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
 * @file cmnroutines.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief Commonly encountered routines that are used throughout this application are defined here.
 */

#include "cmnroutines.hpp"
#include "default_var.hpp"
#include "csv.hpp"
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

GekkoFyre::CmnRoutines::CmnRoutines(const GekkoFyre::GkFile::FileDb &database, QObject *parent) : QObject(parent)
{
    setlocale (LC_ALL, "");

    try {
        db = database;
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        QApplication::exit(-1);
    }

    return;
}

GekkoFyre::CmnRoutines::~CmnRoutines()
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
 * @brief GekkoFyre::CmnRoutines::createId generates a unique ID and returns the value.
 * @date 2016-12-12
 * @note <http://stackoverflow.com/questions/13445688/how-to-generate-a-random-number-in-c>
 * @return The uniquely generated ID.
 */
std::string GekkoFyre::CmnRoutines::createId(const size_t &id_length)
{
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist10(0,9);
    std::ostringstream oss;

    for (size_t i = 0; i < (id_length - 1); ++i) {
        oss << dist10(rng);
    }

    if (oss.str().empty()) {
        throw std::invalid_argument(tr("An invalid Unique Identifier has been created! It is empty.").toStdString());
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

    if (db_mutex.tryLock()) {
        if (!dbFileName.empty()) {
            std::string db_location = leveldb_location(dbFileName);
            sys::error_code ec;
            bool doesExist;
            doesExist = !fs::exists(db_location, ec) ? false : true;

            leveldb::DB *raw_db_ptr;
            s = leveldb::DB::Open(db_struct.options, db_location, &raw_db_ptr);
            db_struct.db.reset(raw_db_ptr);
            if (!s.ok()) {
                db_mutex.unlock();
                throw std::runtime_error(tr("Unable to open/create database! %1").arg(QString::fromStdString(s.ToString())).toStdString());
            }

            if (fs::exists(db_location, ec) && fs::is_directory(db_location) && !doesExist) {
                std::cout << tr("Database object created. Status: ").toStdString() << s.ToString() << std::endl;
            }
        }

        db_mutex.unlock();
    }

    return db_struct;
}

std::string GekkoFyre::CmnRoutines::multipart_key(const std::initializer_list<std::string> &args) {
    std::ostringstream ret_val;
    static int counter;
    counter = 0;
    for (const auto &arg: args) {
        ++counter;
        if (counter == 1) {
            ret_val << arg;
        } else {
            ret_val << "_" << arg;
        }
    }

    return ret_val.str();
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

bool GekkoFyre::CmnRoutines::convertBool_fromInt(const int &value) noexcept {
    bool bool_convert;
    switch (value) {
        case 0:
            bool_convert = false;
            break;
        case 1:
            bool_convert = true;
            break;
        default:
            std::cerr << tr("An invalid integer was provided to convert from to a boolean value.").toStdString() << std::endl;
            return false;
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

    std::stringstream csv_out;
    if (!csv_read_data.empty() && csv_read_data.size() > CFG_CSV_MIN_PARSE_SIZE) {
        QMap<std::string, std::pair<std::string, bool>> cache;
        GkCsvReader csv_reader(3, csv_read_data, LEVELDB_CSV_UID_KEY, LEVELDB_CSV_UID_VALUE1, LEVELDB_CSV_UID_VALUE2);

        csv_reader.force_cache_reload();
        std::string uid_key, path, is_torrent_bool;
        while (csv_reader.read_row(uid_key, path, is_torrent_bool)) {
            if (!uid_key.empty() && !path.empty()) {
                if (!cache.contains(uid_key)) {
                    cache.insert(uid_key, std::make_pair(path, convertBool_fromInt(std::atoi(is_torrent_bool.c_str()))));
                } else {
                    std::cerr << tr("Unique ID already exists in database! Creating new Unique ID...").toStdString() << std::endl;
                    return add_download_id(file_path, db_struct, is_torrent, override_unique_id);
                }
            }
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
                                             const bool &is_torrent)
{
    auto download_ids = extract_download_ids(db_struct, is_torrent);
    std::ostringstream csv_data;

    if (db_mutex.tryLock()) {
        for (const auto &id: download_ids.toStdMap()) {
            if (id.first != unique_id) {
                csv_data << id.first << ",";
                csv_data << id.second.first << ",";
                csv_data << id.second.second << std::endl;
            }
        }

        db_mutex.unlock();
    }

    leveldb::Status s;
    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::WriteBatch batch;

    if (db_mutex.tryLock()) {
        batch.Delete(LEVELDB_STORE_UNIQUE_ID);
        batch.Put(LEVELDB_STORE_UNIQUE_ID, csv_data.str());
        s = db_struct.db->Write(write_options, &batch);
        if (!s.ok()) {
            QMessageBox::warning(nullptr, tr("Error!"), tr("There was an issue while deleting Unique ID, \"%1\", from the "
                                                                   "database. See below.\n\n%2")
                    .arg(QString::fromStdString(unique_id)).arg(QString::fromStdString(s.ToString())), QMessageBox::Ok);

            db_mutex.unlock();
            return false;
        }

        db_mutex.unlock();
    }

    return true;
}

/**
 * @brief GekkoFyre::CmnRoutines::add_item_db adds an item to the database as a key-value pair. With regard to the key, it
 * is formed as a combination of the parameters 'download_id' and 'key'.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-08
 * @param download_id The first part to the key.
 * @param key The second part to the key.
 * @param value The value you wish to store in the database along with the key.
 * @param db_struct The database object used for connecting to the Google LevelDB database.
 */
void GekkoFyre::CmnRoutines::add_item_db(const std::string download_id, const std::string &key, std::string value,
                                         const GekkoFyre::GkFile::FileDb &db_struct)
{
    if (value.empty()) {
        value = "";
    }

    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::WriteBatch batch;

    if (db_mutex.tryLock()) {
        std::string key_joined = multipart_key({download_id, key});
        batch.Delete(key_joined);
        batch.Put(key_joined, value);
        leveldb::Status s;
        s = db_struct.db->Write(write_options, &batch);
        if (!s.ok()) {
            db_mutex.unlock();
            throw std::runtime_error(s.ToString());
        }

        db_mutex.unlock();
    }

    return;
}

void GekkoFyre::CmnRoutines::del_item_db(const std::string download_id, const std::string &key,
                                         const GekkoFyre::GkFile::FileDb &db_struct)
{
    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::WriteBatch batch;

    if (db_mutex.tryLock()) {
        std::string key_joined = multipart_key({download_id, key});
        batch.Delete(key_joined);
        leveldb::Status s;
        s = db_struct.db->Write(write_options, &batch);
        if (!s.ok()) {
            db_mutex.unlock();
            throw std::runtime_error(s.ToString());
        }

        db_mutex.unlock();
    }

    return;
}

/**
 * @brief GekkoFyre::CmnRoutines::read_item_db is a simple function that reads an item from the Google LevelDB database as a
 * key-value pair and returns the value as a 'std::string' object.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-08
 * @param download_id The unique identifier for the value to retrieve from the database.
 * @param db_struct The database object used for connecting to the Google LevelDB database.
 * @return The value that was returned from the given key.
 */
std::string GekkoFyre::CmnRoutines::read_item_db(const std::string download_id, const std::string &key,
                                                 const GekkoFyre::GkFile::FileDb &db_struct)
{
    leveldb::ReadOptions read_opt;
    leveldb::Status s;
    read_opt.verify_checksums = true;

    if (db_mutex.tryLock()) {
        std::string key_joined = multipart_key({download_id, key});
        std::string read_data;
        s = db_struct.db->Get(read_opt, key_joined, &read_data);
        if (!s.ok()) {
            db_mutex.unlock();
            throw std::runtime_error(s.ToString());
        }

        if (!read_data.empty()) {
            db_mutex.unlock();
            return read_data;
        } else {
            db_mutex.unlock();
            return "";
        }
    }
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
    std::string csv_read_data;
    s = db_struct.db->Get(read_opt, LEVELDB_STORE_UNIQUE_ID, &csv_read_data);
    if (!s.ok()) {
        throw std::runtime_error(s.ToString());
    }

    if (!csv_read_data.empty() && csv_read_data.size() > CFG_CSV_MIN_PARSE_SIZE) {
        QMap<std::string, std::pair<std::string, bool>> cache;
        GkCsvReader csv_in(3, csv_read_data, LEVELDB_CSV_UID_KEY, LEVELDB_CSV_UID_VALUE1, LEVELDB_CSV_UID_VALUE2);
        if (!csv_in.has_column(LEVELDB_CSV_UID_KEY) || !csv_in.has_column(LEVELDB_CSV_UID_VALUE1) ||
                !csv_in.has_column(LEVELDB_CSV_UID_VALUE2)) {
            throw std::invalid_argument(tr("Information provided from database is invalid!").toStdString());
        }

        csv_in.force_cache_reload();
        std::string unique_id, path, is_torrent_csv_str;
        int is_torrent_csv = 0;
        while (csv_in.read_row(unique_id, path, is_torrent_csv_str)) {
            is_torrent_csv = std::atoi(is_torrent_csv_str.c_str());
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
    read_opt.verify_checksums = true;

    std::string csv_read_data;
    db_struct.db->Get(read_opt, LEVELDB_STORE_UNIQUE_ID, &csv_read_data);

    QMap<std::string, std::pair<std::string, bool>> cache;
    std::stringstream csv_out;
    if (!csv_read_data.empty() && csv_read_data.size() > CFG_CSV_MIN_PARSE_SIZE) {
        GkCsvReader csv_in(3, csv_read_data, LEVELDB_CSV_UID_KEY, LEVELDB_CSV_UID_VALUE1, LEVELDB_CSV_UID_VALUE2);

        csv_in.force_cache_reload();
        std::string unique_id, path, is_torrent_csv_str;
        bool is_torrent_csv = 0;
        while (csv_in.read_row(unique_id, path, is_torrent_csv_str)) {
            is_torrent_csv = convertBool_fromInt(std::atoi(is_torrent_csv_str.c_str()));
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
                                                                          const int &depth_limit)
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
        throw std::runtime_error(tr("BitTorrent file, \"%1\", too big. Aborting...")
                                         .arg(QString::fromStdString(file_dest)).toStdString());
    }

    if (ret != 0) {
        throw std::runtime_error(tr("Failed to load file, \"%1\".").arg(QString::fromStdString(file_dest)).toStdString());
    }

    bdecode_node e;
    int pos = -1;
    std::cout << tr("Decoding! Recursion limit: %1. Total item count limit: %2.")
            .arg(depth_limit).arg(item_limit).toStdString() << std::endl;
    ret = bdecode(&buf[0], &buf[0] + buf.size(), e, ec, &pos, depth_limit, item_limit);

    if (ret != 0) {
        throw std::invalid_argument(tr("Failed to decode: '%1' at character: %2")
                                            .arg(QString::fromStdString(ec.message())).arg(QString::number(pos)).toStdString());
    }

    torrent_info t(e, ec);
    if (ec) {
        throw ec.message();
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

    std::string unique_id = createId(FYREDL_UNIQUE_ID_DIGIT_COUNT);

    // Trackers
    to_info_mutex.lock();
    int num_trackers = 0;
    size_t bad_trackers = 0;
    for (const auto &i: t.trackers()) {
        GekkoFyre::GkTorrent::TorrentTrackers gk_torrent_tracker;
        if (!i.url.empty()) {
            gk_torrent_tracker.tier = i.tier;
            gk_torrent_tracker.url = i.url;
            gk_torrent_tracker.enabled = true;
            gk_torrent_tracker.unique_id = unique_id;
            ++num_trackers;
            gk_torrent_struct.trackers.push_back(gk_torrent_tracker);
        } else {
            ++bad_trackers;

            if (t.trackers().size() >= bad_trackers) {
                throw std::invalid_argument(tr("No trackers were given for BitTorrent download, \"%1\".")
                                                    .arg(QString::fromStdString(file_dest)).toStdString());
            }

            continue;
        }
    }

    to_info_mutex.unlock();
    // std::ostringstream ih;
    // to_hex((char const*)&t.info_hash()[0], 20, ih);
    // ih << std::hex << t.info_hash();

    if (t.num_pieces() >= 0) {
        gk_torrent_struct.general.num_pieces = t.num_pieces();
    } else {
        throw std::invalid_argument(tr("No downloadable items were specified for BitTorrent item, \"%1\".")
                                            .arg(QString::fromStdString(gk_torrent_struct.general.torrent_name)).toStdString());
    }

    if (t.piece_length() >= 0) {
        gk_torrent_struct.general.piece_length = t.piece_length();
    } else {
        throw std::invalid_argument(tr("No downloadable items were specified for BitTorrent item, \"%1\".")
                                            .arg(QString::fromStdString(gk_torrent_struct.general.torrent_name)).toStdString());
    }

    if (!t.name().empty()) {
        gk_torrent_struct.general.torrent_name = t.name();
    } else {
        gk_torrent_struct.general.torrent_name = "";
    }

    if (!t.comment().empty()) {
        gk_torrent_struct.general.comment = t.comment();
    } else {
        gk_torrent_struct.general.comment = "";
    }

    if (!t.creator().empty()) {
        gk_torrent_struct.general.creator = t.creator();
    } else {
        gk_torrent_struct.general.creator = "";
    }

    if (!make_magnet_uri(t).empty()) {
        gk_torrent_struct.general.magnet_uri = make_magnet_uri(t);
    } else {
        throw std::invalid_argument(tr("An invalid Magnet URI was given for BitTorrent item, \"%1\".")
                                            .arg(QString::fromStdString(gk_torrent_struct.general.torrent_name)).toStdString());
    }

    if (t.num_files() >= 0) {
        if (t.num_files() > item_limit) {
            throw std::invalid_argument(tr("The BitTorrent item, \"%1\", contains more internal files (%2) than the maximum "
                                                   "specified allowable amount: %3.").arg(QString::fromStdString(t.name()))
                                                .arg(QString::number(t.num_files())).arg(QString::number(item_limit)).toStdString());
        }

        gk_torrent_struct.general.num_files = t.num_files();
    } else {
        throw std::invalid_argument(tr("No downloadable items were specified for BitTorrent item, \"%1\".")
                                            .arg(QString::fromStdString(gk_torrent_struct.general.torrent_name)).toStdString());
    }

    // gk_torrent_struct.creatn_timestamp = t.creation_date().get();
    gk_torrent_struct.general.unique_id = unique_id;
    gk_torrent_struct.general.num_trackers = num_trackers;

    const file_storage &st = t.files();
    for (int i = 0; i < st.num_files(); ++i) {
        const int first = st.map_file(i, 0, 0).piece;
        const int last = st.map_file(i, (std::max)(boost::int64_t(st.file_size(i))-1, boost::int64_t(0)), 0).piece;
        GekkoFyre::GkTorrent::TorrentFile gk_tf;
        gk_tf.flags = 0;
        gk_tf.map_file_piece = std::make_pair(0, 0);

        gk_tf.unique_id = unique_id;

        if (!st.hash(i).to_string().empty()) {
            std::ostringstream sha1_hex;
            sha1_hex << std::hex << st.hash(i).to_string();
            if (!sha1_hex.str().empty()) {
                gk_tf.sha1_hash_hex = sha1_hex.str();
            } else {
                gk_tf.sha1_hash_hex = "";
            }
        } else {
            gk_tf.sha1_hash_hex = "";
        }

        if (!st.file_path(i).empty()) {
            gk_tf.file_path = st.file_path(i);
        }

        if (st.file_offset(i) >= 0) {
            gk_tf.file_offset = st.file_offset(i);
        } else {
            gk_tf.file_offset = 0;
        }

        if (st.file_size(i) >= 0) {
            gk_tf.content_length = st.file_size(i);
        } else {
            gk_tf.content_length = 0;
            std::cerr << tr("Invalid content length of zero-bytes has been given! File: \"%1\".")
                    .arg(QString::fromStdString(gk_tf.file_path)).toStdString() << std::endl;
        }

        if (st.mtime(i) >= 0) {
            gk_tf.mtime = (uint32_t)st.mtime(i);
        } else {
            gk_tf.mtime = 0;
        }

        gk_tf.map_file_piece = std::make_pair(first, last);
        gk_tf.flags = st.file_flags(i);
        gk_tf.downloaded = false;
        gk_torrent_struct.files_vec.push_back(gk_tf);
    }

    return gk_torrent_struct;
}

/**
 * @brief GekkoFyre::CmnRoutines::readCurlItems extracts the history information from a Google LevelDB database relating
 * to all the saved HTTP(S)/FTP(S) downloads and parses any CSV herein, outputting a STL container ready for use.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10
 * @param hashesOnly excludes all the 'extended' information by not loading it into memory.
 * @return A STL standard container holding a struct pertaining to all the needed libcurl information is returned.
 */
std::vector<GekkoFyre::GkCurl::CurlDlInfo> GekkoFyre::CmnRoutines::readCurlItems(const bool &hashesOnly)
{
    try {
        // TODO: Implement 'hashesOnly' as originally designed!
        auto download_ids = extract_download_ids(db, false);
        if (!download_ids.empty()) {
            std::vector<GekkoFyre::GkCurl::CurlDlInfo> output;
            for (auto const &id: download_ids.toStdMap()) {
                if (!id.second.second && !id.second.first.empty()) { // Therefore it's a libcurl item!
                    GekkoFyre::GkCurl::CurlDlInfo dl_info;
                    std::string curl_stat, insert_date, complt_date, stat_msg, effec_url, resp_code, cont_lgnth, hash_type,
                            hash_val_given, hash_val_rtrnd, hash_succ_type, down_dest;

                    down_dest = id.second.first;
                    curl_stat = read_item_db(id.first, LEVELDB_KEY_CURL_STAT, db);
                    insert_date = read_item_db(id.first, LEVELDB_KEY_CURL_INSERT_DATE, db);
                    complt_date = read_item_db(id.first, LEVELDB_KEY_CURL_COMPLT_DATE, db);
                    stat_msg = read_item_db(id.first, LEVELDB_KEY_CURL_STATMSG, db);
                    effec_url = read_item_db(id.first, LEVELDB_KEY_CURL_EFFEC_URL, db);
                    resp_code = read_item_db(id.first, LEVELDB_KEY_CURL_RESP_CODE, db);
                    cont_lgnth = read_item_db(id.first, LEVELDB_KEY_CURL_CONT_LNGTH, db);
                    hash_type = read_item_db(id.first, LEVELDB_KEY_CURL_HASH_TYPE, db);
                    hash_val_given = read_item_db(id.first, LEVELDB_KEY_CURL_HASH_VAL_GIVEN, db);
                    hash_val_rtrnd = read_item_db(id.first, LEVELDB_KEY_CURL_HASH_VAL_RTRND, db);
                    hash_succ_type = read_item_db(id.first, LEVELDB_KEY_CURL_HASH_SUCC_TYPE, db);

                    dl_info.dlStatus = convDlStat_IntToEnum(std::atoi(curl_stat.c_str()));
                    dl_info.insert_timestamp = std::atoll(insert_date.c_str());
                    dl_info.complt_timestamp = std::atoll(complt_date.c_str());
                    dl_info.ext_info.status_msg = stat_msg;
                    dl_info.ext_info.effective_url = effec_url;
                    dl_info.ext_info.response_code = std::atoll(resp_code.c_str());
                    dl_info.ext_info.content_length = std::stod(cont_lgnth);
                    dl_info.hash_type = convHashType_IntToEnum(std::atoi(hash_type.c_str()));
                    dl_info.hash_val_given = hash_val_given;
                    dl_info.hash_val_rtrnd = hash_val_rtrnd;
                    dl_info.hash_succ_type = convHashVerif_IntToEnum(std::atoi(hash_succ_type.c_str()));

                    output.push_back(dl_info);
                }
            }

            return output;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return std::vector<GekkoFyre::GkCurl::CurlDlInfo>();
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
                std::string download_key = add_download_id(dl_info.file_loc, db, false, dl_info.unique_id);
                add_item_db(download_key, LEVELDB_KEY_CURL_STAT, std::to_string(convDlStat_toInt(dl_info.dlStatus)), db);
                add_item_db(download_key, LEVELDB_KEY_CURL_INSERT_DATE, std::to_string(dl_info.insert_timestamp), db);
                add_item_db(download_key, LEVELDB_KEY_CURL_COMPLT_DATE, std::to_string(dl_info.complt_timestamp), db);
                add_item_db(download_key, LEVELDB_KEY_CURL_STATMSG, dl_info.ext_info.status_msg, db);
                add_item_db(download_key, LEVELDB_KEY_CURL_EFFEC_URL, dl_info.ext_info.effective_url, db);
                add_item_db(download_key, LEVELDB_KEY_CURL_RESP_CODE, std::to_string(dl_info.ext_info.response_code), db);
                add_item_db(download_key, LEVELDB_KEY_CURL_CONT_LNGTH, std::to_string(dl_info.ext_info.content_length), db);
                add_item_db(download_key, LEVELDB_KEY_CURL_HASH_TYPE, std::to_string(dl_info.hash_type), db);
                add_item_db(download_key, LEVELDB_KEY_CURL_HASH_VAL_GIVEN, dl_info.hash_val_given, db);
                add_item_db(download_key, LEVELDB_KEY_CURL_HASH_VAL_RTRND, dl_info.hash_val_rtrnd, db);
                add_item_db(download_key, LEVELDB_KEY_CURL_HASH_SUCC_TYPE, std::to_string(dl_info.hash_succ_type), db);
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
        std::string download_id;
        if (unique_id_backup.empty() && !file_dest.isEmpty()) {
            auto identifier = determine_download_id(file_dest.toStdString(), db);
            download_id = identifier.first;
        } else {
            // TODO: An empty 'unique_id_backup' can still possibly be provided, as-is dealt with by the exception below!
            download_id = unique_id_backup;
        }

        if (download_id.empty()) {
            throw std::invalid_argument(tr("An invalid Unique ID has been provided. Unable to delete download item with "
                                                   "storage path, \"%1\".").arg(file_dest).toStdString());
        }

        del_item_db(download_id, LEVELDB_KEY_CURL_STAT, db);
        del_item_db(download_id, LEVELDB_KEY_CURL_INSERT_DATE, db);
        del_item_db(download_id, LEVELDB_KEY_CURL_COMPLT_DATE, db);
        del_item_db(download_id, LEVELDB_KEY_CURL_STATMSG, db);
        del_item_db(download_id, LEVELDB_KEY_CURL_EFFEC_URL, db);
        del_item_db(download_id, LEVELDB_KEY_CURL_RESP_CODE, db);
        del_item_db(download_id, LEVELDB_KEY_CURL_CONT_LNGTH, db);
        del_item_db(download_id, LEVELDB_KEY_CURL_HASH_TYPE, db);
        del_item_db(download_id, LEVELDB_KEY_CURL_HASH_VAL_GIVEN, db);
        del_item_db(download_id, LEVELDB_KEY_CURL_HASH_VAL_RTRND, db);
        del_item_db(download_id, LEVELDB_KEY_CURL_HASH_SUCC_TYPE, db);
        bool ret = del_download_id(download_id, db, false);
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
        auto identifier = determine_download_id(file_loc, db);
        if (!identifier.first.empty()) {
            std::string dl_id = identifier.first;

            //
            // General
            add_item_db(dl_id, LEVELDB_KEY_CURL_STAT, std::to_string(convDlStat_toInt(status)), db);
            if (complt_timestamp > 0) {
                add_item_db(dl_id, LEVELDB_KEY_CURL_COMPLT_DATE, std::to_string(complt_timestamp), db);
            }

            //
            // Hash Values
            if (ret_succ_type != GekkoFyre::HashVerif::NotApplicable) {
                add_item_db(dl_id, LEVELDB_KEY_CURL_HASH_TYPE, std::to_string(convHashType_toInt(hash_type)), db);
                add_item_db(dl_id, LEVELDB_KEY_CURL_HASH_VAL_GIVEN, hash_given, db);
                add_item_db(dl_id, LEVELDB_KEY_CURL_HASH_VAL_RTRND, hash_rtrnd, db);
                add_item_db(dl_id, LEVELDB_KEY_CURL_HASH_SUCC_TYPE, std::to_string(convHashVerif_toInt(ret_succ_type)), db);
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
 * @brief GekkoFyre::CmnRoutines::addTorrentItem writes BitTorrent related information to a Google LevelDB database that is kept on
 * the user's local storage within the home directory. This information is stored in a readibly parsable XML format.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07
 * @param gk_ti The BitTorrent related information to write to the database.
 * @return Whether the write operation proceeded successfully or not.
 */
bool GekkoFyre::CmnRoutines::addTorrentItem(GekkoFyre::GkTorrent::TorrentInfo &gk_ti)
{
    try {
        if (!gk_ti.general.down_dest.empty()) {
            QDateTime now = QDateTime::currentDateTime();
            gk_ti.general.insert_timestamp = now.toTime_t();

            std::string download_key = add_download_id(gk_ti.general.down_dest, db, true, gk_ti.general.unique_id);
            if (gk_ti.general.unique_id != download_key) {
                throw std::invalid_argument(tr("Unique ID mismatch while adding a BitTorrent item!\n\n%1 != %2")
                                                    .arg(QString::fromStdString(gk_ti.general.unique_id))
                                                    .arg(QString::fromStdString(download_key)).toStdString());
            }

            add_item_db(download_key, LEVELDB_KEY_TORRENT_INSERT_DATE, std::to_string(gk_ti.general.insert_timestamp), db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_COMPLT_DATE, std::to_string(gk_ti.general.complt_timestamp), db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_CREATN_DATE, std::to_string(gk_ti.general.creatn_timestamp), db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_DLSTATUS, convDlStat_toString(gk_ti.general.dlStatus).toStdString(), db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_TORRNT_COMMENT, gk_ti.general.comment, db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_TORRNT_CREATOR, gk_ti.general.creator, db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_MAGNET_URI, gk_ti.general.magnet_uri, db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_TORRNT_NAME, gk_ti.general.torrent_name, db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_NUM_FILES, std::to_string(gk_ti.general.num_files), db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_NUM_TRACKERS, std::to_string(gk_ti.general.num_trackers), db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_TORRNT_PIECES, std::to_string(gk_ti.general.num_pieces), db);
            add_item_db(download_key, LEVELDB_KEY_TORRENT_TORRNT_PIECE_LENGTH, std::to_string(gk_ti.general.piece_length), db);

            //
            // Files
            //
            bool wtf_ret = write_torrent_files_addendum(gk_ti.files_vec, download_key, db);

            //
            // Trackers
            //
            bool wtt_ret = write_torrent_trkrs_addendum(gk_ti.trackers, download_key, db);

            if (!wtf_ret || !wtt_ret) {
                QMessageBox::warning(nullptr, tr("Error!"), tr("There was an error with inserting the following BitTorrent "
                                                                       "item into the database: \"%1\".")
                        .arg(QString::fromStdString(gk_ti.general.torrent_name)), QMessageBox::Ok);
                return false;
            }

            return true;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return false;
    }

    return false;
}

/**
 * @brief GekkoFyre::CmnRoutines::readTorrentItems extracts all of the users stored history relating to BitTorrent downloads
 * from a Google LevelDB database and parses any CSV therein, outputting a STL container that's ready for use.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07
 * @param minimal_readout only extracts the most vital history information, thus (potentially) saving CPU time and
 * memory.
 * @return A STL standard container holding a struct pertaining to all the needed BitTorrent information is returned.
 */
std::vector<GekkoFyre::GkTorrent::TorrentInfo> GekkoFyre::CmnRoutines::readTorrentItems(const bool &minimal_readout)
{
    try {
        // TODO: Implement 'minimal_readout' as originally designed! Use boost::optional<>() for this.
        auto download_ids = extract_download_ids(db, true);
        if (!download_ids.empty()) {
            std::vector<GekkoFyre::GkTorrent::TorrentInfo> output;
            for (auto const &id: download_ids.toStdMap()) {
                if (id.second.second && !id.second.first.empty()) { // Therefore it's a BitTorrent item!
                    GekkoFyre::GkTorrent::TorrentInfo to_info;
                    GekkoFyre::GkTorrent::GeneralInfo gen_info;
                    std::string insert_date, complt_date, creatn_date, dlstatus, comment, creator, magnet_uri, torrent_name,
                            num_files, num_trackers, num_pieces, piece_length;

                    insert_date = read_item_db(id.first, LEVELDB_KEY_TORRENT_INSERT_DATE, db);
                    complt_date = read_item_db(id.first, LEVELDB_KEY_TORRENT_COMPLT_DATE, db);
                    creatn_date = read_item_db(id.first, LEVELDB_KEY_TORRENT_CREATN_DATE, db);
                    dlstatus = read_item_db(id.first, LEVELDB_KEY_TORRENT_DLSTATUS, db);
                    comment = read_item_db(id.first, LEVELDB_KEY_TORRENT_TORRNT_COMMENT, db);
                    creator = read_item_db(id.first, LEVELDB_KEY_TORRENT_TORRNT_CREATOR, db);
                    magnet_uri = read_item_db(id.first, LEVELDB_KEY_TORRENT_MAGNET_URI, db);
                    torrent_name = read_item_db(id.first, LEVELDB_KEY_TORRENT_TORRNT_NAME, db);
                    num_files = read_item_db(id.first, LEVELDB_KEY_TORRENT_NUM_FILES, db);
                    num_trackers = read_item_db(id.first, LEVELDB_KEY_TORRENT_NUM_TRACKERS, db);
                    num_pieces = read_item_db(id.first, LEVELDB_KEY_TORRENT_TORRNT_PIECES, db);
                    piece_length = read_item_db(id.first, LEVELDB_KEY_TORRENT_TORRNT_PIECE_LENGTH, db);

                    gen_info.unique_id = id.first;
                    gen_info.down_dest = id.second.first;
                    gen_info.insert_timestamp = std::atoll(insert_date.c_str());
                    gen_info.complt_timestamp = std::atoll(complt_date.c_str());
                    gen_info.creatn_timestamp = std::atol(creatn_date.c_str());
                    gen_info.dlStatus = convDlStat_IntToEnum(std::atoi(dlstatus.c_str()));
                    gen_info.comment = comment;
                    gen_info.creator = creator;
                    gen_info.magnet_uri = magnet_uri;
                    gen_info.torrent_name = torrent_name;
                    gen_info.num_files = std::atoi(num_files.c_str());
                    gen_info.num_trackers = std::atoi(num_trackers.c_str());
                    gen_info.num_pieces = std::atoi(num_pieces.c_str());
                    gen_info.piece_length = std::atoi(piece_length.c_str());

                    //
                    // Files
                    auto files_info_vec = read_torrent_files_addendum(gen_info.num_files, id.first, db);

                    //
                    // Trackers
                    auto trackers_info_vec = read_torrent_trkrs_addendum(gen_info.num_trackers, id.first, db);

                    to_info.general = gen_info;
                    if (!files_info_vec.empty()) {
                        to_info.files_vec = files_info_vec;
                    } else {
                        throw std::invalid_argument(tr("Unable to interpret the internal file-layout for BitTorrent item, \"%1\".")
                                                            .arg(QString::fromStdString(gen_info.torrent_name)).toStdString());
                    }

                    if (!trackers_info_vec.empty()) {
                        to_info.trackers = trackers_info_vec;
                    } else {
                        throw std::invalid_argument(tr("Unable to determine the trackers for BitTorrent item, \"%1\".")
                                                            .arg(QString::fromStdString(gen_info.torrent_name)).toStdString());
                    }

                    output.push_back(to_info);
                }
            }

            return output;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return std::vector<GekkoFyre::GkTorrent::TorrentInfo>();
    }

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

bool GekkoFyre::CmnRoutines::write_torrent_files_addendum(std::vector<GekkoFyre::GkTorrent::TorrentFile> &to_files_vec,
                                                          const std::string &download_key,
                                                          const GekkoFyre::GkFile::FileDb &db_struct) noexcept
{
    if (!to_files_vec.empty()) {
        static int counter;
        counter = 0;
        for (auto &f: to_files_vec) {
            ++counter;
            std::string file_key, mapflepce_key;
            std::ostringstream file_write_data, mapflepce_write_data;
            leveldb::WriteOptions write_options;
            leveldb::WriteBatch batch;

            file_key = multipart_key({download_key, LEVELDB_KEY_TORRENT_TORRENT_FILES, std::to_string(counter)});
            mapflepce_key = multipart_key({download_key, LEVELDB_CHILD_NODE_TORRENT_FILES_MAPFLEPCE, std::to_string(counter)});
            write_options.sync = true;
            batch.Delete(file_key);
            batch.Delete(mapflepce_key);

            if (f.sha1_hash_hex.empty()) {
                f.sha1_hash_hex = "";
            }

            file_write_data << LEVELDB_CSV_TORRENT_FILE_PATH << "," << LEVELDB_CSV_TORRENT_FILE_CONTENT_LENGTH << ",";
            file_write_data << LEVELDB_CSV_TORRENT_FILE_SHA1 << "," << LEVELDB_CSV_TORRENT_FILE_FILE_OFFSET << ",";
            file_write_data << LEVELDB_CSV_TORRENT_FILE_MTIME << "," << LEVELDB_CSV_TORRENT_FILE_MAPFLEPCE_KEY ",";
            file_write_data << LEVELDB_CSV_TORRENT_FILE_BOOL_DLED << "," << LEVELDB_CSV_TORRENT_FILE_FLAGS << std::endl;
            file_write_data << f.file_path << "," << std::to_string(f.content_length) << "," << f.sha1_hash_hex << ",";
            file_write_data << std::to_string(f.file_offset) << "," << std::to_string(f.mtime) << ",";
            file_write_data << mapflepce_key << "," << std::to_string(f.downloaded) << "," << std::to_string(f.flags);

            mapflepce_write_data << LEVELDB_CSV_TORRENT_MAPFLEPCE_1 << "," << LEVELDB_CSV_TORRENT_MAPFLEPCE_2 << std::endl;
            mapflepce_write_data << std::to_string(f.map_file_piece.first) << "," << std::to_string(f.map_file_piece.second);

            batch.Put(file_key, file_write_data.str());
            batch.Put(mapflepce_key, mapflepce_write_data.str());
            leveldb::Status s;
            s = db_struct.db->Write(write_options, &batch);
            if (!s.ok()) {
                std::cerr << tr("Error whilst processing files for BitTorrent item: \"%1\".\nError: ")
                        .arg(QString::fromStdString(download_key)).toStdString() << s.ToString() << std::endl;
                return false;
            }
        }

        return true;
    }

    return false;
}

std::vector<GekkoFyre::GkTorrent::TorrentFile> GekkoFyre::CmnRoutines::read_torrent_files_addendum(const int &num_files, const std::string &download_key,
                                                                                                   const GekkoFyre::GkFile::FileDb &db_struct)
{
    if (num_files > 0) {
        static int counter;
        counter = (num_files + 1);
        std::vector<GekkoFyre::GkTorrent::TorrentFile> to_files;
        while (counter > num_files) {
            --counter;
            std::string file_key, csv_file_data;
            leveldb::ReadOptions read_opt;
            leveldb::Status s;
            read_opt.verify_checksums = true;

            file_key = multipart_key({download_key, LEVELDB_KEY_TORRENT_TORRENT_FILES, std::to_string(counter)});
            s = db_struct.db->Get(read_opt, file_key, &csv_file_data);
            if (!s.ok()) {
                std::cerr << tr("Error whilst processing files for BitTorrent item: \"%1\".\nError: ")
                        .arg(QString::fromStdString(download_key)).toStdString() << s.ToString() << std::endl;
                return std::vector<GekkoFyre::GkTorrent::TorrentFile>();
            }

            if (!csv_file_data.empty() && csv_file_data.size() > CFG_CSV_MIN_PARSE_SIZE) {
                GkCsvReader csv_parse(8, csv_file_data, LEVELDB_CSV_TORRENT_FILE_PATH, LEVELDB_CSV_TORRENT_FILE_CONTENT_LENGTH,
                                      LEVELDB_CSV_TORRENT_FILE_SHA1, LEVELDB_CSV_TORRENT_FILE_FILE_OFFSET, LEVELDB_CSV_TORRENT_FILE_MTIME,
                                      LEVELDB_CSV_TORRENT_FILE_MAPFLEPCE_KEY, LEVELDB_CSV_TORRENT_FILE_BOOL_DLED, LEVELDB_CSV_TORRENT_FILE_FLAGS);
                if (!csv_parse.has_column(LEVELDB_CSV_TORRENT_FILE_PATH) || !csv_parse.has_column(LEVELDB_CSV_TORRENT_FILE_CONTENT_LENGTH) ||
                    !csv_parse.has_column(LEVELDB_CSV_TORRENT_FILE_FILE_OFFSET) || !csv_parse.has_column(LEVELDB_CSV_TORRENT_FILE_BOOL_DLED)) {
                    QMessageBox::warning(nullptr, tr("Error!"), tr("Missing vital data as FyreDL attempts to import BitTorrent item, \"%1\".")
                            .arg(QString::fromStdString(download_key)), QMessageBox::Ok);
                }

                csv_parse.force_cache_reload();
                std::string file_path, content_length, sha1, file_offset, mod_time, mapflepce_key, bool_dled, flags;
                GekkoFyre::GkTorrent::TorrentFile item;
                while (csv_parse.read_row(file_path, content_length, sha1, file_offset, mod_time, mapflepce_key, bool_dled, flags)) {
                    if (!download_key.empty()) {
                        item.unique_id = download_key; // This is needed because we are not extracting the key in the CSV data itself
                        item.file_path = file_path;
                        item.content_length = std::atol(content_length.c_str());
                        item.sha1_hash_hex = sha1;
                        item.file_offset = std::atol(file_offset.c_str());
                        item.mtime = std::atoi(mod_time.c_str());
                        item.downloaded = convertBool_fromInt(std::atoi(bool_dled.c_str()));
                        item.flags = std::atoi(flags.c_str());

                        std::string csv_mapflepce_data;
                        s = db_struct.db->Get(read_opt, mapflepce_key, &csv_mapflepce_data);
                        if (!s.ok()) {
                            std::cerr << tr("Error whilst processing files for BitTorrent item: \"%1\".\nError: ")
                                    .arg(QString::fromStdString(download_key)).toStdString() << s.ToString() << std::endl;
                            return std::vector<GekkoFyre::GkTorrent::TorrentFile>();
                        }

                        GkCsvReader csv_mapflepce_parse(2, csv_mapflepce_data, LEVELDB_CSV_TORRENT_MAPFLEPCE_1, LEVELDB_CSV_TORRENT_MAPFLEPCE_2);
                        if (!csv_mapflepce_parse.has_column(LEVELDB_CSV_TORRENT_MAPFLEPCE_1) ||
                            !csv_mapflepce_parse.has_column(LEVELDB_CSV_TORRENT_MAPFLEPCE_2)) {
                            QMessageBox::warning(nullptr, tr("Error!"), tr("Missing vital data as FyreDL attempts to import BitTorrent item, \"%1\".")
                                    .arg(QString::fromStdString(download_key)), QMessageBox::Ok);
                        }

                        std::string mapflepce_1, mapflepce_2;
                        while (csv_mapflepce_parse.read_row(mapflepce_1, mapflepce_2)) {
                            item.map_file_piece.first = std::atoi(mapflepce_1.c_str());
                            item.map_file_piece.second = std::atoi(mapflepce_2.c_str());
                            break;
                        }
                    }
                }

                to_files.push_back(item);
            }
        }

        return to_files;
    }

    return std::vector<GekkoFyre::GkTorrent::TorrentFile>();
}

bool GekkoFyre::CmnRoutines::write_torrent_trkrs_addendum(const std::vector<GekkoFyre::GkTorrent::TorrentTrackers> &to_trackers_vec,
                                                          const std::string &download_key,
                                                          const GekkoFyre::GkFile::FileDb &db_struct) noexcept {
    if (!to_trackers_vec.empty()) {
        static int counter;
        counter = 0;
        for (const auto &t: to_trackers_vec) {
            ++counter;
            std::string tracker_key;
            std::ostringstream tracker_write_data;
            leveldb::WriteOptions write_options;
            leveldb::WriteBatch batch;

            tracker_key = multipart_key({download_key, LEVELDB_KEY_TORRENT_TRACKERS, std::to_string(counter)});
            write_options.sync = true;
            batch.Delete(tracker_key);

            tracker_write_data << LEVELDB_CSV_TORRENT_TRACKER_URL << "," << LEVELDB_CSV_TORRENT_TRACKER_TIER ",";
            tracker_write_data << LEVELDB_CSV_TORRENT_TRACKER_BOOL_ENABLED << std::endl;
            tracker_write_data << t.url << "," << std::to_string(t.tier) << "," << std::to_string(t.enabled);
            batch.Put(tracker_key, tracker_write_data.str());
            leveldb::Status s;
            s = db_struct.db->Write(write_options, &batch);
            if (!s.ok()) {
                std::cerr << tr("Error whilst processing trackers for BitTorrent item: \"%1\".\nError: ")
                        .arg(QString::fromStdString(download_key)).toStdString() << s.ToString() << std::endl;
                return false;
            }
        }
    }

    return false;
}

std::vector<GekkoFyre::GkTorrent::TorrentTrackers> GekkoFyre::CmnRoutines::read_torrent_trkrs_addendum(const int &num_trackers, const std::string &download_key,
                                                                                                       const GekkoFyre::GkFile::FileDb &db_struct)
{
    if (num_trackers > 0) {
        static int counter;
        counter = (num_trackers + 1);
        std::vector<GekkoFyre::GkTorrent::TorrentTrackers> to_trackers;
        while (counter > num_trackers) {
            --counter;
            std::string tracker_key, csv_tracker_data;
            leveldb::ReadOptions read_opt;
            leveldb::Status s;
            read_opt.verify_checksums = true;

            tracker_key = multipart_key({download_key, LEVELDB_KEY_TORRENT_TRACKERS, std::to_string(counter)});
            s = db_struct.db->Get(read_opt, tracker_key, &csv_tracker_data);
            if (!s.ok()) {
                std::cerr << tr("Error whilst processing files for BitTorrent item: \"%1\".\nError: ")
                        .arg(QString::fromStdString(download_key)).toStdString() << s.ToString() << std::endl;
                return std::vector<GekkoFyre::GkTorrent::TorrentTrackers>();
            }

            if (!csv_tracker_data.empty() && csv_tracker_data.size() > CFG_CSV_MIN_PARSE_SIZE) {
                GkCsvReader csv_parse(3, csv_tracker_data, LEVELDB_CSV_TORRENT_TRACKER_URL, LEVELDB_CSV_TORRENT_TRACKER_TIER,
                                      LEVELDB_CSV_TORRENT_TRACKER_BOOL_ENABLED);
                if (!csv_parse.has_column(LEVELDB_CSV_TORRENT_TRACKER_URL) || !csv_parse.has_column(LEVELDB_CSV_TORRENT_TRACKER_TIER) ||
                        !csv_parse.has_column(LEVELDB_CSV_TORRENT_TRACKER_BOOL_ENABLED)) {
                    QMessageBox::warning(nullptr, tr("Error!"), tr("Missing vital data as FyreDL attempts to import BitTorrent item, \"%1\".")
                            .arg(QString::fromStdString(download_key)), QMessageBox::Ok);
                }

                csv_parse.force_cache_reload();
                std::string tracker_url, tracker_tier, tracker_bool_enabled;
                GekkoFyre::GkTorrent::TorrentTrackers item;
                while (csv_parse.read_row(tracker_url, tracker_tier, tracker_bool_enabled)) {
                    item.url = tracker_url;
                    item.tier = std::atoi(tracker_tier.c_str());
                    item.enabled = convertBool_fromInt(std::atoi(tracker_bool_enabled.c_str()));
                    item.unique_id = download_key;
                }

                to_trackers.push_back(item);
            }
        }

        return to_trackers;
    }

    return std::vector<GekkoFyre::GkTorrent::TorrentTrackers>();
}
