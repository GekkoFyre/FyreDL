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
 * @file default_var.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief Default variables used throughout the program that the user may change to suit their preferences
 */

#ifndef DEFVAR_HPP
#define DEFVAR_HPP

#include <locale>
#include <string>

#define CFG_HISTORY_FILE "fyredl_history.xml"
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL 3
#define MAX_WAIT_MSECS (30 * 1000) /* Wait max. 30 seconds */

#define MN_FILENAME_COL 0
#define MN_FILESIZE_COL 1
#define MN_DOWNLOADED_COL 2
#define MN_PROGRESS_COL 3
#define MN_UPSPEED_COL 4
#define MN_DOWNSPEED_COL 5
#define MN_STATUS_COL 6
#define MN_DESTINATION_COL 7
#define MN_URL_COL 8

#define CURL_MAX_WAIT_MSECS 15000

namespace GekkoFyre {
enum DownloadStatus {
    Downloading,
    Completed,
    Failed,
    Paused,
    Stopped,
    Unknown
};

enum DownloadType {
    HTTP,
    FTP,
    Torrent
};
}

#endif // DEFVAR_HPP
