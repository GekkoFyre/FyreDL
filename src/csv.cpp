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
**   Copyright (C) 2016-2017. GekkoFyre.
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
 * @file csv.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08
 * @brief A comma-separated-value parser that is used throughout FyreDL.
 */

#include "csv.hpp"
#include <iostream>

GekkoFyre::GkLineReader::GkLineReader(const std::string &csv_data)
{
    if (!csv_data.empty()) {
        csv_raw_data << csv_data;
    }
}

bool GekkoFyre::GkLineReader::has_column(const std::string &name)
{
    return false;
}

std::unordered_map<int, std::string> GekkoFyre::GkLineReader::read_lines()
{
    std::unordered_map<int, std::string> lines;
    if (!csv_raw_data.str().empty()) {
        if (search_string(csv_raw_data.str(), "\r\n")) {
            csv_raw_data.str().erase(std::remove(csv_raw_data.str().begin(), csv_raw_data.str().end(), '\r'),
                                     csv_raw_data.str().end());
        } else if (!search_string(csv_raw_data.str(), "\n")) {
            throw std::invalid_argument("Unable to find any suitable line-endings for the given CSV data!");
        }

        std::string row;
        while (std::getline(csv_raw_data, row, '\n')) {
            std::iota(rows.begin(), rows.end(), rows.back());
            lines.insert(std::make_pair(rows.back(), row));
        }
    }

    return lines;
}

void GekkoFyre::GkLineReader::parse_csv(std::string *args, ...)
{
    return;
}
