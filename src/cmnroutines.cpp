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

#include "cmnroutines.hpp"
#include "default_var.hpp"
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

GekkoFyre::CmnRoutines::CmnRoutines()
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
 * @brief GekkoFyre::CmnRoutines::singleAppInstance_Win32 detects, under Microsoft Windows, if an existing instance of this
 * application is already open, even across different logins.
 * @author krishna_kp <http://stackoverflow.com/questions/4191465/how-to-run-only-one-instance-of-application>
 * @date 2013-06-07
 * @return Returns false upon finding an existing instance of this application, otherwise returns true on finding none.
 */
bool GekkoFyre::CmnRoutines::singleAppInstance_Win32()
{
    #ifdef _WIN32
    HANDLE m_hStartEvent = CreateEventW(NULL, FALSE, FALSE, L"Global\\FyreDL");
    if(m_hStartEvent == NULL) {
        CloseHandle(m_hStartEvent);
        return false;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(m_hStartEvent);
        m_hStartEvent = NULL;
            QMessageBox::information(nullptr, tr("Greetings!"), tr("It seems that an existing instance of this application "
                                                                   "is already open! Please close that first before "
                                                                   "re-opening again."),
                             QMessageBox::Ok);
        return false;
    }

    return true;
    #elif __linux__
    return true;
    #else
    #error "Platform not supported!"
    #endif
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

    return oss.str();
}

/**
 * @brief GekkoFyre::CmnRoutines::findCfgFile finds the defined configuration file and checks if it exists.
 * @note <http://theboostcpplibraries.com/boost.filesystem-paths>
 * @param cfgFileName
 * @return
 */
std::string GekkoFyre::CmnRoutines::findCfgFile(const std::string &cfgFileName)
{
    fs::path home_dir(QDir::homePath().toStdString());
    std::ostringstream oss;
    if (fs::is_directory(home_dir)) {
        oss << home_dir.string() << fs::path::preferred_separator << cfgFileName;
    } else {
        throw std::invalid_argument(tr("Unable to find home directory!").toStdString());
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
    std::unique_ptr<leveldb::Cache>(db_struct.options.block_cache).reset(leveldb::NewLRUCache(LEVELDB_CFG_CACHE_SIZE));
    db_struct.options.compression = leveldb::CompressionType::kSnappyCompression;

    // Determine the home directory and where to put the database files, depending on whether this is a Linux or
    // Microsoft Windows operating system that FyreDL is running on.
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
            fs::create_directory(oss_db_dir.str());
        }

        oss_db_file << oss_db_dir.str() << fs::path::preferred_separator << dbFileName;
    } else {
        throw std::invalid_argument(tr("Unable to find home directory!").toStdString());
    }

    leveldb::DB *raw_db_ptr;
    s = leveldb::DB::Open(db_struct.options, oss_db_file.str(), &raw_db_ptr);
    db_struct.db.reset(raw_db_ptr);
    if (!s.ok()) {
        throw std::runtime_error(tr("Unable to open/create database! %1").arg(QString::fromStdString(s.ToString())).toStdString());
    }

    sys::error_code ec;
    if (fs::exists(oss_db_file.str(), ec) && fs::is_regular_file(oss_db_file.str())) {
        std::cout << tr("Database object created. Status: ").toStdString() << s.ToString();
    }

    return db_struct;
}

/**
 * @brief GekkoFyre::CmnRoutines::batch_write_single_db
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-09
 * @note <https://github.com/google/leveldb/blob/master/doc/index.md>
 *       <http://www.gerald-fahrnholz.eu/sw/DocGenerated/HowToUse/html/group___grp_pugi_xml.html#pugi_xml_add_data>
 * @param key
 * @param value
 * @param unique_id
 * @param dbFileName
 * @return
 */
bool GekkoFyre::CmnRoutines::batch_write_single_db(const std::string &key, const std::string &value,
                                                   const std::string &unique_id, const GekkoFyre::GkFile::FileDb &file_db_struct)
{
    leveldb::Status s;
    if (s.ok()) {
        std::string existing_value;
        leveldb::ReadOptions read_opt;
        read_opt.verify_checksums = true;

        s = file_db_struct.db->Get(read_opt, key, &existing_value);
        leveldb::WriteOptions write_options;
        write_options.sync = true;
        leveldb::WriteBatch batch;

        pugi::xml_node root;
        std::unique_ptr<pugi::xml_document> doc = std::make_unique<pugi::xml_document>();
        if (!existing_value.empty() && existing_value.size() > CFG_XML_MIN_PARSE_SIZE) {
            pugi::xml_parse_result result = doc->load_string(existing_value.c_str());
            if (!result) {
                throw std::invalid_argument(tr("There has been an error within the database batch writer routine.\n\nParse error: %1, character pos = %2")
                                                    .arg(result.description()).arg(result.offset).toStdString());
            }

            batch.Delete(key);
        } else {
            std::stringstream new_xml_data;
            // Generate XML declaration
            auto declarNode = doc->append_child(pugi::node_declaration);
            declarNode.append_attribute("version") = "1.0";
            declarNode.append_attribute("encoding") = "UTF-8";;
            declarNode.append_attribute("standalone") = "yes";

            // A valid XML doc must contain a single root node of any name
            root = doc->append_child(LEVELDB_PARENT_NODE);
            pugi::xml_node xml_version_node = root.append_child(LEVELDB_CHILD_NODE_VERS);
            pugi::xml_node xml_version_child = xml_version_node.append_child(LEVELDB_CHILD_ITEM_VERS);
            xml_version_child.append_attribute(LEVELDB_ITEM_ATTR_VERS_NO) = FYREDL_PROG_VERS;
        }

        pugi::xml_node nodeParent = root.append_child(LEVELDB_XML_CHILD_NODE);
        nodeParent.append_attribute(LEVELDB_XML_ATTR_DL_TYPE) = convDownType_toInt(GekkoFyre::DownloadType::Torrent); // TODO: Fix this immediately!
        nodeParent.append_attribute(LEVELDB_XML_ATTR_ITEM_VALUE) = value.c_str();
        nodeParent.append_attribute(LEVELDB_ITEM_ATTR_UNIQUE_ID) = unique_id.c_str();

        // Save XML tree to file.
        // Remark: second optional param is indent string to be used;
        // default indentation is tab character.
        std::stringstream ss;
        doc->save(ss, PUGIXML_TEXT("    "));
        batch.Put(key, ss.str());

        s = file_db_struct.db->Write(write_options, &batch);
        if (!s.ok()) {
            throw std::runtime_error(s.ToString());
        } else {
            return true;
        }
    }

    return false;
}

/**
 * @brief GekkoFyre::CmnRoutines::read_database will read the LevelDB database and process the XML contained within,
 * outputting an STL container that's ready for use.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-09
 * @note <https://github.com/google/leveldb/blob/master/doc/index.md>
 * @param key
 * @param unique_id
 * @param dbFileName
 * @return An STL container of the processed XML from the database, all ready for use right away.
 */
GekkoFyre::GkFile::FileDbVal GekkoFyre::CmnRoutines::read_database(const std::string &key, const std::string &unique_id,
                                                                   const std::string &dbFileName)
{
    GekkoFyre::GkFile::FileDb db_struct = openDatabase(dbFileName);
    leveldb::Status s;
    if (s.ok()) {
        std::string value;
        s = db_struct.db->Get(leveldb::ReadOptions(), key, &value);
        if (!s.ok()) {
            throw std::runtime_error(tr("A problem has occured while attmepting to read key, \"%1\", from the database. Details:\n\n%2")
                                             .arg(QString::fromStdString(key)).arg(QString::fromStdString(s.ToString())).toStdString());
        }

        pugi::xml_node root;
        std::unique_ptr<pugi::xml_document> doc = std::make_unique<pugi::xml_document>();
        pugi::xml_parse_result result = doc->load_string(value.c_str());
        if (!result) {
            throw std::invalid_argument(tr("There has been an error reading information from the database.\n\nParse error: %1, character pos = %2")
                                                .arg(result.description()).arg(result.offset).toStdString());
        }

        std::vector<GekkoFyre::GkFile::FileDbVal> file_db_vec;
        pugi::xml_node items = doc->child(LEVELDB_PARENT_NODE);
        for (const auto &file: items.children(LEVELDB_XML_CHILD_NODE)) {
            GekkoFyre::GkFile::FileDbVal file_db_val;
            file_db_val.dl_type = convDownType_StringToEnum(file.attribute(LEVELDB_XML_ATTR_DL_TYPE).as_string()); // TODO: Fix this immediately! Should be 'IntToEnum'.
            file_db_val.value = file.attribute(LEVELDB_XML_ATTR_ITEM_VALUE).as_string();
            file_db_val.unique_id = file.attribute(LEVELDB_ITEM_ATTR_UNIQUE_ID).as_string();
            file_db_vec.push_back(file_db_val);
        }

        if (file_db_vec.size() > 0) {
            for (size_t i = 0; i < file_db_vec.size(); ++i) {
                if (file_db_vec.at(i).unique_id == unique_id) {
                    return file_db_vec.at(i);
                }
            }
        }
    }

    return GekkoFyre::GkFile::FileDbVal();
}

/**
 * @brief GekkoFyre::CmnRoutines::read_db_vec will read the LevelDB database and process the XML contained within,
 * outputting an STL container that's ready for use.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-09
 * @note <https://github.com/google/leveldb/blob/master/doc/index.md>
 * @param key The key whose value we are retrieving.
 * @param file_db_struct An object that contains data relating to and pointing towards the database in question.
 * @return An STL container of the processed XML from the database, all ready for use right away.
 */
std::vector<GekkoFyre::GkFile::FileDbVal> GekkoFyre::CmnRoutines::read_db_vec(const std::string &key, const GekkoFyre::GkFile::FileDb &file_db_struct)
{
    leveldb::ReadOptions read_opt;
    leveldb::Status s;
    read_opt.verify_checksums = true;

    std::string read_xml;
    s = file_db_struct.db->Get(read_opt, key, &read_xml);
    if (!s.ok()) {
        throw std::runtime_error(tr("A problem has occured while attmepting to read from the database. Details:\n\n%1")
                                         .arg(QString::fromStdString(s.ToString())).toStdString());
    } else {
        // Proceed
        pugi::xml_node root;
        std::unique_ptr<pugi::xml_document> doc = std::make_unique<pugi::xml_document>();
        pugi::xml_parse_result result = doc->load_string(read_xml.c_str());
        if (!result) {
            throw std::invalid_argument(tr("There has been an error reading information from the database.\n\nParse error: %1, character pos = %2")
                                                .arg(result.description()).arg(result.offset).toStdString());
        }

        std::vector<GekkoFyre::GkFile::FileDbVal> file_db_vec;
        pugi::xml_node items = doc->child(LEVELDB_PARENT_NODE);
        for (const auto &file: items.children(LEVELDB_XML_CHILD_NODE)) {
            GekkoFyre::GkFile::FileDbVal file_db_val;
            file_db_val.dl_type = convDownType_StringToEnum(file.attribute(LEVELDB_XML_ATTR_DL_TYPE).as_string()); // TODO: Fix this immediately! Should be 'IntToEnum'.
            file_db_val.value = file.attribute(LEVELDB_XML_ATTR_ITEM_VALUE).as_string();
            file_db_val.unique_id = file.attribute(LEVELDB_ITEM_ATTR_UNIQUE_ID).as_string();
            file_db_vec.push_back(file_db_val);
        }

        if (file_db_vec.size() > 0) {
            return file_db_vec;
        }
    }

    return std::vector<GekkoFyre::GkFile::FileDbVal>();
}

std::pair<std::string, std::string> GekkoFyre::CmnRoutines::read_db_min(const std::string &key, const GekkoFyre::GkFile::FileDb &file_db_struct)
{
    leveldb::ReadOptions read_opt;
    leveldb::Status s;
    read_opt.verify_checksums = true;

    std::string read_xml;
    s = file_db_struct.db->Get(read_opt, key, &read_xml);

    return std::make_pair(key, read_xml);
}

/**
 * @brief GekkoFyre::CmnRoutines::process_db processes raw database output into a slightly more readable 'QMap<std::string, std::string>', that's
 * ready for further processing.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-09
 * @note <https://isocpp.org/wiki/faq/templates>
 *       <http://kevinushey.github.io/blog/2016/01/27/introduction-to-c++-variadic-templates/>
 *       <https://stackoverflow.com/questions/37139379/stdinitializer-list-vs-variadic-templates>
 *       <https://stackoverflow.com/questions/10044449/a-function-with-variable-number-of-arguments-with-known-types-the-c11-way>
 * @param args The raw database information.
 * @return A slightly more readable 'QMap<std::string, std::string>' that's ready for further processing.
 */
QMap<std::string, std::string> GekkoFyre::CmnRoutines::process_db(std::initializer_list<std::tuple<std::string, std::string>> args)
{
    QMap<std::string, std::string> mmap;
    for (const std::tuple<std::string, std::string> &i: args) {
        // Arguments have been split up
        std::string val_to_down_dest, val_to_insert_timestamp, val_to_complt_timestamp, val_to_creatn_timestamp,
                val_to_status, val_to_comment, val_to_creator, val_to_magnet_uri, val_to_name, val_to_num_files,
                val_to_num_pieces, val_to_piece_length;

        if (std::get<0>(i) == LEVELDB_KEY_TORRENT_FLOC) {
            val_to_down_dest = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_FLOC, val_to_down_dest);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_INSERT_DATE) {
            val_to_insert_timestamp = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_INSERT_DATE, val_to_insert_timestamp);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_COMPLT_DATE) {
            val_to_complt_timestamp = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_COMPLT_DATE, val_to_complt_timestamp);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_CREATN_DATE) {
            val_to_creatn_timestamp = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_CREATN_DATE, val_to_creatn_timestamp);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_DLSTATUS) {
            val_to_status = DownloadStatus::Unknown;
            mmap.insert(LEVELDB_KEY_TORRENT_DLSTATUS, val_to_status);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_TORRNT_COMMENT) {
            val_to_comment = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_TORRNT_COMMENT, val_to_comment);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_TORRNT_CREATOR) {
            val_to_creator = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_TORRNT_CREATOR, val_to_creator);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_MAGNET_URI) {
            val_to_magnet_uri = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_MAGNET_URI, val_to_magnet_uri);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_TORRNT_NAME) {
            val_to_name = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_TORRNT_NAME, val_to_name);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_NUM_FILES) {
            val_to_num_files = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_NUM_FILES, val_to_num_files);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_TORRNT_PIECES) {
            val_to_num_pieces = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_TORRNT_PIECES, val_to_num_pieces);
        } else if (std::get<0>(i) == LEVELDB_KEY_TORRENT_TORRNT_PIECE_LENGTH) {
            val_to_piece_length = std::get<1>(i);
            mmap.insert(LEVELDB_KEY_TORRENT_TORRNT_PIECE_LENGTH, val_to_piece_length);
        }
    }

    return mmap;
}

/**
 * @brief GekkoFyre::CmnRoutines::process_db_map process a 'QMap<std::string, std::string>' into a pre-defined struct
 * that is far more readable and easily accessed.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-12
 * @param map The container with the XML data to process.
 * @param args The keys required to access the values within the container.
 * @return A pre-defined struct that is far more readable and easily accessed, specifically, 'GekkoFyre::GkTorrent::TorrentInfo'.
 * @see GekkoFyre::CmnRoutines::process_db(), GekkoFyre::CmnRoutines::process_db_xml()
 */
std::vector<GekkoFyre::GkTorrent::GeneralInfo> GekkoFyre::CmnRoutines::process_db_map(const QMap<std::string, std::string> &map,
                                                                                      std::initializer_list<std::string> args)
{
    QMultiMap<std::pair<std::string, std::string>, std::string> mmap_store;
    QList<std::string> found_unique_id;
    for (const auto &arg_key: args) {
        // Expand the arguments
        std::string val = map.value(arg_key);
        if (!val.empty()) {
            std::vector<GekkoFyre::GkFile::FileDbVal> xml_data = process_db_xml(val);
            if (xml_data.size() > 0) {
                for (const auto &j: xml_data) {
                    std::string tmp_val, tmp_unique_id;
                    if (!j.value.empty()) {
                        // Value
                        tmp_val = j.value;

                        if (!j.unique_id.empty()) {
                            // Unique ID
                            tmp_unique_id = j.unique_id;
                        } else {
                            // Determine the home directory and where to put the database files, depending on
                            // whether this is a Linux or Microsoft Windows operating system that FyreDL is running on.
                            fs::path home_dir(QDir::homePath().toStdString());
                            std::ostringstream oss;
                            #ifdef __linux__
                            oss << home_dir.string() << fs::path::preferred_separator << CFG_FILES_DIR_LINUX << fs::path::preferred_separator << CFG_HISTORY_DB_FILE;
                            #elif _WIN32
                            oss << home_dir.string() << fs::path::preferred_separator << CFG_FILES_DIR_WNDWS << fs::path::preferred_separator << CFG_HISTORY_DB_FILE;
                            #endif

                            QMessageBox::warning(nullptr, tr("Error!"), tr("We have encountered some corrupt data! If this problem persists, please delete:\n\n\"%1\"")
                                    .arg(QString::fromStdString(oss.str())), QMessageBox::Ok);
                            continue;
                        }
                    }

                    // Each key identified by something such as, 'LEVELDB_KEY_TORRENT_INSERT_DATE', will store MANY values
                    // whilst the Unique ID will be unique per 'GekkoFyre::GkTorrent::TorrentInfo' object.
                    mmap_store.insert(std::make_pair(arg_key, tmp_unique_id), tmp_val);

                    // This is for speeding up searches later on in the function
                    if (!found_unique_id.contains(tmp_unique_id)) {
                        found_unique_id.push_back(tmp_unique_id);
                    }
                }
            } else {
                break;
            }
        } else {
            // There is no XML data present
            continue;
        }
    }

    // We need to match each item of struct 'GekkoFyre::GkTorrent::TorrentInfo' to its respective place, but to do that,
    // we must first match a Unique Identifier to each 'GekkoFyre::GkTorrent::TorrentInfo' object.
    std::vector<GekkoFyre::GkTorrent::GeneralInfo> vec_output;
    for (const auto &i: mmap_store.keys()) {
        // Expand out the list of keys stored within the QMultiMap
        for (const auto &j: found_unique_id) {
            if (i.second == j) {

            }
        }
    }

    for (auto i = 0; i < found_unique_id.size(); ++i) {
        GekkoFyre::GkTorrent::GeneralInfo tor_info;
        for (const auto &j: args) {
            std::pair<std::string, std::string> key_pair = std::make_pair(j, found_unique_id.at(i));
            if (j == LEVELDB_KEY_TORRENT_FLOC) {
                tor_info.down_dest = mmap_store.value(key_pair);
            } else if (j == LEVELDB_KEY_TORRENT_INSERT_DATE) {
                tor_info.insert_timestamp = atoll(mmap_store.value(key_pair).c_str());
            } else if (j == LEVELDB_KEY_TORRENT_COMPLT_DATE) {
                tor_info.complt_timestamp = atoll(mmap_store.value(key_pair).c_str());
            } else if (j == LEVELDB_KEY_TORRENT_CREATN_DATE) {
                tor_info.creatn_timestamp = atol(mmap_store.value(key_pair).c_str());
            } else if (j == LEVELDB_KEY_TORRENT_DLSTATUS) {
                tor_info.dlStatus = convDlStat_StringToEnum(QString::fromStdString(mmap_store.value(key_pair)));
            } else if (j == LEVELDB_KEY_TORRENT_TORRNT_COMMENT) {
                tor_info.comment = mmap_store.value(key_pair);
            } else if (j == LEVELDB_KEY_TORRENT_TORRNT_CREATOR) {
                tor_info.creator = mmap_store.value(key_pair);
            } else if (j == LEVELDB_KEY_TORRENT_MAGNET_URI) {
                tor_info.magnet_uri = mmap_store.value(key_pair);
            } else if (j == LEVELDB_KEY_TORRENT_TORRNT_NAME) {
                tor_info.torrent_name = mmap_store.value(key_pair);
            } else if (j == LEVELDB_KEY_TORRENT_NUM_FILES) {
                tor_info.num_files = atoi(mmap_store.value(key_pair).c_str());
            } else if (j == LEVELDB_KEY_TORRENT_TORRNT_PIECES) {
                tor_info.num_pieces = atoi(mmap_store.value(key_pair).c_str());
            } else if (j == LEVELDB_KEY_TORRENT_TORRNT_PIECE_LENGTH) {
                tor_info.piece_length = atoi(mmap_store.value(key_pair).c_str());
            }
        }

        tor_info.unique_id = found_unique_id.at(i);
        vec_output.push_back(tor_info);
    }

    if (vec_output.size() > 0) {
        return vec_output;
    }

    return std::vector<GekkoFyre::GkTorrent::GeneralInfo>();
}

/**
 * @brief GekkoFyre::CmnRoutines::process_db_xml processes a string containing specific raw XML data into two outputs.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-12
 * @param xml_input The string containing the specifically formatted XML data.
 * @return A container of the two aforementioned outputs are generated, specifically, the value being stored in question
 * and the Unique ID relating to that value so that it maybe identified as part of a larger object.
 * @see GekkoFyre::CmnRoutines::batch_write_single_db()
 */
std::vector<GekkoFyre::GkFile::FileDbVal> GekkoFyre::CmnRoutines::process_db_xml(const std::string &xml_input)
{
    if (!xml_input.empty() && xml_input.size() > CFG_XML_MIN_PARSE_SIZE) {
        std::vector<GekkoFyre::GkFile::FileDbVal> ret_data;
        pugi::xml_node root;
        std::unique_ptr<pugi::xml_document> doc = std::make_unique<pugi::xml_document>();
        pugi::xml_parse_result result = doc->load_string(xml_input.c_str());
        if (!result) {
            throw std::invalid_argument(tr("There has been an error whilst processing the XML from the database.\n\nParse error: %1, character pos = %2")
                                                .arg(result.description()).arg(result.offset).toStdString());
        }

        pugi::xml_node items = doc->child(LEVELDB_PARENT_NODE);
        for (const auto &file: items.children(LEVELDB_XML_CHILD_NODE)) {
            GekkoFyre::GkFile::FileDbVal file_db_val;
            file_db_val.dl_type = convDownType_StringToEnum(file.attribute(LEVELDB_XML_ATTR_DL_TYPE).as_string()); // TODO: Fix this immediately! Should be 'IntToEnum'.
            file_db_val.value = file.attribute(LEVELDB_XML_ATTR_ITEM_VALUE).as_string();
            file_db_val.unique_id = file.attribute(LEVELDB_ITEM_ATTR_UNIQUE_ID).as_string();
            ret_data.push_back(file_db_val);
        }

        if (ret_data.size() > 0) {
            return ret_data;
        }
    }

    return std::vector<GekkoFyre::GkFile::FileDbVal>();
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
    gk_torrent_struct.general.cId = 0;
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

    gk_torrent_struct.general.num_pieces = t.num_pieces();
    gk_torrent_struct.general.piece_length = t.piece_length();
    gk_torrent_struct.general.comment = t.comment();
    gk_torrent_struct.general.creator = t.creator();
    gk_torrent_struct.general.magnet_uri = make_magnet_uri(t);
    gk_torrent_struct.general.num_files = t.num_files();
    // gk_torrent_struct.creatn_timestamp = t.creation_date().get();
    gk_torrent_struct.general.torrent_name = t.name();
    gk_torrent_struct.general.unique_id = createId(FYREDL_UNIQUE_ID_DIGIT_COUNT);

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
 * @brief GekkoFyre::CmnRoutines::clearLayout will clear any QLayout, such as a QVBoxLayout, of any widgets.
 * @note <http://stackoverflow.com/questions/4857188/clearing-a-layout-in-qt>
 *       <http://stackoverflow.com/questions/4272196/qt-remove-all-widgets-from-layout>
 * @param layout
 */
void GekkoFyre::CmnRoutines::clearLayout(QLayout *layout)
{
    QLayoutItem *item;
    while ((item = layout->takeAt(0))) {
        if (item->layout()) {
            clearLayout(item->layout());
            delete item->layout();
        }

        if (item->widget()) {
            delete item->widget();
        }

        delete item;
    }
}

/**
 * @brief GekkoFyre::CmnRoutines::readDownloadInfo extracts the history information from 'CFG_HISTORY_FILE' relating to
 * HTTP(S) or FTP(S) downloads that have been added to FyreDL. This excludes BitTorrent downloads, which are handled by
 * different functions.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10
 * @note   <http://www.gerald-fahrnholz.eu/sw/DocGenerated/HowToUse/html/group___grp_pugi_xml.html>
 *         <http://stackoverflow.com/questions/16155888/proper-way-to-parse-xml-using-pugixml>
 * @param xmlCfgFile is the XML history file in question.
 * @param hashesOnly excludes all the 'extended' information by not loading it into memory. *** Warning! The extended
 * information will not be initialized! ***
 * @return A STL standard container holding a struct pertaining to all the needed CURL information is returned.
 */
std::vector<GekkoFyre::GkCurl::CurlDlInfo> GekkoFyre::CmnRoutines::readDownloadInfo(const std::string &xmlCfgFile,
                                                                                    const bool &hashesOnly)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (fs::exists(xmlCfgFile_loc, ec) && fs::is_regular_file(xmlCfgFile_loc)) {
        std::vector<GekkoFyre::GkCurl::CurlDlInfo> dl_info_list;

        pugi::xml_document doc;
        mutex.lock();
        pugi::xml_parse_result result = doc.load_file(xmlCfgFile_loc.string().c_str(), pugi::parse_default|pugi::parse_declaration);
        mutex.unlock();
            if (!result) {
                throw std::invalid_argument(tr("XML parse error: %1, character pos= %2")
                                            .arg(result.description(),
                                                 QString::number(result.offset)).toStdString());
            }

        pugi::xml_node items = doc.child(XML_PARENT_NODE);
        for (const auto& file: items.children(XML_CHILD_NODE_FILE)) {
            for (const auto& item: file.children(XML_CHILD_ITEM_FILE)) {
                GekkoFyre::GkCurl::CurlDlInfo i;
                if (hashesOnly) {
                    // Only a minimum of information is required
                    i.cId = item.attribute(XML_ITEM_ATTR_FILE_CID).as_uint();
                    i.unique_id = item.attribute(XML_ITEM_ATTR_FILE_UNIQUE_ID).as_string();
                    i.file_loc = item.attribute(XML_ITEM_ATTR_FILE_FLOC).value();
                    i.dlStatus = GekkoFyre::DownloadStatus::Unknown;
                    i.insert_timestamp = 0;
                    i.ext_info.status_msg = "";
                    i.ext_info.effective_url = item.attribute(XML_ITEM_ATTR_FILE_EFFEC_URL).value();
                    i.ext_info.response_code = -1;
                    i.ext_info.content_length = -1.0;
                    i.hash_type = convHashType_IntToEnum(item.attribute(XML_ITEM_ATTR_FILE_HASH_TYPE).as_int());
                    i.hash_val_given = item.attribute(XML_ITEM_ATTR_FILE_HASH_VAL_GIVEN).value();
                    i.hash_val_rtrnd = item.attribute(XML_ITEM_ATTR_FILE_HASH_VAL_RTRND).value();
                    i.hash_succ_type = convHashVerif_IntToEnum(item.attribute(XML_ITEM_ATTR_FILE_HASH_SUCC_TYPE).as_int());
                } else {
                    // We require /all/ the information that the XML history file can provide
                    i.cId = item.attribute(XML_ITEM_ATTR_FILE_CID).as_uint();
                    i.unique_id = item.attribute(XML_ITEM_ATTR_FILE_UNIQUE_ID).as_string();
                    i.file_loc = item.attribute(XML_ITEM_ATTR_FILE_FLOC).value();
                    i.dlStatus = convDlStat_IntToEnum(item.attribute(XML_ITEM_ATTR_FILE_STAT).as_int());
                    i.insert_timestamp = item.attribute(XML_ITEM_ATTR_FILE_INSERT_DATE).as_llong();
                    i.complt_timestamp = item.attribute(XML_ITEM_ATTR_FILE_COMPLT_DATE).as_llong();
                    i.ext_info.status_msg = item.attribute(XML_ITEM_ATTR_FILE_STATMSG).value();
                    i.ext_info.effective_url = item.attribute(XML_ITEM_ATTR_FILE_EFFEC_URL).value();
                    i.ext_info.response_code = item.attribute(XML_ITEM_ATTR_FILE_RESP_CODE).as_llong();
                    i.ext_info.content_length = item.attribute(XML_ITEM_ATTR_FILE_CONT_LNGTH).as_double();
                    i.hash_type = convHashType_IntToEnum(item.attribute(XML_ITEM_ATTR_FILE_HASH_TYPE).as_int());
                    i.hash_val_given = item.attribute(XML_ITEM_ATTR_FILE_HASH_VAL_GIVEN).value();
                    i.hash_val_rtrnd = item.attribute(XML_ITEM_ATTR_FILE_HASH_VAL_RTRND).value();
                    i.hash_succ_type = convHashVerif_IntToEnum(item.attribute(XML_ITEM_ATTR_FILE_HASH_SUCC_TYPE).as_int());
                }

                dl_info_list.push_back(i);
            }
        }

        return dl_info_list;
    }

    return std::vector<GekkoFyre::GkCurl::CurlDlInfo>();
}

/**
 * @brief GekkoFyre::CmnRoutines::writeDownloadInfo
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10
 * @note   <http://stackoverflow.com/questions/9387610/what-xml-parser-should-i-use-in-c>
 *         <http://www.gerald-fahrnholz.eu/sw/DocGenerated/HowToUse/html/group___grp_pugi_xml.html>
 * @param dl_info
 * @param xmlCfgFile is the XML history file in question.
 * @return
 */
bool GekkoFyre::CmnRoutines::writeDownloadItem(GekkoFyre::GkCurl::CurlDlInfo &dl_info,
                                               const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (dl_info.ext_info.status_ok) {
        if (!xmlCfgFile_loc.string().empty()) {
            if (dl_info.dlStatus == GekkoFyre::DownloadStatus::Stopped || dl_info.dlStatus == GekkoFyre::DownloadStatus::Invalid ||
                    dl_info.dlStatus == GekkoFyre::DownloadStatus::Unknown) {
                pugi::xml_node root;
                unsigned int cId = 0;
                dl_info.dlStatus = GekkoFyre::DownloadStatus::Unknown;
                QDateTime now = QDateTime::currentDateTime();
                dl_info.insert_timestamp = now.toTime_t();
                dl_info.complt_timestamp = 0;
                dl_info.ext_info.status_msg = "";

                // Generate new XML document within memory
                pugi::xml_document doc;
                // Alternatively store as shared pointer if tree shall be used for longer
                // time or multiple client calls:
                // std::shared_ptr<pugi::xml_document> spDoc = std::make_shared<pugi::xml_document>();

                if (!fs::exists(xmlCfgFile_loc, ec) && !fs::is_regular_file(xmlCfgFile_loc)) {
                    root = createNewXmlFile(xmlCfgFile);
                } else {
                    std::vector<GekkoFyre::GkCurl::CurlDlInfo> existing_info = readDownloadInfo(xmlCfgFile);

                    // Finds the largest 'existing_info.cId' value and then increments by +1, ready for
                    // assignment to a new download entry.
                    for (size_t i = 0; i < existing_info.size(); ++i) {
                        static unsigned int tmp;
                        tmp = existing_info.at(i).cId;
                        if (tmp > cId) {
                            cId = tmp;
                        } else {
                            ++cId;
                        }
                    }
                }

                // Load XML into memory
                // Remark: to fully read declaration entries you have to specify, "pugi::parse_declaration"
                pugi::xml_parse_result result = doc.load_file(xmlCfgFile_loc.string().c_str(), pugi::parse_default|pugi::parse_declaration);
                if (!result) {
                    throw std::invalid_argument(tr("XML parse error: %1, character pos= %2")
                                                        .arg(result.description(),
                                                             QString::number(result.offset)).toStdString());
                }

                dl_info.cId = cId;

                // A valid XML document must have a single root node
                root = doc.document_element();

                pugi::xml_node nodeParent = root.append_child(XML_CHILD_NODE_FILE);
                pugi::xml_node nodeChild = nodeParent.append_child(XML_CHILD_ITEM_FILE);
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_CID) = dl_info.cId;
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_FLOC) = dl_info.file_loc.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_STAT) = convDlStat_toInt(dl_info.dlStatus);
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_INSERT_DATE) = dl_info.insert_timestamp;
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_COMPLT_DATE) = dl_info.complt_timestamp;
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_STATMSG) = dl_info.ext_info.status_msg.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_EFFEC_URL) = dl_info.ext_info.effective_url.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_RESP_CODE) = dl_info.ext_info.response_code;
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_CONT_LNGTH) = dl_info.ext_info.content_length;
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_UNIQUE_ID) = dl_info.unique_id.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_HASH_TYPE) = convHashType_toInt(dl_info.hash_type);
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_HASH_VAL_GIVEN) = dl_info.hash_val_given.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_HASH_VAL_RTRND) = dl_info.hash_val_rtrnd.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_FILE_HASH_SUCC_TYPE) = convHashVerif_toInt(dl_info.hash_succ_type);

                bool saveSucceed = doc.save_file(xmlCfgFile_loc.string().c_str(), PUGIXML_TEXT("    "));
                if (!saveSucceed) {
                    throw std::runtime_error(tr("Error with saving XML config file!").toStdString());
                }

                return true;
            } else {
                throw std::invalid_argument(tr("You should not be seeing this!").toStdString());
            }
        } else {
            throw std::invalid_argument(tr("Internal error: no path has been given for the XML "
                                           "configuration file!").toStdString());
        }
    }

    return false;
}

pugi::xml_node GekkoFyre::CmnRoutines::createNewXmlFile(const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    pugi::xml_document doc;

    // Generate XML declaration
    auto declarNode = doc.append_child(pugi::node_declaration);
    declarNode.append_attribute("version") = "1.0";
    declarNode.append_attribute("encoding") = "UTF-8";;
    declarNode.append_attribute("standalone") = "yes";

    // A valid XML doc must contain a single root node of any name
    pugi::xml_node root = doc.append_child(XML_PARENT_NODE);

    pugi::xml_node xml_version_node = root.append_child(XML_CHILD_NODE_VERS);
    pugi::xml_node xml_version_child = xml_version_node.append_child(XML_CHILD_ITEM_VERS);
    xml_version_child.append_attribute(XML_ITEM_ATTR_VERS_NO) = FYREDL_PROG_VERS;

    // Save XML tree to file.
    // Remark: second optional param is indent string to be used;
    // default indentation is tab character.
   bool saveSucceed = doc.save_file(xmlCfgFile_loc.string().c_str(), PUGIXML_TEXT("    "));
   if (!saveSucceed) {
       throw std::runtime_error(tr("Error with saving XML config file!").toStdString());
   }

   return root;
}

/**
 * @brief GekkoFyre::CmnRoutines::delDownloadItem removes a node from the XML session file; 'CFG_HISTORY_FILE'.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10
 * @note   <http://www.gerald-fahrnholz.eu/sw/DocGenerated/HowToUse/html/group___grp_pugi_xml.html>
 *         <http://stackoverflow.com/questions/19196683/c-pugixml-get-children-of-parent-by-attribute-id>
 * @param dl_info
 * @param xmlCfgFile is the XML history file in question.
 * @return
 */
bool GekkoFyre::CmnRoutines::delDownloadItem(const QString &file_dest, const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (fs::exists(xmlCfgFile_loc, ec) && fs::is_regular_file(xmlCfgFile_loc)) {
        std::vector<GekkoFyre::GkCurl::CurlDlInfo> dl_info_list;
        dl_info_list = readDownloadInfo(xmlCfgFile);

        pugi::xml_document doc;

        // Load XML into memory
        // Remark: to fully read declaration entries you have to specify, "pugi::parse_declaration"
        pugi::xml_parse_result result = doc.load_file(xmlCfgFile_loc.string().c_str(),
                                                      pugi::parse_default|pugi::parse_declaration);
        if (!result) {
            throw std::invalid_argument(tr("XML parse error: %1, character pos= %2")
                                        .arg(result.description(),
                                             QString::number(result.offset)).toStdString());
        }

        pugi::xml_node items = doc.child(XML_PARENT_NODE);
        for (const auto& file: items.children(XML_CHILD_NODE_FILE)) {
            for (const auto& item: file.children(XML_CHILD_ITEM_FILE)) {
                if (file.find_child_by_attribute(XML_ITEM_ATTR_FILE_FLOC, file_dest.toStdString().c_str())) {
                    item.parent().remove_child(item);
                }
            }
        }

        bool saveSucceed = doc.save_file(xmlCfgFile_loc.string().c_str(), PUGIXML_TEXT("    "));
        if (!saveSucceed) {
            throw std::runtime_error(tr("Error with saving XML config file!").toStdString());
        }

        return true;
    } else {
        throw std::invalid_argument(tr("XML config file does not exist! Location attempt: "
                                       "%1").arg(xmlCfgFile_loc.string().c_str()).toStdString());
    }

    return false;
}

/**
 * @brief GekkoFyre::CmnRoutines::modifyDlState allows the modification of the download state, whether it
 * be 'paused', 'actively downloading', 'unknown', or something else, it will update the given XML history file.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-10
 * @param file_loc relates to the location of the download on the user's local storage.
 * @param status is the download state you wish to change towards.
 * @param hash_checksum is the calculated checksum of the file in question.
 * @param ret_succ_type regards whether the checksum comparison with the original, given checksum was a success or not.
 * @param xmlCfgFile is the XML history file in question.
 * @return Whether the operation was a success or not.
 */
bool GekkoFyre::CmnRoutines::modifyDlState(const std::string &file_loc,
                                           const GekkoFyre::DownloadStatus &status,
                                           const std::string &hash_checksum,
                                           const GekkoFyre::HashVerif &ret_succ_type,
                                           const GekkoFyre::HashType &hash_type,
                                           const long long &complt_timestamp,
                                           const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (fs::exists(xmlCfgFile_loc, ec) && fs::is_regular_file(xmlCfgFile_loc)) {
        pugi::xml_document doc;

        // Load XML into memory
        // Remark: to fully read declaration entries you have to specify, "pugi::parse_declaration"
        pugi::xml_parse_result result = doc.load_file(xmlCfgFile_loc.string().c_str(),
                                                      pugi::parse_default|pugi::parse_declaration);
        if (!result) {
            throw std::invalid_argument(tr("XML parse error: %1, character pos= %2")
                                        .arg(result.description(),
                                             QString::number(result.offset)).toStdString());
        }

        pugi::xml_node items = doc.child(XML_PARENT_NODE);
        for (const auto& file: items.children(XML_CHILD_NODE_FILE)) {
            for (const auto& item: file.children(XML_CHILD_ITEM_FILE)) {
                if (file.find_child_by_attribute(XML_ITEM_ATTR_FILE_FLOC, file_loc.c_str())) {
                    item.attribute(XML_ITEM_ATTR_FILE_STAT).set_value(convDlStat_toInt(status));
                    if (!hash_checksum.empty()) {
                        if (hash_type != GekkoFyre::HashType::None) {
                            item.attribute(XML_ITEM_ATTR_FILE_HASH_TYPE).set_value(convHashType_toInt(hash_type));
                        }

                        item.attribute(XML_ITEM_ATTR_FILE_HASH_VAL_RTRND).set_value(hash_checksum.c_str());
                        item.attribute(XML_ITEM_ATTR_FILE_HASH_SUCC_TYPE).set_value(convHashVerif_toInt(ret_succ_type));
                    }

                    if (complt_timestamp > 0) {
                        item.attribute(XML_ITEM_ATTR_FILE_COMPLT_DATE).set_value(complt_timestamp);
                    }
                }
            }
        }

        bool saveSucceed = doc.save_file(xmlCfgFile_loc.string().c_str(), PUGIXML_TEXT("    "));
        if (!saveSucceed) {
            throw std::runtime_error(tr("Error with saving XML config file!").toStdString());
        }

        return true;
    } else {
        throw std::invalid_argument(tr("XML config file does not exist! Location attempt: "
                                       "%1").arg(xmlCfgFile_loc.string().c_str()).toStdString());
    }

    return false;
}

/**
 * @note <http://www.thomaswhitton.com/blog/2013/07/01/xml-c-plus-plus-examples/>
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-12
 * @param gk_ti
 * @return
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

                    using namespace GekkoFyre::GkFile;
                    batch_write_single_db(LEVELDB_KEY_TORRENT_FLOC, gk_ti.general.down_dest, gk_ti.general.unique_id, db_struct);

                    batch_write_single_db(LEVELDB_KEY_TORRENT_INSERT_DATE,
                                          std::to_string(gk_ti.general.insert_timestamp),
                                          gk_ti.general.unique_id, db_struct);

                    batch_write_single_db(LEVELDB_KEY_TORRENT_COMPLT_DATE,
                                          std::to_string(gk_ti.general.complt_timestamp),
                                          gk_ti.general.unique_id, db_struct);

                    batch_write_single_db(LEVELDB_KEY_TORRENT_CREATN_DATE,
                                          std::to_string(gk_ti.general.creatn_timestamp),
                                          gk_ti.general.unique_id, db_struct);

                    batch_write_single_db(LEVELDB_KEY_TORRENT_DLSTATUS,
                                          convDlStat_toString(gk_ti.general.dlStatus).toStdString(), gk_ti.general.unique_id, db_struct);
                    batch_write_single_db(LEVELDB_KEY_TORRENT_TORRNT_COMMENT, gk_ti.general.comment, gk_ti.general.unique_id, db_struct);
                    batch_write_single_db(LEVELDB_KEY_TORRENT_TORRNT_CREATOR, gk_ti.general.creator, gk_ti.general.unique_id, db_struct);
                    batch_write_single_db(LEVELDB_KEY_TORRENT_MAGNET_URI, gk_ti.general.magnet_uri, gk_ti.general.unique_id, db_struct);
                    batch_write_single_db(LEVELDB_KEY_TORRENT_TORRNT_NAME, gk_ti.general.torrent_name, gk_ti.general.unique_id, db_struct);
                    batch_write_single_db(LEVELDB_KEY_TORRENT_NUM_FILES, std::to_string(gk_ti.general.num_files),
                                          gk_ti.general.unique_id, db_struct);
                    batch_write_single_db(LEVELDB_KEY_TORRENT_TORRNT_PIECES, std::to_string(gk_ti.general.num_pieces),
                                          gk_ti.general.unique_id, db_struct);
                    batch_write_single_db(LEVELDB_KEY_TORRENT_TORRNT_PIECE_LENGTH,
                                          std::to_string(gk_ti.general.piece_length), gk_ti.general.unique_id, db_struct);

                    //
                    // Files and Trackers
                    //
                    bool resp = writeTorrent_extra_data(gk_ti, db_struct);
                    return resp ? true : false;
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

bool GekkoFyre::CmnRoutines::writeTorrent_extra_data(const GekkoFyre::GkTorrent::TorrentInfo &gk_ti,
                                                     const GekkoFyre::GkFile::FileDb &ext_db_struct)
{
    try {
        pugi::xml_node root;
        std::unique_ptr<pugi::xml_document> doc = std::make_unique<pugi::xml_document>();

        const std::string key = LEVELDB_KEY_TORRENT_EXTRANEOUS;
        std::string existing_value;
        leveldb::ReadOptions read_opt;
        leveldb::WriteOptions write_opt;
        leveldb::Status s;
        leveldb::WriteBatch batch;
        read_opt.verify_checksums = true;
        write_opt.sync = true;
        s = ext_db_struct.db->Get(read_opt, key, &existing_value);
        if (!existing_value.empty() && existing_value.size() > CFG_XML_MIN_PARSE_SIZE) { // Check if previous XML data exists or not
            //
            // There is previous XML data, which we will build on
            //
            pugi::xml_parse_result result = doc->load_string(existing_value.c_str());
            if (!result) {
                throw std::invalid_argument(tr("There has been an error whilst reading extraneous information about a BitTorrent item from the database.\n\nParse error: %1, character pos = %2")
                                                    .arg(result.description()).arg(result.offset).toStdString());
            }

            batch.Delete(key);
        } else {
            //
            // No previous records exist, therefore create a new XML document from scratch!
            //
            std::stringstream new_xml_data;
            // Generate XML declaration
            auto declarNode = doc->append_child(pugi::node_declaration);
            declarNode.append_attribute("version") = "1.0";
            declarNode.append_attribute("encoding") = "UTF-8";;
            declarNode.append_attribute("standalone") = "yes";

            // A valid XML doc must contain a single root node of any name
            root = doc->append_child(LEVELDB_PARENT_NODE);
            pugi::xml_node xml_version_node = root.append_child(LEVELDB_CHILD_NODE_VERS);
            pugi::xml_node xml_version_child = xml_version_node.append_child(LEVELDB_CHILD_ITEM_VERS);
            xml_version_child.append_attribute(LEVELDB_ITEM_ATTR_VERS_NO) = FYREDL_PROG_VERS;
        }

        root = doc->document_element();
        pugi::xml_node nodeParent = root.append_child(LEVELDB_XML_CHILD_NODE);
        nodeParent.append_child(pugi::node_pcdata).set_value(gk_ti.general.unique_id.c_str());
        pugi::xml_node node_val_files = nodeParent.append_child(LEVELDB_CHILD_NODE_TORRENT_FILES);
        for (size_t i = 0; i < gk_ti.files_vec.size(); ++i) {
            //
            // Files
            //
            pugi::xml_node nodeFilesPath = node_val_files.append_child(LEVELDB_CHILD_FILES_PATH_TORRENT);
            nodeFilesPath.append_child(pugi::node_pcdata)
                    .set_value(gk_ti.files_vec.at(i).file_path.c_str());

            // Sub-nodes
            pugi::xml_node nodeFilesHash = nodeFilesPath.append_child(LEVELDB_CHILD_FILES_HASH_TORRENT);
            nodeFilesHash.append_child(pugi::node_pcdata)
                    .set_value(gk_ti.files_vec.at(i).sha1_hash_hex.c_str());

            pugi::xml_node nodeFilesFlags = nodeFilesPath.append_child(LEVELDB_CHILD_FILES_FLAGS_TORRENT);
            nodeFilesFlags.append_child(pugi::node_pcdata).set_value(std::to_string(gk_ti.files_vec.at(i).flags).data());

            pugi::xml_node nodeFilesContentLength = nodeFilesPath.append_child(LEVELDB_CHILD_FILES_CONTLNGTH_TORRENT);
            nodeFilesContentLength.append_child(pugi::node_pcdata)
                    .set_value(std::to_string(gk_ti.files_vec.at(i).content_length).data());

            pugi::xml_node nodeFilesFileOffset = nodeFilesPath.append_child(LEVELDB_CHILD_FILES_FILEOFFST_TORRENT);
            nodeFilesFileOffset.append_child(pugi::node_pcdata)
                    .set_value(std::to_string(gk_ti.files_vec.at(i).file_offset).data());

            pugi::xml_node nodeFilesMTime = nodeFilesPath.append_child(LEVELDB_CHILD_FILES_MTIME_TORRENT);
            nodeFilesMTime.append_child(pugi::node_pcdata).set_value(std::to_string(gk_ti.files_vec.at(i).mtime).data());

            pugi::xml_node nodeFilesMapFilePiece = nodeFilesPath.append_child(LEVELDB_CHILD_NODE_TORRENT_FILES_MAPFLEPCE);
            pugi::xml_node nodeFilesMapFilePiece_first = nodeFilesMapFilePiece
                    .append_child(LEVELDB_CHILD_FILES_MAPFLEPCE_1_TORRENT);
            pugi::xml_node nodeFilesMapFilePiece_second = nodeFilesMapFilePiece
                    .append_child(LEVELDB_CHILD_FILES_MAPFLEPCE_2_TORRENT);
            nodeFilesMapFilePiece_first.append_child(pugi::node_pcdata)
                    .set_value(std::to_string(gk_ti.files_vec.at(i).map_file_piece.first).data());
            nodeFilesMapFilePiece_second.append_child(pugi::node_pcdata)
                    .set_value(std::to_string(gk_ti.files_vec.at(i).map_file_piece.second).data());

            pugi::xml_node nodeFilesDownBool = nodeFilesPath.append_child(LEVELDB_CHILD_FILES_DOWNBOOL_TORRENT);
            nodeFilesDownBool.append_child(pugi::node_pcdata)
                    .set_value(std::to_string(gk_ti.files_vec.at(i).downloaded).data());
        }

        pugi::xml_node node_val_trackers = nodeParent.append_child(LEVELDB_CHILD_NODE_TORRENT_TRACKERS);
        for (size_t i = 0; i < gk_ti.trackers.size(); ++i) {
            //
            // Trackers
            //
            pugi::xml_node nodeTrackersTier = node_val_trackers.append_child(LEVELDB_CHILD_TRACKERS_TIER_TORRENT);
            nodeTrackersTier.append_child(pugi::node_pcdata).set_value(std::to_string(gk_ti.trackers.at(i).tier).data());

            // Sub-node
            pugi::xml_node nodeTrackersUrl = nodeTrackersTier.append_child(LEVELDB_CHILD_TRACKERS_URL_TORRENT);
            nodeTrackersUrl.append_child(pugi::node_pcdata).set_value(gk_ti.trackers.at(i).url.c_str());

            pugi::xml_node nodeTrackersAvailable = nodeTrackersTier.append_child(LEVELDB_CHILD_TRACKERS_AVAILABLE_TORRENT);
            nodeTrackersAvailable.append_child(pugi::node_pcdata).set_value(std::to_string(gk_ti.trackers.at(i).enabled).data());
        }

        std::stringstream ss;
        doc->save(ss, PUGIXML_TEXT("    "));

        batch.Put(LEVELDB_KEY_TORRENT_EXTRANEOUS, ss.str());
        s = ext_db_struct.db->Write(write_opt, &batch);
        if (s.ok()) {
            return true;
        } else {
            throw std::runtime_error(s.ToString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), tr("An issue was encountered whilst writing the extraneous data for torrent item, \"%1\".").arg(QString::fromStdString(gk_ti.general.unique_id)),
                             QMessageBox::Ok);
        return false;
    }

    return false;
}

/**
 * @brief GekkoFyre::CmnRoutines::readTorrentInfo extracts the history information from 'CFG_HISTORY_FILE' relating to
 * BitTorrent downloads that have been added to FyreDL. This excludes HTTP(S) and FTP(S) downloads, which are handled
 * by different functions.
 * @note <http://en.cppreference.com/w/cpp/header/cstdlib>
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-12
 * @param minimal_readout only extracts the most vital history information, thus (potentially) saving CPU time and
 * memory.
 * @param xmlCfgFile The XML configuration file in question.
 * @return A STL standard container holding a struct pertaining to all the needed BitTorrent information is returned.
 */
std::vector<GekkoFyre::GkTorrent::TorrentInfo> GekkoFyre::CmnRoutines::readTorrentInfo(const bool &minimal_readout)
{
    std::vector<GekkoFyre::GkTorrent::TorrentInfo> to_info_vec;
    try {
        GekkoFyre::GkFile::FileDb db_struct = openDatabase(CFG_HISTORY_DB_FILE);
        leveldb::Status s;
        if (s.ok()) {
            std::tuple<std::string, std::string> val_to_down_dest, val_to_insert_timestamp,
                    val_to_complt_timestamp, val_to_creatn_timestamp, val_to_status, val_to_comment, val_to_creator,
                    val_to_magnet_uri, val_to_name, val_to_num_files, val_to_num_pieces, val_to_piece_length;

            // Proceed, the database status is 'okay'!
            val_to_down_dest = read_db_min(LEVELDB_KEY_TORRENT_FLOC, db_struct);
            val_to_insert_timestamp = read_db_min(LEVELDB_KEY_TORRENT_INSERT_DATE, db_struct);
            val_to_complt_timestamp = read_db_min(LEVELDB_KEY_TORRENT_COMPLT_DATE, db_struct);
            val_to_creatn_timestamp = read_db_min(LEVELDB_KEY_TORRENT_CREATN_DATE, db_struct);
            val_to_status = read_db_min(LEVELDB_KEY_TORRENT_DLSTATUS, db_struct);
            val_to_comment = read_db_min(LEVELDB_KEY_TORRENT_TORRNT_COMMENT, db_struct);
            val_to_creator = read_db_min(LEVELDB_KEY_TORRENT_TORRNT_CREATOR, db_struct);
            val_to_magnet_uri = read_db_min(LEVELDB_KEY_TORRENT_MAGNET_URI, db_struct);
            val_to_name = read_db_min(LEVELDB_KEY_TORRENT_TORRNT_NAME, db_struct);
            val_to_num_files = read_db_min(LEVELDB_KEY_TORRENT_NUM_FILES, db_struct);
            val_to_num_pieces = read_db_min(LEVELDB_KEY_TORRENT_TORRNT_PIECES, db_struct);
            val_to_piece_length = read_db_min(LEVELDB_KEY_TORRENT_TORRNT_PIECE_LENGTH, db_struct);

            QMap<std::string, std::string> map_output;
            map_output = process_db({val_to_down_dest, val_to_insert_timestamp, val_to_complt_timestamp, val_to_creatn_timestamp,
                                     val_to_status, val_to_comment, val_to_creator, val_to_magnet_uri, val_to_name, val_to_num_files,
                                     val_to_num_pieces, val_to_piece_length});

            std::vector<GekkoFyre::GkTorrent::GeneralInfo> fin_output;
            fin_output = process_db_map(map_output, {LEVELDB_KEY_TORRENT_FLOC, LEVELDB_KEY_TORRENT_INSERT_DATE, LEVELDB_KEY_TORRENT_COMPLT_DATE,
                                        LEVELDB_KEY_TORRENT_CREATN_DATE, LEVELDB_KEY_TORRENT_DLSTATUS, LEVELDB_KEY_TORRENT_TORRNT_COMMENT,
                                        LEVELDB_KEY_TORRENT_TORRNT_CREATOR, LEVELDB_KEY_TORRENT_MAGNET_URI, LEVELDB_KEY_TORRENT_TORRNT_NAME,
                                        LEVELDB_KEY_TORRENT_NUM_FILES, LEVELDB_KEY_TORRENT_TORRNT_PIECES, LEVELDB_KEY_TORRENT_TORRNT_PIECE_LENGTH});

            std::string read_extraneous_xml;
            s = db_struct.db->Get(leveldb::ReadOptions(), LEVELDB_KEY_TORRENT_EXTRANEOUS, &read_extraneous_xml);

            pugi::xml_node root;
            std::unique_ptr<pugi::xml_document> doc = std::make_unique<pugi::xml_document>();
            if (!read_extraneous_xml.empty() && read_extraneous_xml.size() > CFG_XML_MIN_PARSE_SIZE) {
                pugi::xml_parse_result result = doc->load_string(read_extraneous_xml.c_str());
                if (!result) {
                    throw std::invalid_argument(tr("There has been an error reading information from the BitTorrent area of the database.\n\nParse error: %1, character pos = %2")
                                                        .arg(result.description()).arg(result.offset).toStdString());
                }

                pugi::xml_node nodeParent = doc->child(LEVELDB_XML_CHILD_NODE);

                //
                // Files
                //
                QMap<std::string, GekkoFyre::GkTorrent::TorrentFile> files_map;
                pugi::xml_node node_val_files = nodeParent.child(LEVELDB_CHILD_NODE_TORRENT_FILES);
                std::string file_unique_id = nodeParent.child_value();
                for (const auto &i: node_val_files.children(LEVELDB_CHILD_FILES_PATH_TORRENT)) {
                    GekkoFyre::GkTorrent::TorrentFile tf;
                    tf.file_path = i.child_value();
                    tf.sha1_hash_hex = i.child_value(LEVELDB_CHILD_FILES_HASH_TORRENT);
                    tf.mtime = strtoul(i.child_value(LEVELDB_CHILD_FILES_MTIME_TORRENT), nullptr, 10);
                    tf.file_offset = atol(i.child_value(LEVELDB_CHILD_FILES_FILEOFFST_TORRENT));
                    tf.content_length = atol(i.child_value(LEVELDB_CHILD_FILES_CONTLNGTH_TORRENT));
                    tf.flags = atoi(i.child_value(LEVELDB_CHILD_FILES_FLAGS_TORRENT));
                    tf.downloaded = atoi(i.child_value(LEVELDB_CHILD_FILES_DOWNBOOL_TORRENT));

                    pugi::xml_node file_map_child_node = i.child(LEVELDB_CHILD_NODE_TORRENT_FILES_MAPFLEPCE);
                    int map_int_one, map_int_two = { 0 };
                    map_int_one = atoi(file_map_child_node.child_value(LEVELDB_CHILD_FILES_MAPFLEPCE_1_TORRENT));
                    map_int_two = atoi(file_map_child_node.child_value(LEVELDB_CHILD_FILES_MAPFLEPCE_2_TORRENT));
                    tf.map_file_piece.first = map_int_one;
                    tf.map_file_piece.second = map_int_two;

                    files_map.insert(file_unique_id, tf);
                }

                //
                // Trackers
                //
                QMultiMap<std::string, GekkoFyre::GkTorrent::TorrentTrackers> trackers_map;
                pugi::xml_node node_val_trackers = nodeParent.child(LEVELDB_CHILD_NODE_TORRENT_TRACKERS);
                for (const auto &i: node_val_trackers.children(LEVELDB_CHILD_TRACKERS_TIER_TORRENT)) {
                    pugi::xml_node url_node = i.child(LEVELDB_CHILD_TRACKERS_URL_TORRENT);
                    pugi::xml_node avail_node = i.child(LEVELDB_CHILD_TRACKERS_AVAILABLE_TORRENT);
                    GekkoFyre::GkTorrent::TorrentTrackers tracker;
                    tracker.tier = atoi(i.child_value());
                    tracker.url = url_node.child_value();
                    tracker.enabled = atoi(avail_node.child_value());
                    trackers_map.insert(file_unique_id, tracker);
                }

                for (size_t i = 0; i < fin_output.size(); ++i) {
                    GekkoFyre::GkTorrent::TorrentInfo to_info;
                    if (files_map.keys().size() > 0) {
                        for (const auto &j: files_map.values(fin_output.at(i).unique_id)) {
                            // Expand the values within 'files_map'
                            GekkoFyre::GkTorrent::TorrentFile to_file;
                            to_file.unique_id = fin_output.at(i).unique_id;
                            to_file.file_path = j.file_path;
                            to_file.sha1_hash_hex = j.sha1_hash_hex;
                            to_file.flags = j.flags;
                            to_file.content_length = j.content_length;
                            to_file.file_offset = j.file_offset;
                            to_file.mtime = j.mtime;
                            to_file.map_file_piece = std::make_pair(j.map_file_piece.first, j.map_file_piece.second);
                            to_file.downloaded = j.downloaded;
                            to_info.files_vec.push_back(to_file);
                        }
                    } else {
                        QMessageBox::warning(nullptr, tr("Problem!"),
                                             tr("There is a BitTorrent item present that has no files to download. Deleting item..."),
                                             QMessageBox::Ok);
                        // TODO: Insert code for this action!
                    }

                    if (trackers_map.keys().size() > 0) {
                        for (const auto &j: trackers_map.values(fin_output.at(i).unique_id)) {
                            // Expand the values within 'trackers_map'
                            GekkoFyre::GkTorrent::TorrentTrackers to_tracker;
                            to_tracker.unique_id = fin_output.at(i).unique_id;
                            to_tracker.tier = j.tier;
                            to_tracker.url = j.url;
                            to_tracker.enabled = j.enabled;
                            to_info.trackers.push_back(to_tracker);
                        }
                    } else {
                        QMessageBox::warning(nullptr, tr("Problem!"),
                                             tr("There is a BitTorrent item present that has no trackers. Deleting item..."),
                                             QMessageBox::Ok);
                        // TODO: Insert code for this action!
                    }

                    to_info.general = fin_output.at(i);
                    to_info_vec.push_back(to_info);
                }
            }
        } else {
            // There is a problem with the database!
            throw std::runtime_error(tr("There is an issue with the database! Error: %1").arg(QString::fromStdString(s.ToString())).toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
        return std::vector<GekkoFyre::GkTorrent::TorrentInfo>();
    }

    return to_info_vec;
}

bool GekkoFyre::CmnRoutines::delTorrentItem(const std::string &unique_id, const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (fs::exists(xmlCfgFile_loc, ec) && fs::is_regular_file(xmlCfgFile_loc)) {
        std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_torrent_info;
        gk_torrent_info = readTorrentInfo(false);

        pugi::xml_document doc;

        // Load XML into memory
        // Remark: to fully read declaration entries you have to specify, "pugi::parse_declaration"
        pugi::xml_parse_result result = doc.load_file(xmlCfgFile_loc.string().c_str(),
                                                      pugi::parse_default | pugi::parse_declaration);
        if (!result) {
            throw std::invalid_argument(tr("XML parse error: %1, character pos= %2")
                                                .arg(result.description(),
                                                     QString::number(result.offset)).toStdString());
        }

        pugi::xml_node items = doc.child(XML_PARENT_NODE);
        for (const auto &file: items.children(XML_CHILD_NODE_TORRENT)) {
            for (const auto &item: file.children(XML_CHILD_ITEM_TORRENT)) {
                if (file.find_child_by_attribute(XML_ITEM_ATTR_TORRENT_UNIQUE_ID, unique_id.c_str())) {
                    item.parent().remove_child(item);
                }
            }
        }

        bool saveSucceed = doc.save_file(xmlCfgFile_loc.string().c_str(), PUGIXML_TEXT("    "));
        if (!saveSucceed) {
            throw std::runtime_error(tr("Error with saving XML config file!").toStdString());
        }

        return true;
    } else {
        throw std::invalid_argument(tr("XML config file does not exist! Location attempt: "
                                               "%1").arg(xmlCfgFile_loc.string().c_str()).toStdString());
    }
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

QString GekkoFyre::CmnRoutines::numberConverter(const double &value)
{
    QMutexLocker locker(&mutex);
    locker.relock(); // Should lock automatically though
    if (value < 1024) {
        QString conv = bytesToKilobytes(value);
        return conv;
    } else if (value > 1024) {
        QString conv = bytesToMegabytes(value);
        return conv;
    } else {
        QString conv = bytesToKilobytes(value);
        return conv;
    }
}

/**
 * @brief GekkoFyre::CmnRoutines::writeXmlSettings creates a new XML entry where there is none in regards to the saving
 * of settings information.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-06
 * @param settings The settings information that needs to be saved.
 * @param xmlCfgFile The XML configuration file in question.
 * @return '1' indicates an already existing entry, '0' indicates an entry was successfully added, and '-1' indicates
 * an error at an attempt to add an entry.
 */
short GekkoFyre::CmnRoutines::writeXmlSettings(const GekkoFyre::GkSettings::FyreDL &settings,
                                               const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (!xmlCfgFile_loc.string().empty()) {
        pugi::xml_node root;

        // Generate new XML document within memory
        pugi::xml_document doc;
        // Alternatively store as shared pointer if tree shall be used for longer
        // time or multiple client calls:
        // std::shared_ptr<pugi::xml_document> spDoc = std::make_shared<pugi::xml_document>();

        if (!fs::exists(xmlCfgFile_loc, ec) && !fs::is_regular_file(xmlCfgFile_loc)) {
            root = createNewXmlFile(xmlCfgFile);
        }

        // Load XML into memory
        // Remark: to fully read declaration entries you have to specify, "pugi::parse_declaration"
        pugi::xml_parse_result result = doc.load_file(xmlCfgFile_loc.string().c_str(), pugi::parse_default|pugi::parse_declaration);
        if (!result) {
            throw std::invalid_argument(tr("XML parse error: %1, character pos= %2")
                                                .arg(result.description(),
                                                     QString::number(result.offset)).toStdString());
        }

        unsigned short counter = 0;
        pugi::xml_node items = doc.child(XML_PARENT_NODE);
        for (const auto& file: items.children(XML_CHILD_NODE_SETTINGS)) {
            for (const auto &item: file.children(XML_CHILD_ITEM_SETTINGS)) {
                Q_UNUSED(item);
                ++counter;
            }
        }

        if (counter > 0) {
            return 1;
        }

        // A valid XML document must have a single root node
        root = doc.document_element();

        pugi::xml_node nodeParent = root.append_child(XML_CHILD_NODE_SETTINGS);
        pugi::xml_node nodeChild = nodeParent.append_child(XML_CHILD_ITEM_SETTINGS);
        if (settings.main_win_y > 0 && settings.main_win_x > 0) {
            nodeChild.append_attribute(XML_ITEM_ATTR_SETTINGS_WIN_Y) = settings.main_win_y;
            nodeChild.append_attribute(XML_ITEM_ATTR_SETTINGS_WIN_X) = settings.main_win_x;
        }

        bool saveSucceed = doc.save_file(xmlCfgFile_loc.string().c_str(), PUGIXML_TEXT("    "));
        if (!saveSucceed) {
            throw std::runtime_error(tr("Error with saving XML config file!").toStdString());
        } else {
            return 0;
        }
    } else {
        throw std::invalid_argument(tr("Internal error: no path has been given for the XML "
                                               "configuration file!").toStdString());
    }

    return -1;
}

/**
 * @brief GekkoFyre::CmnRoutines::modifyXmlSettings modifies an already pre-existing, XML configuration file for FyreDL.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-06
 * @param settings The settings information that needs to be saved.
 * @param xmlCfgFile The XML configuration file in question.
 * @return Whether the operation was a success or not.
 */
bool GekkoFyre::CmnRoutines::modifyXmlSettings(const GekkoFyre::GkSettings::FyreDL &settings,
                                               const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (fs::exists(xmlCfgFile_loc, ec) && fs::is_regular_file(xmlCfgFile_loc)) {
        pugi::xml_document doc;

        // Load XML into memory
        // Remark: to fully read declaration entries you have to specify, "pugi::parse_declaration"
        pugi::xml_parse_result result = doc.load_file(xmlCfgFile_loc.string().c_str(),
                                                      pugi::parse_default|pugi::parse_declaration);
        if (!result) {
            throw std::invalid_argument(tr("XML parse error: %1, character pos= %2")
                                                .arg(result.description(),
                                                     QString::number(result.offset)).toStdString());
        }

        pugi::xml_node items = doc.child(XML_PARENT_NODE);
        const auto xml_settings_node = items.child(XML_CHILD_NODE_SETTINGS);
        const auto xml_user_item = xml_settings_node.child(XML_CHILD_ITEM_SETTINGS);
        if (settings.main_win_x > 0 && settings.main_win_y > 0) {
            xml_user_item.attribute(XML_ITEM_ATTR_SETTINGS_WIN_X).set_value(settings.main_win_x);
            xml_user_item.attribute(XML_ITEM_ATTR_SETTINGS_WIN_Y).set_value(settings.main_win_y);
        }

        bool saveSucceed = doc.save_file(xmlCfgFile_loc.string().c_str(), PUGIXML_TEXT("    "));
        if (!saveSucceed) {
            throw std::runtime_error(tr("Error with saving XML config file!").toStdString());
        }

        return true;
    } else {
        throw std::invalid_argument(tr("XML config file does not exist! Location attempt: "
                                               "%1").arg(xmlCfgFile_loc.string().c_str()).toStdString());
    }

    return false;
}
