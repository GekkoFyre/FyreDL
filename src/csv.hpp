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
#include <array>
#include <tuple>

namespace GekkoFyre {
class GkCsvReader {
public:
    GkCsvReader() = delete;
    GkCsvReader(const GkCsvReader&) = delete;
    explicit GkCsvReader(const int &column_count, const std::string &csv_data);

    template<typename ...ColNames>
    void add_headers(ColNames... args) {
        headers.clear();
        headers = { to_string(args)... };
        constexpr int args_count = sizeof...(args);
        cols_count = args_count;
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
    template<typename ...ColTypes>
    bool read_row(ColTypes& ...cols) {
        static int rows_parsed = 0;
        if (!csv_data.empty()) {
            ++rows_parsed;
            if (rows_parsed <= rows_count) {
                static int col_no = -1;
                while (col_no <= (cols_count - 1)) {
                    ++col_no;
                    parse_cols<std::string>(col_no, parse_helper(parse_cols_ptr<std::string>(col_no, read_row_helper<std::string>(rows_parsed, col_no, cols)...), cols)...);
                };

                if (col_no >= cols_count) {
                    return true;
                }
            }
        }

        return false;
    }

private:
    template<typename ...Ts>
    auto dummy_func(Ts ...args) {
        return std::make_tuple(args...);
    }

    template<typename T>
    std::string to_string(const T &value) {
        std::ostringstream ss;
        ss << value;
        return ss.str();
    }

    std::string to_string(const std::string &value) {
        return value;
    }

    std::stringstream csv_raw_data;
    int cols_count;
    int rows_count;
    std::mutex mutex;
    std::list<std::string> headers;                           // The key is the column number, whilst the value is the header associated with that column.
    std::multimap<int, std::pair<int, std::string>> csv_data; // The key is the row number whilst the values are are the column number and the comma-separated-values.

    template<typename Container>
    bool search_string(const Container &cont, const std::string &to_find) {
        return std::search(cont.begin(), cont.end(), to_find.begin(), to_find.end()) != cont.end();
    }

    std::unordered_map<int, std::string> read_rows();
    std::map<int, std::string> split_values(std::stringstream raw_csv_line);
    void parse_csv();

    template<typename T, typename X>
    auto parse_helper(T *value, X& col) {
        if (value != nullptr) {
            col = *value;
            return *value;
        }

        return std::string();
    };

    template<typename T, typename ...Ts>
    auto parse_cols(const int &col_no, Ts ...values) {
        std::array<T, 15> init_array { values... };
        T val_ret;
        for (size_t i = 0; i < init_array.size(); ++i) {
            if (col_no == (int)i) {
                val_ret = init_array[col_no];
            }
        }

        return val_ret;
    }

    template<typename T, typename ...Ts>
    auto parse_cols_ptr(const int &col_no, Ts* ...values) {
        std::array<T*, 15> init_array { values... };
        T *val_ret = nullptr;
        for (size_t i = 0; i < init_array.size(); ++i) {
            if (col_no == (int)i) {
                val_ret = init_array[col_no];
            }
        }

        return val_ret;
    }

    template<typename T>
    T* read_row_helper(const int &row_no, const int &col_no, T& col) {
        if (!csv_data.empty() && (row_no <= rows_count)) {
            T val;
            for (auto id: csv_data) {
                if ((id.first == row_no) && (id.second.first == (col_no + 1))) {
                    val = id.second.second;
                }
            }

            return new T(std::forward<T>(val));
        }

        return new T(std::forward<T>(col));
    }
};
}

#endif // GKCSV_HPP