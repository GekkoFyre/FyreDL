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
#include <QUrl>

GekkoFyre::CmnRoutines::CmnRoutines()
{}

GekkoFyre::CmnRoutines::~CmnRoutines()
{}

/**
 * @brief GekkoFyre::CmnRoutines::extractFilename
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @note   <http://stackoverflow.com/questions/39647342/extracting-the-filename-off-a-url-with-qregexp>
 * @param url
 * @return
 */
QString GekkoFyre::CmnRoutines::extractFilename(const QString &url)
{
    QUrl url_parse = url;
    return url_parse.fileName();
}

GekkoFyre::CmnRoutines::CurlInfo GekkoFyre::CmnRoutines::verifyFileExists(const QString &url)
{
    // curl_easy_perform(), to initiate the request and fire any callbacks

    CurlInfo info;
    CurlInit curl_struct = curlInit(url, "", "");
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

        curlCleanup(curl_struct);
    }

    return info;
}

/**
 * @brief GekkoFyre::CmnRoutines::curlInit
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @note   <http://www.linuxdevcenter.com/pub/a/linux/2005/05/05/libcurl.html>
 *         <https://gist.github.com/whoshuu/2dc858b8730079602044>
 *         <http://stackoverflow.com/questions/16277894/curl-libhow-can-i-get-response-redirect-url>
 * @param url
 * @param username
 * @param password
 * @return
 */
GekkoFyre::CmnRoutines::CurlInit GekkoFyre::CmnRoutines::curlInit(const QString &url,
                                                                  const std::string &username,
                                                                  const std::string &password)
{
    // curl_global_init(), to initialize the curl library (once per program)
    // curl_easy_init(), to create a context
    // curl_easy_setopt(), to configure that context

    CurlInit curl_struct;
    curl_struct.mem_chunk.memory = (char *)malloc(1); // Will be grown as needed by the realloc above
    curl_struct.mem_chunk.size = 0; // No data at this point
    curl_global_init(CURL_GLOBAL_ALL);
    curl_struct.curl_ptr = curl_easy_init(); // Initiate the curl session

    if (curl_struct.curl_ptr) {
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_URL, url.toStdString().c_str());

        // Provide a buffer to store errors in (as a string)
        // https://curl.haxx.se/libcurl/c/curl_easy_strerror.html
        // https://curl.haxx.se/libcurl/c/CURLOPT_ERRORBUFFER.html
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_ERRORBUFFER, curl_struct.errbuf);

        // Set the error buffer as empty before performing a request
        curl_struct.errbuf[0] = 0;

        // Send all data to this function
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_WRITEFUNCTION, curl_write_memory_callback);

        // We pass our 'chunk' struct to the callback function
        curl_easy_setopt(curl_struct.curl_ptr, CURLOPT_WRITEDATA, (void *)&curl_struct.mem_chunk);

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
 * @param url
 * @return
 */
GekkoFyre::CmnRoutines::CurlInfoExt GekkoFyre::CmnRoutines::curlGrabInfo(const QString &url)
{
    // curl_easy_perform(), to initiate the request and fire any callbacks

    CurlInfoExt info;
    CurlInit curl_struct = curlInit(url, "", "");
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

        curlCleanup(curl_struct);
    }

    return info;
}

/**
 * @brief GekkoFyre::CmnRoutines::curlGetProgress
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @note   <https://curl.haxx.se/libcurl/c/progressfunc.html>
 * @param curl_pointer
 */
void GekkoFyre::CmnRoutines::curlGetProgress(CURL *curl_pointer)
{}

/**
 * @brief GekkoFyre::CmnRoutines::curlCleanup
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param curl_pointer
 */
void GekkoFyre::CmnRoutines::curlCleanup(CurlInit curl_init)
{
    // curl_easy_cleanup(), to clean up the context
    // curl_global_cleanup(), to tear down the curl library (once per program)

    curl_easy_cleanup(curl_init.curl_ptr); // Always cleanup
    curl_global_cleanup(); // We're done with libcurl, globally, so clean it up!
    if (curl_init.mem_chunk.memory != NULL) {
        free(curl_init.mem_chunk.memory);
    }

    // curl_init.curl_ptr = NULL;
    return;
}

/**
 * @brief GekkoFyre::CmnRoutines::curl_write_memory_callback
 * @author Daniel Stenberg <daniel@haxx.se>, et al.
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
