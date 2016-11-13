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
 * @file singleton_proc.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-10
 * @brief Verifies and /ensures/ that only a singal instance of this program is operational, for Linux systems.
 */

#ifndef SINGLEPROC_HPP
#define SINGLEPROC_HPP

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

extern "C" {
#include <netinet/in.h>
}

#endif

#include <string>
#include <cstring>

#ifdef __linux__
// Using a socket implementation, this checks to see if the application, "FyreDL", is already open or not.
// The socket is deleted automatically upon exit or crash. You must also choose a port that is unique to
// this application in the usage of this singleton function.
class SingletonProcess {
public:
    SingletonProcess(uint16_t port_zero) : socket_fd(-1), rc(1), port(port_zero) {}

    ~SingletonProcess() {
        if (socket_fd != -1) {
            close(socket_fd);
        }
    }

    bool operator()() {
        if (socket_fd == -1 || rc) {
            socket_fd = -1;
            rc = 1;

            if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                throw std::runtime_error(std::string("Could not create socket: ") + strerror(errno));
            } else {
                struct sockaddr_in name;
                name.sin_family = AF_INET;
                name.sin_port = htons (port);
                name.sin_addr.s_addr = htonl (INADDR_ANY);
                rc = bind(socket_fd, (struct sockaddr *)&name, sizeof(name));
            }
        }

        return (socket_fd != -1 && rc == 0);
    }

    std::string GetLockFileName() {
        return "port " + std::to_string(port);
    }

private:
    int socket_fd = -1;
    int rc;
    uint16_t port;
};
#endif

#endif // SINGLEPROC_HPP
