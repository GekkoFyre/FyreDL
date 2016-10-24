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
