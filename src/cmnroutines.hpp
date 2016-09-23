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
#include <string>
#include <QString>

extern "C" {
#include <curl/curl.h>
}

namespace GekkoFyre {
class CmnRoutines
{
public:
    CmnRoutines();

    /**
     * @brief GekkoFyre::CmnRoutines::string_sprintf provides std::sprintf-like formatting with the use
     * of std::string and all of its benefits instead.
     * @author  user2622016 <http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf>
     * @date    2015-09-28
     */
    template< typename... Args >
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

    static QString extractFilename(const QString &url);

    struct CurlInfo {
        bool status_ok;            // Whether 'CURLE_OK' was returned or not
        std::string status_msg;    // The status message, if any, returned by the libcurl functions
        long response_code;        // The HTTP/FTP response code
        double elapsed;            // Total time in seconds for the previous transfer (including name resolving, TCP connect, etc.)
        std::string effective_url; // In cases when you've asked libcurl to follow redirects, it may very well not be the same value you set with 'CURLOPT_URL'
        double content_length;     // The size of the download, i.e. content length
    };

    CURL *curlInit(const std::string &url, const std::string &username, const std::string &password);
    CurlInfo curlGrabInfo(const std::string &url);
    void curlGetProgress(CURL *curl_pointer);
    void curlCleanup(CURL *curl_pointer);
};
}

#endif // DL_VIEW_HPP
