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
 * @file async_buf.hpp
 * @date 2016-11
 * @note <http://stackoverflow.com/questions/21126950/asynchronously-writing-to-a-file-in-c-unix>
 * @brief Asynchronous file writing I/O.
 */

#ifndef ASYNCBUF_HPP
#define ASYNCBUF_HPP

#include "default_var.hpp"
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>

struct async_buf : std::streambuf {
    std::ofstream out;
    std::mutex mutex;
    std::condition_variable condition;
    std::queue<std::vector<char>> queue;
    std::vector<char> buffer;
    std::thread thread;
    bool done;

    void worker() {
        bool local_done(false);
        std::vector<char> buf;
        while (!this->queue.empty() || !local_done) {
            {
                std::unique_lock<std::mutex> guard(this->mutex);
                this->condition.wait(guard, [this](){ return !this->queue.empty() || this->done; });
                while (this->queue.empty() && !this->done) {
                    this->condition.wait(guard);
                }

                if (!this->queue.empty()) {
                    buf.swap(queue.front());
                    queue.pop();
                }

                local_done = this->done;
            }

            if (!buf.empty()) {
                out.write(buf.data(), std::streamsize(buf.size()));
                buf.clear();
            }
        }
    }

public:
    async_buf(std::string const& name) : out(name), buffer(WRITE_BUFFER_SIZE), thread(std::bind(&async_buf::worker, this)) {
        this->setp(this->buffer.data(), this->buffer.data() + this->buffer.size() - 1);
    }

    ~async_buf() {
        {
            std::unique_lock<std::mutex> guard(this->mutex);
            this->done = true;
        }

        this->condition.notify_one();
        this->thread.join();
    }

    int overflow(int c) {
        if (c != std::char_traits<char>::eof()) {
            *this->pptr() = std::char_traits<char>::to_char_type(c);
            this->pbump(1);
        }

        return this->sync() ? std::char_traits<char>::not_eof(c): std::char_traits<char>::eof();
    }

    int sync() {
        if (this->pbase() != this->pptr()) {
            this->buffer.resize(std::size_t(this->pptr() - this->pbase()));
            {
                std::unique_lock<std::mutex> guard(this->mutex);
                this->queue.push(std::move(this->buffer));
            }

            this->condition.notify_one();
            this->buffer = std::vector<char>(WRITE_BUFFER_SIZE);
            this->setp(this->buffer.data(), this->buffer.data() + this->buffer.size() - 1);
        }

        return 0;
    }
};

#endif // ASYNCBUF_HPP
