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
 * @file curl_multi.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11-01
 * @brief This is the easy-interface to libcurl. It's good for verifying, checking, etc. and where a full
 * download of the file is not required, but just the header.
 */

#include "curl_easy.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>

GekkoFyre::CurlEasy::CurlEasy()
{}

GekkoFyre::CurlEasy::~CurlEasy()
{
    // curl_global_cleanup(); // We're done with libcurl, globally, so clean it up!
}

GekkoFyre::GkCurl::CurlInit GekkoFyre::CurlEasy::new_easy_handle(const QString &url)
{
    GekkoFyre::GkCurl::CurlInit *ci;
    ci = new GekkoFyre::GkCurl::CurlInit;
    ci->conn_info = (GekkoFyre::GkCurl::ConnInfo *) calloc(1, sizeof(GekkoFyre::GkCurl::ConnInfo));

    curl_global_init(CURL_GLOBAL_DEFAULT);
    ci->conn_info->easy = curl_easy_init(); // Initiate the curl session

    if (!ci->conn_info->easy) {
        throw std::runtime_error(tr("'curl_easy_init()' failed, exiting!").toStdString());
    }

    ci->conn_info->url = url.toStdString();
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_URL, ci->conn_info->url.c_str());

    // Grab only the header
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_HEADER, 1);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_NOBODY, 1);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_NOPROGRESS, 1L);

    ci->mem_chunk.size = 0; // No data at this point

    // Send all data to this function, via the computer's RAM
    // NOTE: On Windows, 'CURLOPT_WRITEFUNCTION' /must/ be set, otherwise a crash will occur!
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_WRITEFUNCTION, &curl_write_memory_callback);

    // We pass our 'chunk' struct to the callback function
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_WRITEDATA, &ci->mem_chunk);

    // A long parameter set to 1 tells the library to follow any Location: header that the server
    // sends as part of a HTTP header in a 3xx response. The Location: header can specify a relative
    // or an absolute URL to follow.
    // https://curl.haxx.se/libcurl/c/CURLOPT_FOLLOWLOCATION.html
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_FOLLOWLOCATION, 1L);

    // Some servers don't like requests that are made without a user-agent field, so we provide one
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_USERAGENT, "FyreDL/0.0.1");

    // The maximum redirection limit goes here
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_MAXREDIRS, 12L);

    // Enable TCP keep-alive for this transfer
    // https://curl.haxx.se/libcurl/c/CURLOPT_TCP_KEEPALIVE.html
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_TCP_KEEPIDLE, 120L); // Keep-alive idle time to 120 seconds
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_TCP_KEEPINTVL, 60L); // Interval time between keep-alive probes is 60 seconds

    curl_easy_setopt(ci->conn_info->easy, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_ERRORBUFFER, ci->conn_info->error);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_PRIVATE, ci->conn_info);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_LOW_SPEED_TIME, 3L);
    curl_easy_setopt(ci->conn_info->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);

    return *ci;
}

GekkoFyre::GkCurl::CurlInfo GekkoFyre::CurlEasy::verifyFileExists(const QString &url)
{
    // curl_easy_perform(), to initiate the request and fire any callbacks

    CURLcode curl_res;
    GekkoFyre::GkCurl::CurlInfo info;
    GekkoFyre::GkCurl::CurlInit curl_struct = new_easy_handle(url);
    if (curl_struct.conn_info->easy != nullptr) {
        curl_res = curl_easy_perform(curl_struct.conn_info->easy);
        if (curl_res != CURLE_OK) {
            info.response_code = curl_res;
            info.effective_url = curl_struct.conn_info->error;
            return info;
        } else {
            // https://curl.haxx.se/libcurl/c/curl_easy_getinfo.html
            // http://stackoverflow.com/questions/14947821/how-do-i-use-strdup
            long rescode;
            char *effec_url;
            curl_easy_getinfo(curl_struct.conn_info->easy, CURLINFO_RESPONSE_CODE, &rescode);
            curl_easy_getinfo(curl_struct.conn_info->easy, CURLINFO_EFFECTIVE_URL, &effec_url);

            std::memcpy(&info.response_code, &rescode, sizeof(unsigned long)); // Must be trivially copyable otherwise UB!
            info.effective_url = effec_url;
        }
    }

    curlCleanup(curl_struct);
    return info;
}

GekkoFyre::GkCurl::CurlInfoExt GekkoFyre::CurlEasy::curlGrabInfo(const QString &url)
{
    CURLcode curl_res;
    GekkoFyre::GkCurl::CurlInfoExt info;
    GekkoFyre::GkCurl::CurlInit curl_struct = new_easy_handle(url);
    if (curl_struct.conn_info->easy != nullptr) {
        curl_res = curl_easy_perform(curl_struct.conn_info->easy);
        if (curl_res != CURLE_OK) {
            info.response_code = curl_res;
            info.effective_url = curl_struct.conn_info->error;
            return info;
        } else {
            long rescode;
            double elapsed, content_length;
            char *effec_url;
            curl_easy_getinfo(curl_struct.conn_info->easy, CURLINFO_RESPONSE_CODE, &rescode);
            curl_easy_getinfo(curl_struct.conn_info->easy, CURLINFO_TOTAL_TIME, &elapsed);
            curl_easy_getinfo(curl_struct.conn_info->easy, CURLINFO_EFFECTIVE_URL, &effec_url);
            curl_easy_getinfo(curl_struct.conn_info->easy, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);

            // Must be trivially copyable otherwise UB!
            std::memcpy(&info.response_code, &rescode, sizeof(unsigned long));
            std::memcpy(&info.elapsed, &elapsed, sizeof(double));
            std::memcpy(&info.content_length, &content_length, sizeof(double));
            info.effective_url = effec_url;
            info.status_msg = curl_struct.conn_info->error;
            info.status_ok = true;
        }
    }

    curlCleanup(curl_struct);
    return info;
}

void GekkoFyre::CurlEasy::curlCleanup(GekkoFyre::GkCurl::CurlInit curl_init)
{
    // curl_easy_cleanup(), to clean up the context
    // curl_global_cleanup(), to tear down the curl library (once per program)

    curl_easy_cleanup(curl_init.conn_info->easy); // Always cleanup
    if (!curl_init.mem_chunk.memory.empty()) {
        curl_init.mem_chunk.memory.clear();
    }

    curl_init.conn_info->easy = nullptr;
    curl_init.conn_info->url.clear();
    free(curl_init.conn_info);
    return;
}

size_t GekkoFyre::CurlEasy::curl_write_memory_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
    GekkoFyre::GkCurl::MemoryStruct *mstr = static_cast<GekkoFyre::GkCurl::MemoryStruct *>(userp);
    size_t realsize = (size * nmemb);
    mstr->memory.append((char *)ptr, realsize);
    return realsize;
}
