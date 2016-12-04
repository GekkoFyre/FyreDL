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
#include <cmath>
#include <iostream>
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
    if (!given_hash_val.isEmpty()) {
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
            if ((hash_type == GekkoFyre::HashType::MD5 || hash_type == GekkoFyre::HashType::SHA1 ||
                hash_type == GekkoFyre::HashType::SHA256||  hash_type == GekkoFyre::HashType::SHA512 ||
                hash_type == GekkoFyre::HashType::SHA3_256 ||  hash_type == GekkoFyre::HashType::SHA3_512) &&
                    hash_type != GekkoFyre::HashType::None) {
                QCryptographicHash hash(convHashType_toAlgo(hash_type));
                if (hash.addData(&f)) {
                    result = hash.result();
                    info.checksum = result.toHex();
                    if (info.checksum == given_hash_val) {
                        info.hash_verif = GekkoFyre::HashVerif::Verified;
                        return info;
                    } else {
                        info.hash_verif = GekkoFyre::HashVerif::Corrupt;
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
                    if (vec_hash_val.at(i) == given_hash_val) {
                        info.hash_verif = GekkoFyre::HashVerif::Verified;
                        return info;
                    }
                }

                info.hash_verif = GekkoFyre::HashVerif::NotApplicable;
                info.hash_type = GekkoFyre::HashType::CannotDetermine;
                info.checksum = "";
                return info;
            }
        }
    }

    info.hash_verif = GekkoFyre::HashVerif::NotApplicable;
    info.hash_type = GekkoFyre::HashType::None;
    info.checksum = "";
    return info;
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
 * @brief GekkoFyre::CmnRoutines::readDownloadInfo
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10
 * @note   <http://www.gerald-fahrnholz.eu/sw/DocGenerated/HowToUse/html/group___grp_pugi_xml.html>
 *         <http://stackoverflow.com/questions/16155888/proper-way-to-parse-xml-using-pugixml>
 * @param xmlCfgFile
 * @return
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
 * @param xmlCfgFile
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
 * @param xmlCfgFile
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
                                           const long long &complt_timestamp,
                                           const std::string &xmlCfgFile)
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
                if (file.find_child_by_attribute(XML_ITEM_ATTR_FILE_FLOC, file_loc.c_str())) {
                    item.attribute(XML_ITEM_ATTR_FILE_STAT).set_value(convDlStat_toInt(status));
                    if (!hash_checksum.empty()) {
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
            return 1;
        case GekkoFyre::HashVerif::NotApplicable:
            return 0;
        case GekkoFyre::HashVerif::Verified:
            return 2;
        case GekkoFyre::HashVerif::Corrupt:
            return -1;
        default:
            return 0;
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
        case 1:
            return GekkoFyre::HashVerif::Analyzing;
        case 0:
            return GekkoFyre::HashVerif::NotApplicable;
        case 2:
            return GekkoFyre::HashVerif::Verified;
        case -1:
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
