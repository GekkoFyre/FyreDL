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
    gk_torrent_struct.cId = 0;
    gk_torrent_struct.comment = "";
    gk_torrent_struct.complt_timestamp = 0;
    gk_torrent_struct.creatn_timestamp = 0;
    gk_torrent_struct.creator = "";
    gk_torrent_struct.dlStatus = GekkoFyre::DownloadStatus::Failed;
    gk_torrent_struct.down_dest = "";
    gk_torrent_struct.insert_timestamp = 0;
    gk_torrent_struct.magnet_uri = "";
    gk_torrent_struct.num_files = 0;
    gk_torrent_struct.num_pieces = 0;
    gk_torrent_struct.piece_length = 0;

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

    gk_torrent_struct.num_pieces = t.num_pieces();
    gk_torrent_struct.piece_length = t.piece_length();
    gk_torrent_struct.comment = t.comment();
    gk_torrent_struct.creator = t.creator();
    gk_torrent_struct.magnet_uri = make_magnet_uri(t);
    gk_torrent_struct.num_files = t.num_files();
    // gk_torrent_struct.creatn_timestamp = t.creation_date().get();
    gk_torrent_struct.torrent_name = t.name();
    gk_torrent_struct.unique_id = createId(FYREDL_UNIQUE_ID_DIGIT_COUNT);

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
 * @param xmlCfgFile
 * @return
 */
bool GekkoFyre::CmnRoutines::writeTorrentItem(GekkoFyre::GkTorrent::TorrentInfo &gk_ti,
                                              const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (!gk_ti.down_dest.empty()) {
        if (!xmlCfgFile_loc.string().empty()) {
            if (gk_ti.dlStatus == GekkoFyre::DownloadStatus::Stopped || gk_ti.dlStatus == GekkoFyre::DownloadStatus::Invalid ||
                    gk_ti.dlStatus == GekkoFyre::DownloadStatus::Unknown) {
                pugi::xml_node root;
                unsigned int cId = 0;
                QDateTime now = QDateTime::currentDateTime();
                gk_ti.insert_timestamp = now.toTime_t();

                // Generate new XML document within memory
                pugi::xml_document doc;
                // Alternatively store as shared pointer if tree shall be used for longer
                // time or multiple client calls:
                // std::shared_ptr<pugi::xml_document> spDoc = std::make_shared<pugi::xml_document>();

                if (!fs::exists(xmlCfgFile_loc, ec) && !fs::is_regular_file(xmlCfgFile_loc)) {
                    root = createNewXmlFile(xmlCfgFile);
                } else {
                    std::vector<GekkoFyre::GkTorrent::TorrentInfo> existing_info = readTorrentInfo(true, xmlCfgFile);

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

                gk_ti.cId = cId;

                // A valid XML document must have a single root node
                root = doc.document_element();

                pugi::xml_node nodeParent = root.append_child(XML_CHILD_NODE_TORRENT);
                pugi::xml_node nodeChild = nodeParent.append_child(XML_CHILD_ITEM_TORRENT);
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_CID) = gk_ti.cId;
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_FLOC) = gk_ti.down_dest.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_INSERT_DATE) = gk_ti.insert_timestamp;
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_COMPLT_DATE) = gk_ti.complt_timestamp;
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_CREATN_DATE) = (long long)gk_ti.creatn_timestamp;
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_DLSTATUS) = convDlStat_toInt(gk_ti.dlStatus);
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_TORRNT_COMMENT) = gk_ti.comment.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_TORRNT_CREATOR) = gk_ti.creator.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_MAGNET_URI) = gk_ti.magnet_uri.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_TORRNT_NAME) = gk_ti.torrent_name.c_str();
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_NUM_FILES) = gk_ti.num_files;
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_TORRNT_PIECES) = gk_ti.num_pieces;
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_TORRNT_PIECE_LENGTH) = gk_ti.piece_length;
                nodeChild.append_attribute(XML_ITEM_ATTR_TORRENT_UNIQUE_ID) = gk_ti.unique_id.c_str();

                pugi::xml_node nodeNodes = nodeChild.append_child(XML_CHILD_NODE_TORRENT_NODES);
                for (size_t i = 0; i < gk_ti.nodes.size(); ++i) {
                    pugi::xml_node nodeNodesNames = nodeNodes.append_child(XML_CHILD_NODES_NAMES_TORRENT);
                    nodeNodesNames.append_child(pugi::node_pcdata).set_value(gk_ti.nodes.at(i).first.c_str());

                    // Sub-node
                    std::string buffer = std::to_string(gk_ti.nodes.at(i).second);
                    pugi::xml_node nodeNodesNumbers = nodeNodesNames.append_child(XML_CHILD_NODES_NUMBR_TORRENT);
                    nodeNodesNumbers.append_child(pugi::node_pcdata).set_value(buffer.data());
                }

                pugi::xml_node nodeFiles = nodeChild.append_child(XML_CHILD_NODE_TORRENT_FILES);
                for (size_t i = 0; i < gk_ti.files_vec.size(); ++i) {
                    pugi::xml_node nodeFilesPath = nodeFiles.append_child(XML_CHILD_FILES_PATH_TORRENT);
                    nodeFilesPath.append_child(pugi::node_pcdata)
                            .set_value(gk_ti.files_vec.at(i).file_path.c_str());

                    // Sub-nodes
                    pugi::xml_node nodeFilesHash = nodeFilesPath.append_child(XML_CHILD_FILES_HASH_TORRENT);
                    nodeFilesHash.append_child(pugi::node_pcdata)
                            .set_value(gk_ti.files_vec.at(i).sha1_hash_hex.c_str());

                    pugi::xml_node nodeFilesFlags = nodeFilesPath.append_child(XML_CHILD_FILES_FLAGS_TORRENT);
                    nodeFilesFlags.append_child(pugi::node_pcdata).set_value(std::to_string(gk_ti.files_vec.at(i).flags).data());

                    pugi::xml_node nodeFilesContentLength = nodeFilesPath.append_child(XML_CHILD_FILES_CONTLNGTH_TORRENT);
                    nodeFilesContentLength.append_child(pugi::node_pcdata)
                            .set_value(std::to_string(gk_ti.files_vec.at(i).content_length).data());

                    pugi::xml_node nodeFilesFileOffset = nodeFilesPath.append_child(XML_CHILD_FILES_FILEOFFST_TORRENT);
                    nodeFilesFileOffset.append_child(pugi::node_pcdata)
                            .set_value(std::to_string(gk_ti.files_vec.at(i).file_offset).data());

                    pugi::xml_node nodeFilesMTime = nodeFilesPath.append_child(XML_CHILD_FILES_MTIME_TORRENT);
                    nodeFilesMTime.append_child(pugi::node_pcdata).set_value(std::to_string(gk_ti.files_vec.at(i).mtime).data());

                    pugi::xml_node nodeFilesMapFilePiece = nodeFilesPath.append_child(XML_CHILD_NODE_TORRENT_FILES_MAPFLEPCE);
                    pugi::xml_node nodeFilesMapFilePiece_first = nodeFilesMapFilePiece
                            .append_child(XML_CHILD_FILES_MAPFLEPCE_1_TORRENT);
                    pugi::xml_node nodeFilesMapFilePiece_second = nodeFilesMapFilePiece
                            .append_child(XML_CHILD_FILES_MAPFLEPCE_2_TORRENT);
                    nodeFilesMapFilePiece_first.append_child(pugi::node_pcdata)
                            .set_value(std::to_string(gk_ti.files_vec.at(i).map_file_piece.first).data());
                    nodeFilesMapFilePiece_second.append_child(pugi::node_pcdata)
                            .set_value(std::to_string(gk_ti.files_vec.at(i).map_file_piece.second).data());

                    pugi::xml_node nodeFilesDownBool = nodeFilesPath.append_child(XML_CHILD_FILES_DOWNBOOL_TORRENT);
                    nodeFilesDownBool.append_child(pugi::node_pcdata)
                            .set_value(std::to_string(gk_ti.files_vec.at(i).downloaded).data());
                }

                pugi::xml_node nodeTrackers = nodeChild.append_child(XML_CHILD_NODE_TORRENT_TRACKERS);
                for (size_t i = 0; i < gk_ti.trackers.size(); ++i) {
                    pugi::xml_node nodeTrackersTier = nodeTrackers.append_child(XML_CHILD_TRACKERS_TIER_TORRENT);
                    nodeTrackersTier.append_child(pugi::node_pcdata).set_value(std::to_string(gk_ti.trackers.at(i).tier).data());

                    // Sub-node
                    pugi::xml_node nodeTrackersUrl = nodeTrackersTier.append_child(XML_CHILD_TRACKERS_URL_TORRENT);
                    nodeTrackersUrl.append_child(pugi::node_pcdata).set_value(gk_ti.trackers.at(i).url.c_str());

                    pugi::xml_node nodeTrackersAvailable = nodeTrackersTier.append_child(XML_CHILD_TRACKERS_AVAILABLE_TORRENT);
                    nodeTrackersAvailable.append_child(pugi::node_pcdata).set_value(std::to_string(gk_ti.trackers.at(i).enabled).data());
                }

                bool saveSucceed = doc.save_file(xmlCfgFile_loc.string().c_str(), PUGIXML_TEXT("    "));
                if (!saveSucceed) {
                    throw std::runtime_error(tr("Error with saving XML config file!").toStdString());
                }

                return true;
            }
        } else {
            throw std::invalid_argument(tr("Internal error: no path has been given for the XML "
                                                   "configuration file!").toStdString());
        }
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
std::vector<GekkoFyre::GkTorrent::TorrentInfo> GekkoFyre::CmnRoutines::readTorrentInfo(const bool &minimal_readout,
                                                                                       const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (fs::exists(xmlCfgFile_loc, ec) && fs::is_regular_file(xmlCfgFile_loc)) {
        std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_torrent_info_list;

        pugi::xml_document doc;
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
                GekkoFyre::GkTorrent::TorrentInfo i;
                i.cId = item.attribute(XML_ITEM_ATTR_TORRENT_CID).as_uint();
                i.down_dest = item.attribute(XML_ITEM_ATTR_TORRENT_FLOC).as_string();
                i.insert_timestamp = item.attribute(XML_ITEM_ATTR_TORRENT_INSERT_DATE).as_llong();
                i.complt_timestamp = item.attribute(XML_ITEM_ATTR_TORRENT_COMPLT_DATE).as_llong();
                i.creatn_timestamp = item.attribute(XML_ITEM_ATTR_TORRENT_CREATN_DATE).as_llong();
                i.dlStatus = convDlStat_IntToEnum(item.attribute(XML_ITEM_ATTR_TORRENT_DLSTATUS).as_int());
                i.comment = item.attribute(XML_ITEM_ATTR_TORRENT_TORRNT_COMMENT).as_string();
                i.creator = item.attribute(XML_ITEM_ATTR_TORRENT_TORRNT_CREATOR).as_string();
                i.magnet_uri = item.attribute(XML_ITEM_ATTR_TORRENT_MAGNET_URI).as_string();
                i.torrent_name = item.attribute(XML_ITEM_ATTR_TORRENT_TORRNT_NAME).as_string();
                i.num_files = item.attribute(XML_ITEM_ATTR_TORRENT_NUM_FILES).as_int();
                i.num_pieces = item.attribute(XML_ITEM_ATTR_TORRENT_TORRNT_PIECES).as_int();
                i.piece_length = item.attribute(XML_ITEM_ATTR_TORRENT_TORRNT_PIECE_LENGTH).as_int();
                i.unique_id = item.attribute(XML_ITEM_ATTR_TORRENT_UNIQUE_ID).as_string();

                if (!minimal_readout) {
                    std::vector<GekkoFyre::GkTorrent::TorrentFile> files_vec;
                    std::string tmp_str = "";
                    int tmp_int = 0;

                    /**
                     * #########
                     * # Nodes #
                     * #########
                     */
                    // http://pugixml.org/docs/manual.html#access.nodedata
                    std::vector<std::pair<std::string, int>> nodes;
                    pugi::xml_node nodes_node = item.child(XML_CHILD_NODE_TORRENT_NODES);
                    for (pugi::xml_node node_node = nodes_node.child(XML_CHILD_NODES_NAMES_TORRENT); node_node;
                         node_node = node_node.next_sibling(XML_CHILD_NODES_NAMES_TORRENT)) {
                        pugi::xml_node number_node = node_node.child(XML_CHILD_NODES_NUMBR_TORRENT);

                        tmp_str = node_node.child_value();
                        tmp_int = atoi(number_node.child_value());
                        nodes.push_back(std::make_pair(tmp_str, tmp_int));
                    }

                    /**
                     * #########
                     * # Files #
                     * #########
                     */
                    pugi::xml_node files_node = item.child(XML_CHILD_NODE_TORRENT_FILES);
                    for (pugi::xml_node file_node = files_node.child(XML_CHILD_FILES_PATH_TORRENT); file_node;
                         file_node = file_node.next_sibling(XML_CHILD_FILES_PATH_TORRENT)) {

                        GekkoFyre::GkTorrent::TorrentFile tf;
                        tf.file_path = file_node.child_value();
                        tf.sha1_hash_hex = file_node.child_value(XML_CHILD_FILES_HASH_TORRENT);
                        tf.mtime = strtoul(file_node.child_value(XML_CHILD_FILES_MTIME_TORRENT), nullptr, 10);
                        tf.file_offset = atol(file_node.child_value(XML_CHILD_FILES_FILEOFFST_TORRENT));
                        tf.content_length = atol(file_node.child_value(XML_CHILD_FILES_CONTLNGTH_TORRENT));
                        tf.flags = atoi(file_node.child_value(XML_CHILD_FILES_FLAGS_TORRENT));
                        tf.downloaded = atoi(file_node.child_value(XML_CHILD_FILES_DOWNBOOL_TORRENT));

                        pugi::xml_node file_map_child_node = file_node.child(XML_CHILD_NODE_TORRENT_FILES_MAPFLEPCE);
                        int map_int_one, map_int_two = { 0 };
                        map_int_one = atoi(file_map_child_node.child_value(XML_CHILD_FILES_MAPFLEPCE_1_TORRENT));
                        map_int_two = atoi(file_map_child_node.child_value(XML_CHILD_FILES_MAPFLEPCE_2_TORRENT));
                        tf.map_file_piece.first = map_int_one;
                        tf.map_file_piece.second = map_int_two;

                        files_vec.push_back(tf);
                    }

                    /**
                     * ############
                     * # Trackers #
                     * ############
                     */
                    std::vector<GekkoFyre::GkTorrent::TorrentTrackers> trackers;
                    pugi::xml_node trackers_node = item.child(XML_CHILD_NODE_TORRENT_TRACKERS);
                    for (pugi::xml_node tracker_node = trackers_node.child(XML_CHILD_TRACKERS_TIER_TORRENT); tracker_node;
                         tracker_node = tracker_node.next_sibling(XML_CHILD_TRACKERS_TIER_TORRENT)) {
                        pugi::xml_node url_node = tracker_node.child(XML_CHILD_TRACKERS_URL_TORRENT);
                        pugi::xml_node avail_node = tracker_node.child(XML_CHILD_TRACKERS_AVAILABLE_TORRENT);
                        GekkoFyre::GkTorrent::TorrentTrackers tracker;
                        tracker.tier = atoi(tracker_node.child_value());
                        tracker.url = url_node.child_value();
                        tracker.enabled = atoi(avail_node.child_value());
                        trackers.push_back(tracker);
                    }

                    i.nodes = nodes;
                    i.files_vec = files_vec;
                    i.trackers = trackers;
                    gk_torrent_info_list.push_back(i);
                } else {
                    gk_torrent_info_list.push_back(i);
                }
            }
        }

        return gk_torrent_info_list;
    }

    return std::vector<GekkoFyre::GkTorrent::TorrentInfo>();
}

bool GekkoFyre::CmnRoutines::delTorrentItem(const std::string &unique_id, const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (fs::exists(xmlCfgFile_loc, ec) && fs::is_regular_file(xmlCfgFile_loc)) {
        std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_torrent_info;
        gk_torrent_info = readTorrentInfo(false, xmlCfgFile);

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

bool GekkoFyre::CmnRoutines::modifyTorrentItem(const GekkoFyre::GkTorrent::ModifyTorrentInfo &gk_mt,
                                               const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (fs::exists(xmlCfgFile_loc, ec) && fs::is_regular_file(xmlCfgFile_loc)) {
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

        /*
        pugi::xml_node items = doc.child(XML_PARENT_NODE);
        for (const auto& file: items.children(XML_CHILD_NODE_TORRENT)) {
            for (const auto &item: file.children(XML_CHILD_ITEM_TORRENT)) {
                if (file.find_child_by_attribute(XML_ITEM_ATTR_TORRENT_UNIQUE_ID, gk_mt.unique_id.c_str())) {

                }
            }
        }
         */
    }

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
 * @brief GekkoFyre::CmnRoutines::readXmlSettings reads an already pre-existing, XML configuration file for FyreDL.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-06
 * @param xmlCfgFile The XML configuration file in question.
 * @return The data read from the XML configuration file.
 */
GekkoFyre::GkSettings::FyreDL GekkoFyre::CmnRoutines::readXmlSettings(const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    GekkoFyre::GkSettings::FyreDL settings;
    settings.main_win_x = 0;
    settings.main_win_y = 0;
    if (fs::exists(xmlCfgFile_loc, ec) && fs::is_regular_file(xmlCfgFile_loc)) {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(xmlCfgFile_loc.string().c_str(), pugi::parse_default|pugi::parse_declaration);
        if (!result) {
            throw std::invalid_argument(tr("XML parse error: %1, character pos= %2")
                                                .arg(result.description(),
                                                     QString::number(result.offset)).toStdString());
        }

        pugi::xml_node items = doc.child(XML_PARENT_NODE);
        const auto xml_settings_node = items.child(XML_CHILD_NODE_SETTINGS);
        const auto xml_user_item = xml_settings_node.child(XML_CHILD_ITEM_SETTINGS);
        settings.main_win_x = xml_user_item.attribute(XML_ITEM_ATTR_SETTINGS_WIN_X).as_int();
        settings.main_win_y = xml_user_item.attribute(XML_ITEM_ATTR_SETTINGS_WIN_Y).as_int();
    }

    return settings;
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
