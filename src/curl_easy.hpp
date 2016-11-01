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
 * @file curl_multi.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11-01
 * @brief This is the easy-interface to libcurl. It's good for verifying, checking, etc. and where a full
 * download of the file is not required, but just the header.
 */

#ifndef FYREDL_CURLEASY_HPP
#define FYREDL_CURLEASY_HPP

#include "curl_multi.hpp"
#include <QObject>
#include <QString>

extern "C" {
#include <curl/curl.h>
}

namespace GekkoFyre {
class CurlEasy : public QObject {
    Q_OBJECT
public:
    CurlEasy();
    ~CurlEasy();

    static GekkoFyre::GkCurl::CurlInfo verifyFileExists(const QString &url);
    static GekkoFyre::GkCurl::CurlInfoExt curlGrabInfo(const QString &url);

private:
    static size_t curl_write_memory_callback(void *ptr, size_t size, size_t nmemb, void *userp);
    static GekkoFyre::GkCurl::CurlInit new_easy_handle(const QString &url);
    static void curlCleanup(GekkoFyre::GkCurl::CurlInit curl_init);
};
}

#endif // FYREDL_CURLEASY_HPP
