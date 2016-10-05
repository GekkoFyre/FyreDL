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
#include <exception>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <QUrl>

GekkoFyre::CmnRoutines::CmnRoutines()
{}

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
            std::strcpy(info.effective_url, curl_struct.errbuf);
            return info;
        } else {
            // https://curl.haxx.se/libcurl/c/curl_easy_getinfo.html
            // http://stackoverflow.com/questions/14947821/how-do-i-use-strdup
            long rescode;
            char *effec_url;
            curl_easy_getinfo(curl_struct.curl_ptr, CURLINFO_RESPONSE_CODE, &rescode);
            curl_easy_getinfo(curl_struct.curl_ptr, CURLINFO_EFFECTIVE_URL, &effec_url);

            std::memcpy(&info.response_code, &rescode, sizeof(unsigned long)); // Must be trivially copyable otherwise UB!
            info.effective_url = new char[(strlen(effec_url) + 1)];
            std::strcpy(info.effective_url, effec_url);
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
            std::strcpy(info.status_msg, curl_struct.errbuf);
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
            info.effective_url = new char[(strlen(effec_url) + 1)];
            info.status_msg = new char[(strlen(curl_struct.errbuf) + 1)];
            std::memcpy(&info.response_code, &rescode, sizeof(unsigned long));
            std::memcpy(&info.elapsed, &elapsed, sizeof(double));
            std::memcpy(&info.content_length, &content_length, sizeof(double));
            std::strcpy(info.effective_url, effec_url);
            std::strcpy(info.status_msg, curl_struct.errbuf);
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
