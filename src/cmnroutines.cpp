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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <exception>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <QUrl>

#ifdef _WIN32
#define NTDDI_VERSION NTDDI_VISTASP1
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#include <SDKDDKVer.h>
#include <Windows.h>

#elif __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

extern "C" {
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
}

#else
#error "Platform not supported!"
#endif

namespace pt = boost::property_tree;
namespace sys = boost::system;
namespace fs = boost::filesystem;

GekkoFyre::CmnRoutines::CmnRoutines()
{
    setlocale (LC_ALL, "");
}

GekkoFyre::CmnRoutines::~CmnRoutines()
{}

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
double GekkoFyre::CmnRoutines::bytesToKilobytes(const double &content_length)
{
    return std::round((content_length / 1024));
}

/**
 * @brief GekkoFyre::CmnRoutines::percentDownloaded
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-09
 * @param content_length
 * @param amountDl
 * @return
 */
double GekkoFyre::CmnRoutines::percentDownloaded(const double &content_length, const double &amountDl)
{
    double percent = ((amountDl / content_length) * 100);
    if (percent >= 101) {
        throw std::runtime_error(tr("Incorrect download percentage reported!").toStdString());
    }

    return percent;
}

std::string GekkoFyre::CmnRoutines::findCfgFile(const std::string &cfgFileName)
{
    fs::path home_dir;
    #ifdef _WIN32
    #elif __linux__
    struct passwd *pw = getpwuid(getuid()); // NOTE: If you need this in a threaded application, you'll want to use getpwuid_r instead
    const char *home_dir_char = pw->pw_dir;
    home_dir = home_dir_char;
    #endif

    // http://theboostcpplibraries.com/boost.filesystem-paths
    std::ostringstream oss;
    if (fs::is_directory(home_dir)) {
        oss << home_dir.string() << fs::path::preferred_separator << cfgFileName;
    }

    return oss.str();
}

/**
 * @brief GekkoFyre::CmnRoutines::readDownloadInfo
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10
 * @note   <https://akrzemi1.wordpress.com/2011/07/13/parsing-xml-with-boost/>56
 * @param xmlCfgFile
 * @return
 */
std::vector<GekkoFyre::CmnRoutines::CurlDlInfo> GekkoFyre::CmnRoutines::readDownloadInfo(
        const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    if (fs::exists(xmlCfgFile_loc, ec) && fs::is_regular_file(xmlCfgFile_loc)) {
        std::vector<GekkoFyre::CmnRoutines::CurlDlInfo> dl_info_list;
        pt::ptree pt;
        pt::read_xml(xmlCfgFile_loc.string(), pt);

        // Traverse ptree
        for (auto &child : pt.get_child("download-history-db")) {
            if (child.first == "file") {
                GekkoFyre::CmnRoutines::CurlDlInfo i;
                i.cId = child.second.get<unsigned int>("content-id");
                i.file_loc = child.second.get<std::string>("file-loc");
                i.dlStatus = convDlStat_toEnum(child.second.get<short>("status"));
                i.timestamp = child.second.get<uint>("insert-date");
                i.ext_info.status_ok = child.second.get<bool>("status-ok");
                i.ext_info.status_msg = child.second.get<std::string>("status-msg");
                i.ext_info.effective_url = child.second.get<std::string>("effec-url");
                i.ext_info.response_code = child.second.get<long>("resp-code");
                i.ext_info.content_length = child.second.get<double>("content-length");
                dl_info_list.push_back(i);
            }
        }

        return dl_info_list;
    }

    return std::vector<GekkoFyre::CmnRoutines::CurlDlInfo>();
}

/**
 * @brief GekkoFyre::CmnRoutines::writeDownloadInfo
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10
 * @note   <http://stackoverflow.com/questions/6656380/boost-1-46-1-property-tree-how-to-iterate-through-ptree-receiving-sub-ptrees>
 * @param dl_info
 * @param xmlCfgFile
 * @return
 */
bool GekkoFyre::CmnRoutines::writeDownloadInfo(GekkoFyre::CmnRoutines::CurlDlInfo dl_info,
                                               const std::string &xmlCfgFile)
{
    fs::path xmlCfgFile_loc = findCfgFile(xmlCfgFile);
    sys::error_code ec;
    std::vector<GekkoFyre::CmnRoutines::CurlDlInfo> dl_info_list;
    if (dl_info.ext_info.status_ok == true) {
        if (!xmlCfgFile_loc.string().empty()) {
            switch (dl_info.dlStatus) {
            case GekkoFyre::DownloadStatus::Unknown:
            {
                dl_info_list = readDownloadInfo(xmlCfgFile_loc.string());
                unsigned int cId = 0;

                // Finds the largest 'dl_info_list.cId' value and then increments by +1, ready for
                // assignment to a new download entry.
                for (size_t i = 0; i < dl_info_list.size(); ++i) {
                    unsigned int tmp = 0;
                    tmp = dl_info_list.at(i).cId;
                    if (tmp > cId) {
                        cId = tmp;
                    }
                }

                dl_info.cId = cId;
                dl_info.dlStatus = GekkoFyre::DownloadStatus::Unknown;
                QDateTime now = QDateTime::currentDateTime();
                dl_info.timestamp = now.toTime_t();
                dl_info_list.push_back(dl_info);

                for (size_t i = 0; i < dl_info_list.size(); ++i) {
                    dl_info_list.at(i).ext_info.status_msg = "";
                }

                pt::ptree pt;
                pt.add("download-history-db.version", 3);
                for (const auto &d: dl_info_list) {
                    pt::ptree & node = pt.add("download-history-db.file", "");
                    node.put("content-id", d.cId);
                    node.put("file-loc", d.file_loc);
                    node.put("status", convDlStat_toInt(d.dlStatus));
                    node.put("insert-date", d.timestamp);
                    node.put("status-ok", d.ext_info.status_ok);
                    node.put("status-msg", d.ext_info.status_msg);
                    node.put("effec-url", d.ext_info.effective_url);
                    node.put("resp-code", d.ext_info.response_code);
                    node.put("content-length", d.ext_info.content_length);
                }
                pt::write_xml(xmlCfgFile_loc.string(), pt, std::locale());
                return true;
            }
            default:
                throw std::runtime_error(tr("You should not be seeing this!").toStdString());
            }
        } else {
            throw std::invalid_argument(tr("Internal error: no path has been given for the XML "
                                           "configuration file!").toStdString());
        }
    }

    return false;
}

short GekkoFyre::CmnRoutines::convDlStat_toInt(const GekkoFyre::DownloadStatus &status)
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
    default:
        return -1;
    }
}

GekkoFyre::DownloadStatus GekkoFyre::CmnRoutines::convDlStat_toEnum(const short &s)
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
    default:
        return tr("Unknown");
    }
}

/**
 * @brief GekkoFyre::CmnRoutines::fileStream
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-10
 * @note   <http://stackoverflow.com/questions/195323/what-is-the-most-elegant-way-to-read-a-text-file-with-c>
 *         <https://curl.haxx.se/libcurl/c/url2file.html>
 *         <https://www.hackthissite.org/articles/read/1078>
 * @param file_loc
 * @return
 */
bool GekkoFyre::CmnRoutines::fileStream(const QString &url, const QString &file_loc)
{
    CurlInit curl_struct = curlInit(url, "", "", false, false, file_loc);

    if (curl_struct.curl_ptr != nullptr) {
        curl_struct.curl_res = curl_easy_perform(curl_struct.curl_ptr);

        if (curl_struct.curl_res != CURLE_OK) {

        } else {
            if (file_loc.isEmpty()) {
                return false;
            } else {
                if (curl_struct.file_buf.stream) {
                    fclose(curl_struct.file_buf.stream);
                    curlCleanup(curl_struct);
                    return true;
                }
            }
        }
    }

    curlCleanup(curl_struct);
    return false;
}

GekkoFyre::CmnRoutines::CurlInfo GekkoFyre::CmnRoutines::verifyFileExists(const QString &url)
{
    // curl_easy_perform(), to initiate the request and fire any callbacks

    CurlInfo info;
    CurlInit curl_struct = curlInit(url, "", "", true, true);
    if (curl_struct.curl_ptr != nullptr) {
        curl_struct.curl_res = curl_easy_perform(curl_struct.curl_ptr);
        if (curl_struct.curl_res != CURLE_OK) {
            info.response_code = curl_struct.curl_res;
            info.effective_url = curl_struct.errbuf;
            return info;
        } else {
            // https://curl.haxx.se/libcurl/c/curl_easy_getinfo.html
            // http://stackoverflow.com/questions/14947821/how-do-i-use-strdup
            long rescode;
            char *effec_url;
            curl_easy_getinfo(curl_struct.curl_ptr, CURLINFO_RESPONSE_CODE, &rescode);
            curl_easy_getinfo(curl_struct.curl_ptr, CURLINFO_EFFECTIVE_URL, &effec_url);

            std::memcpy(&info.response_code, &rescode, sizeof(unsigned long)); // Must be trivially copyable otherwise UB!
            info.effective_url = effec_url;
        }
    }

    curlCleanup(curl_struct);
    return info;
}

/**
 * @brief GekkoFyre::CmnRoutines::curlInit
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-09
 * @note   <http://www.linuxdevcenter.com/pub/a/linux/2005/05/05/libcurl.html>
 *         <https://gist.github.com/whoshuu/2dc858b8730079602044>
 *         <http://stackoverflow.com/questions/16277894/curl-libhow-can-i-get-response-redirect-url>
 *         <http://stackoverflow.com/questions/7677285/how-to-get-the-length-of-a-file-without-downloading-the-file-in-a-curl-binary-ge>
 * @param url
 * @param username
 * @param password
 * @return
 */
GekkoFyre::CmnRoutines::CurlInit GekkoFyre::CmnRoutines::curlInit(const QString &url,
                                                                  const std::string &username,
                                                                  const std::string &password,
                                                                  bool grabHeaderOnly, bool writeToMemory,
                                                                  const QString &fileLoc)
{
    // curl_global_init(), to initialize the curl library (once per program)
    // curl_easy_init(), to create a context
    // curl_easy_setopt(), to configure that context

    CurlInit curl_struct;
    curl_global_init(CURL_GLOBAL_ALL);
    curl_struct.curl_ptr = curl_easy_init(); // Initiate the curl session

    if (curl_struct.curl_ptr) {
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_URL, url.toStdString().c_str());

        if (grabHeaderOnly) {
            curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_HEADER, 1);
            curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_NOBODY, 1);
        }

        // Provide a buffer to store errors in (as a string)
        // https://curl.haxx.se/libcurl/c/curl_easy_strerror.html
        // https://curl.haxx.se/libcurl/c/CURLOPT_ERRORBUFFER.html
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_ERRORBUFFER, curl_struct.errbuf);

        // Set the error buffer as empty before performing a request
        curl_struct.errbuf[0] = 0;

        // A long parameter set to 1 tells the library to follow any Location: header that the server
        // sends as part of a HTTP header in a 3xx response. The Location: header can specify a relative
        // or an absolute URL to follow.
        // https://curl.haxx.se/libcurl/c/CURLOPT_FOLLOWLOCATION.html
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_NOPROGRESS, 1L); // Tells the library to shut off the progress meter completely for requests done with this handle

        // Some servers don't like requests that are made without a user-agent field, so we provide one
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_USERAGENT, "FyreDL/0.0.1");

        // The maximum redirection limit goes here
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_MAXREDIRS, 12L);

        // Enable TCP keep-alive for this transfer
        // https://curl.haxx.se/libcurl/c/CURLOPT_TCP_KEEPALIVE.html
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_TCP_KEEPIDLE, 120L); // Keep-alive idle time to 120 seconds
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_TCP_KEEPINTVL, 60L); // Interval time between keep-alive probes is 60 seconds

        if (!username.empty() || !password.empty()) {
            std::ostringstream oss;
            oss << username << ":" << password;
            curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_UNRESTRICTED_AUTH, 1L);
            curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_USERPWD, oss.str().c_str()); // user:pass
        }

        if (writeToMemory) {
            curl_struct.mem_chunk.memory = (char *)malloc(1); // Will be grown as needed by the realloc above
            curl_struct.mem_chunk.size = 0; // No data at this point
            curl_struct.file_buf.file_loc = nullptr;
            curl_struct.file_buf.stream = nullptr;

            // Send all data to this function, via the computer's RAM
            curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_WRITEFUNCTION, curl_write_memory_callback);

            // We pass our 'chunk' struct to the callback function
            curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_WRITEDATA, (void *)&curl_struct.mem_chunk);
        } else {
            curl_struct.file_buf.file_loc = fileLoc.toStdString().c_str();
            curl_struct.file_buf.stream = nullptr;
            curl_struct.mem_chunk.memory = nullptr;
            curl_struct.mem_chunk.size = 0;

            // Send all data to this function, via file streaming
            curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_WRITEFUNCTION, curl_write_file_callback);

            // We pass our 'chunk' struct to the callback function
            curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_WRITEDATA, &curl_struct.file_buf);
        }

        return curl_struct;
    } else {
        throw std::runtime_error(tr("Unable to allocate memory for curl!").toStdString());
    }

    curl_struct.curl_ptr = nullptr;
    return curl_struct;
}

/**
 * @brief GekkoFyre::CmnRoutines::curlGrabInfo
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-09
 * @param url
 * @return
 */
GekkoFyre::CmnRoutines::CurlInfoExt GekkoFyre::CmnRoutines::curlGrabInfo(const QString &url)
{
    // curl_easy_perform(), to initiate the request and fire any callbacks

    CurlInfoExt info;
    CurlInit curl_struct = curlInit(url, "", "", true, true);
    if (curl_struct.curl_ptr != nullptr) {
        curl_struct.curl_res = curl_easy_perform(curl_struct.curl_ptr);
        if (curl_struct.curl_res != CURLE_OK) {
            info.status_ok = false;
            info.status_msg = curl_struct.errbuf;
        } else {
            // https://curl.haxx.se/libcurl/c/curl_easy_getinfo.html
            long rescode;
            double elapsed, content_length;
            char *effec_url;
            curl_easy_getinfo(curl_struct.curl_ptr, CURLINFO_RESPONSE_CODE, &rescode);
            curl_easy_getinfo(curl_struct.curl_ptr, CURLINFO_TOTAL_TIME, &elapsed);
            curl_easy_getinfo(curl_struct.curl_ptr, CURLINFO_EFFECTIVE_URL, &effec_url);
            curl_easy_getinfo(curl_struct.curl_ptr, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);

            // Must be trivially copyable otherwise UB!
            std::memcpy(&info.response_code, &rescode, sizeof(unsigned long));
            std::memcpy(&info.elapsed, &elapsed, sizeof(double));
            std::memcpy(&info.content_length, &content_length, sizeof(double));
            info.effective_url = effec_url;
            info.status_msg = curl_struct.errbuf;
            info.status_ok = true;
        }
    }

    curlCleanup(curl_struct);
    return info;
}

/**
 * @brief GekkoFyre::CmnRoutines::curlGetProgress
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-09
 * @note   <https://curl.haxx.se/libcurl/c/progressfunc.html>
 * @param curl_pointer
 */
void GekkoFyre::CmnRoutines::curlGetProgress(CURL *curl_pointer)
{}

/**
 * @brief GekkoFyre::CmnRoutines::curlCleanup
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date   2016-09
 * @param curl_pointer
 */
void GekkoFyre::CmnRoutines::curlCleanup(CurlInit curl_init)
{
    // curl_easy_cleanup(), to clean up the context
    // curl_global_cleanup(), to tear down the curl library (once per program)

    curl_easy_cleanup(curl_init.curl_ptr); // Always cleanup
    curl_global_cleanup(); // We're done with libcurl, globally, so clean it up!

    if (curl_init.mem_chunk.memory != nullptr) {
        free(curl_init.mem_chunk.memory);
    }

    if (curl_init.file_buf.stream != nullptr) {
        fclose(curl_init.file_buf.stream);
    }

    curl_init.curl_ptr = nullptr;
    return;
}

/**
 * @brief GekkoFyre::CmnRoutines::curl_write_memory_callback can be used to download data into a chunk of
 * memory instead of storing it in a local file.
 * @author Daniel Stenberg <daniel@haxx.se>, et al.
 * @date   2015
 * @note   <https://curl.haxx.se/libcurl/c/getinmemory.html>
 * @param ptr
 * @param size
 * @param nmemb
 * @param userp
 * @return
 */
size_t GekkoFyre::CmnRoutines::curl_write_memory_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = (size * nmemb);
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        // Out of memory!
        throw std::runtime_error(tr("Not enough memory (realloc returned NULL)!").toStdString());
    }

    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

/**
 * @brief GekkoFyre::CmnRoutines::curl_write_file_callback can be used to download data into a local file
 * on the user's storage.
 * @author Daniel Stenberg <daniel@haxx.se>, et al.
 * @date   2015
 * @note   <https://curl.haxx.se/libcurl/c/ftpget.html>
 *         <https://curl.haxx.se/libcurl/c/url2file.html>
 * @param ptr
 * @param size
 * @param nmemb
 * @param stream
 * @return
 */
size_t GekkoFyre::CmnRoutines::curl_write_file_callback(void *buffer, size_t size, size_t nmemb,
                                                        void *stream)
{
    struct FileStream *out = (struct FileStream *)stream;
    if(out && !out->stream) {
        // Open file for writing
        out->stream = fopen(out->file_loc, "wb");
        if(!out->stream) {
            return 0; // Failure, can't open file to write!
        }
    }

    return fwrite(buffer, size, nmemb, out->stream);
}
