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
 * @file singleton_emit.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-10
 * @brief Handles the emitting of signals from a static object.
 */

#ifndef SINGLEEMIT_HPP
#define SINGLEEMIT_HPP

#include <cassert>
#include <exception>
#include <stdexcept>

template<class T>
class SingletonEmit {

public:
    static T* instance() {
        if (!m_instance) {
            m_instance = new T;
        }

        if (m_instance == nullptr) {
            throw std::runtime_error("'m_instance' is a nullptr! Aborting!");
        }

        return m_instance;
    }

protected:
    SingletonEmit();
    ~SingletonEmit();

private:
    SingletonEmit(SingletonEmit const&);
    SingletonEmit& operator=(SingletonEmit const&);
    static T* m_instance;
};

template <class T> T* SingletonEmit<T>::m_instance = NULL;

#endif // SINGLEEMIT_HPP
