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

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include <type_traits>
#include <QString>
#include <QObject>

extern "C" {
#include <curl/curl.h>
}

namespace gk {
/**
 * @brief gk::string_sprintf provides 'std::sprintf-like formatting' with the use of std::string and all of
 * its benefits instead.
 * @author  user2622016 <http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf>
 * @date    2015-09-28
 */
template<typename... Args>
static std::string string_sprintf(const char* format, Args... args) {
    int length = std::snprintf(nullptr, 0, format, args...);
    assert(length >= 0);
    length = length + 1;

    char* buf = new char[(unsigned long)abs(length)]();
    std::snprintf(buf, (unsigned long)abs(length), format, args...);

    std::string str(buf);
    delete[] buf;
    return str;
}

/**
 * @brief gk::anydup allows the returning of a type, T, to a duplicate location in memory.
 * @author  Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 *          Christophe <http://stackoverflow.com/questions/39690024/a-templated-strdup>
 *          Kerrek SB <http://stackoverflow.com/questions/8287109/how-to-copy-one-integer-array-to-another>
 * @date    2016-09
 */
template<typename T>
static T anydup(T *src, size_t len) {
    if (std::is_trivially_copyable<T>::value) {
            T *ptr = new T[len];
            memcpy(ptr, src, (len * sizeof(T)));
            return *ptr;
} else {
        throw std::invalid_argument("T must be trivially copyable.");
    }
}
}

namespace GekkoFyre {
class CmnRoutines : public QObject
{
    Q_OBJECT

private:
    struct MemoryStruct {
        char *memory;
        size_t size;
    };

    struct CurlInit {
        char errbuf[CURL_ERROR_SIZE];
        CURLcode curl_res;
        CURL *curl_ptr;
        MemoryStruct mem_chunk;
    };

public:
    CmnRoutines();
    ~CmnRoutines();

    QString extractFilename(const QString &url);

    struct CurlInfo {
        long response_code;  // The HTTP/FTP response code
        char *effective_url; // In cases when you've asked libcurl to follow redirects, it may very well not be the same value you set with 'CURLOPT_URL'
    };

    struct CurlInfoExt {
        bool status_ok;            // Whether 'CURLE_OK' was returned or not
        std::string status_msg;    // The status message, if any, returned by the libcurl functions
        long response_code;        // The HTTP/FTP response code
        double elapsed;            // Total time in seconds for the previous transfer (including name resolving, TCP connect, etc.)
        std::string effective_url; // In cases when you've asked libcurl to follow redirects, it may very well not be the same value you set with 'CURLOPT_URL'
        double content_length;     // The size of the download, i.e. content length
    };

    CurlInfo verifyFileExists(const QString &url);
    CurlInfoExt curlGrabInfo(const QString &url);
    void curlGetProgress(CURL *curl_pointer);

private:
    static size_t curl_write_memory_callback(void *ptr, size_t size, size_t nmemb, void *userp);
    CurlInit curlInit(const QString &url, const std::string &username, const std::string &password);
    void curlCleanup(CurlInit curl_init);
};
}

#endif // CMNROUTINES_HPP
