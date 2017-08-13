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

#include <utility>
#include <algorithm>
#include <numeric>
#include <list>
#include <string>
#include <sstream>
#include <unordered_map>
#include <map>

namespace GekkoFyre {
class GkCsvReader {
public:
    GkCsvReader() = delete;
    GkCsvReader(const GkCsvReader&) = delete;
    explicit GkCsvReader(const std::string &csv_data);

    template<typename I, typename ...ColNames>
    void add_headers(const I &given_cols, ColNames... args) {
        headers.clear();
        headers = { to_string(args)... };
        cols_count = sizeof...(args);
        rows_parsed = 0;
        parse_csv();
    }

    virtual bool has_column(const std::string &name);
    virtual int determine_column(const std::string &header);
    virtual bool force_cache_reload();

    /**
     * @brief GekkoFyre::GkCsvReader::read_row will output the CSV data via the arguments in a variadic fashion, and is intended
     * to be used with a while() loop.
     * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
     * @date 2017-08-13
     * @param cols The ouputted information. Depending on if this is the first, second, third, etc. argument, it will signify
     * which column to draw the data from.
     * @return When to abort or repeat the while() loop.
     */
    template<typename ...ColType>
    bool read_row(ColType& ...cols) {
        int num_args = sizeof...(cols);
        if ((num_args > 0) && (!csv_data.empty()) && (rows_parsed <= rows_count)) {
            read_row_helper(rows_parsed, 0, std::forward<ColType>(cols)...);
            ++rows_parsed;
            return true;
        }

        return false;
    }

private:
    template<typename T>
    std::string to_string(const T &value) {
        std::ostringstream ss;
        ss << value;
        return ss.str();
    }

    std::string to_string(const std::string &value) {
        return value;
    }

    template<class Iter>
    struct iter_for_range : std::pair<Iter, Iter> {
        iter_for_range(std::pair<Iter, Iter> const &x) : std::pair<Iter, Iter>(x) {}
        Iter begin() const { return this->first; }
        Iter end() const { return this->second; }
    };

    template<class Iter>
    inline iter_for_range<Iter> as_range(std::pair<Iter, Iter> const &x) {
        return iter_for_range<Iter>(x);
    }

    std::stringstream csv_raw_data;
    size_t cols_count;
    size_t rows_count;
    size_t rows_parsed;
    std::list<std::string> headers;                           // The key is the column number, whilst the value is the header associated with that column.
    std::multimap<int, std::pair<int, std::string>> csv_data; // The key is the row number whilst the values are are the column number and the comma-separated-values.

    template<typename Container>
    bool search_string(const Container &cont, const std::string &to_find) {
        return std::search(cont.begin(), cont.end(), to_find.begin(), to_find.end()) != cont.end();
    }

    std::unordered_map<int, std::string> read_rows();
    std::map<int, std::string> split_values(std::stringstream raw_csv_line);
    void parse_csv();

    template<typename ...ColType>
    void read_row_helper(std::size_t row, std::size_t last_col, std::string &&new_col, ColType&& ...cols) {
        static size_t last_col_store;
        if ((last_col != 0) && (last_col_store >= last_col)) {
            if (!csv_data.empty() && row <= rows_count) {
                auto range = csv_data.equal_range(row);
                for (const auto &value: as_range(range)) {
                    last_col_store = last_col;
                    new_col = value.second.second;
                    read_row_helper(row, last_col, std::forward<std::string>(new_col), std::forward<ColType>(cols)...);
                }
            }
        } else {
            // We are not dealing with the first column, and we've already dealt with the current column
            if (last_col != cols_count) {
                read_row_helper(row, (last_col + 1), std::forward<std::string>(new_col), std::forward<ColType>(cols)...);
            }
        }

        if (last_col_store >= cols_count) {
            last_col_store = 0;
            return;
        }

        return;
    }
};
}

#endif // GKCSV_HPP