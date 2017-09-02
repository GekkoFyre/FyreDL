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
#include <map>
#include <array>
#include <memory>
#include <mutex>
#include <QMultiMap>

namespace GekkoFyre {
class GkCsvReader {
public:
    GkCsvReader() = delete;
    GkCsvReader(const GkCsvReader&) = delete;

    template<typename ...Headers>
    explicit GkCsvReader(const int &column_count, const bool &download_ids, const std::string &csv_data, const Headers& ...headers) {
        cols_count = column_count;
        key = download_ids;
        if (!csv_data.empty()) {
            csv_raw_data << csv_data;
            already_looped = false;
            add_headers(headers...);
            return;
        } else {
            rows_count = 0;
            key = false;
            already_looped = false;
        }

        return;
    }

    virtual bool has_column(const std::string &name);
    virtual int determine_column(const std::string &header);
    virtual bool force_cache_reload();
    bool read_row();

    /**
     * @brief GekkoFyre::GkCsvReader::read_row will output the CSV data via the arguments in a variadic fashion, and is intended
     * to be used with a while() loop.
     * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
     * @date 2017-08-13
     * @param cols The ouputted information. Depending on if this is the first, second, third, etc. argument, it will signify
     * which column to draw the data from.
     * @return When to abort or repeat the while() loop.
     */
    template<typename T, typename ...ColTypes>
    bool read_row(T& col, ColTypes& ...cols) {
        static int cols_parsed;
        rows_parsed = 0;
        cols_parsed = 0;
        static bool ret_val;
        if (!csv_data.empty()) {
            while ((rows_parsed + 1) <= rows_count) {
                while (cols_parsed <= (cols_count - 1)) { // Keep this at `(cols_count - 1)`!.
                    ++cols_parsed;
                    std::string col_data;
                    col_data = read_row_helper(cols_parsed, (rows_parsed + 1));
                    if (!col_data.empty()) {
                        col = col_data;
                        ret_val = read_row(cols...);
                    }
                }

                cols_parsed = 0;

                if (excl_lock.try_lock()) {
                    ++rows_parsed;
                }
            }
        }

        if ((rows_parsed == rows_count) && !already_looped) {
            already_looped = true;
            excl_lock.unlock();
            return true;
        }

        if ((cols_count % 2) == 0 && already_looped) { // This is an absolute hack, I know...
            already_looped = false;
            excl_lock.unlock();
            return true;
        }

        rows_parsed = 0;
        already_looped = false;
        excl_lock.unlock();
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

    std::stringstream csv_raw_data;
    int cols_count;
    int rows_count;
    static int rows_parsed;
    static bool already_looped;
    std::recursive_mutex excl_lock;
    bool key;
    std::list<std::string> headers;                      // The key is the column number, whilst the value is the header associated with that column.
    static QMultiMap<int, int> proc_cols;
    std::multimap<int, std::pair<int, std::string>> csv_data; // The key is the row number whilst the values are are the column number and the comma-separated-values.

    template<typename Container>
    bool search_string(const Container &cont, const std::string &to_find) {
        return std::search(cont.begin(), cont.end(), to_find.begin(), to_find.end()) != cont.end();
    }

    template<typename ...Headers>
    void add_headers(const Headers& ...args) {
        headers = { to_string(args)... };
        constexpr int args_count = sizeof...(args);
        cols_count = args_count;
        csv_data.clear();
        proc_cols.clear();
        csv_data = parse_csv();
    }

    template<typename T, typename X>
    std::vector<T> extract_keys(const std::map<T, X> &input_map) {
        std::vector<T> ret_val;
        for (const auto &element: input_map) {
            ret_val.push_back(element.second);
        }

        return ret_val;
    }

    template<typename T, typename X>
    std::vector<X> extract_values(const std::map<T, X> &input_map) {
        std::vector<X> ret_val;
        for (const auto &element: input_map) {
            ret_val.push_back(element.second);
        }

        return ret_val;
    }

    std::map<int, std::string> read_rows(std::string raw_data);
    std::map<int, std::string> split_values(std::stringstream raw_csv_line);
    std::multimap<int, std::pair<int, std::string>> parse_csv();
    std::string read_row_helper(const int &col_no, const int &row_no);
};
}

#endif // GKCSV_HPP