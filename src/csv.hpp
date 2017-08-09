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
 * @file csv.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08
 * @brief A comma-separated-value parser that is used throughout FyreDL.
 */

#ifndef GKCSV_HPP
#define GKCSV_HPP

#include "default_var.hpp"
#include <vector>
#include <utility>
#include <algorithm>
#include <numeric>
#include <string>
#include <sstream>
#include <list>
#include <unordered_map>

namespace GekkoFyre {
class GkLineReader {
protected:
    GkLineReader() = delete;
    GkLineReader(const GkLineReader&) = delete;
    explicit GkLineReader(const std::string &csv_data);

    template<typename T>
    std::string to_string(const T &value) {
        std::ostringstream ss;
        ss << value;
        return ss.str();
    }

    std::string to_string(const std::string &value) {
        return value;
    }

    template<typename ...Ts>
    void parse_headers(Ts... args) {
        std::iota(columns.begin(), columns.end(), columns.back());
        headers.insert(std::make_pair(columns.back(), to_string(args...)));
    }

    virtual bool has_column(const std::string &name);
    virtual void parse_csv(std::string *args, ...);

private:
    std::stringstream csv_raw_data;
    std::list<int> columns;
    std::list<int> rows;
    std::unordered_map<int, std::string> headers;                       // The key is the column number, whilst the value is the header associated with that column.
    std::unordered_multimap<int, std::pair<int, std::string>> values;   // The key is the row number, whilst the values are the column number and comma-separated-values.

    template<typename Container>
    bool search_string(const Container &cont, const std::string &to_find) {
        return std::search(cont.begin(), cont.end(), to_find.begin(), to_find.end()) != cont.end();
    }

    std::unordered_map<int, std::string> read_lines();
};

class GkCsvReader : GkLineReader {
private:
    GkLineReader parse;

public:
    GkCsvReader() = delete;
    GkCsvReader(const GkCsvReader&) = delete;
    template<class ...Args>
    explicit GkCsvReader(Args&&...args);
};
}

#endif // GKCSV_HPP