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
        rows_parsed = 0;
        already_run = false;
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
    bool read_row(ColType&& ...cols) {
        constexpr size_t num_args = sizeof...(cols);
        if ((num_args > 0) && (!csv_data.empty()) && (rows_parsed <= rows_count)) {
            read_row_helper<ColType...>(rows_parsed, cols...);
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
    int cols_count;
    int rows_count;
    int rows_parsed;
    bool already_run;
    std::list<std::string> headers;                           // The key is the column number, whilst the value is the header associated with that column.
    std::multimap<int, std::pair<int, std::string>> csv_data; // The key is the row number whilst the values are are the column number and the comma-separated-values.

    template<typename Container>
    bool search_string(const Container &cont, const std::string &to_find) {
        return std::search(cont.begin(), cont.end(), to_find.begin(), to_find.end()) != cont.end();
    }

    std::unordered_map<int, std::string> read_rows();
    std::map<int, std::string> split_values(std::stringstream raw_csv_line);
    void parse_csv();

    // Create a tuple from the array
    template<typename Array, size_t ...I>
    auto array_to_tuple_helper(const Array &array, std::index_sequence<I...>) {
        return std::make_tuple(array[I]...);
    };

    template<class... T>
    using index_sequence_for = std::make_index_sequence<sizeof...(T)>;

    // Create an index sequence for the array, and pass it to the implementation function helper
    template<typename T, size_t N>
    auto array_to_tuple(const std::array<T, N> &array) {
        return array_to_tuple_helper(array, std::make_index_sequence<N>());
    };

    // https://stackoverflow.com/questions/687490/how-do-i-expand-a-tuple-into-variadic-template-functions-arguments
    // http://aherrmann.github.io/programming/2016/02/28/unpacking-tuples-in-cpp14/
    template <class F, size_t ...Is>
    constexpr auto index_apply_impl(F f, std::index_sequence<Is...>) {
        return f(std::integral_constant<size_t, Is> {}...);
    }

    template <size_t N, class F>
    constexpr auto index_apply(F f) {
        return index_apply_impl(f, std::make_index_sequence<N>());
    }

    template<class Tuple>
    constexpr auto tuple_size(Tuple t) {
        return std::min({std::tuple_size<Tuple>{}});
    }

    template<typename ...ColType>
    void read_row_helper(const int &row, ColType ...cols) {
        if (!csv_data.empty() && !already_run) {
            int col = 1;
            constexpr size_t num_args = sizeof...(cols);
            std::array<std::string, num_args> col_data;
            if (((col - 1) <= cols_count) && (row <= rows_count)) {
                for (auto id: csv_data) {
                    if ((id.first == row) && (id.second.first == col)) {
                        col_data[col - 1] = id.second.second;
                        ++col;
                    }
                }
            }

            if (!col_data.empty() && (col == (cols_count + 1))) {
                auto tuple = array_to_tuple<std::string, num_args>(col_data);
                const auto len = tuple_size(tuple);
                index_apply<len>([&](auto ...Is) {
                    already_run = true;
                    read_row(std::forward<std::string>(std::get<Is>(tuple))...);
                });
                return;
            }
        }

        return;
    }
};
}

#endif // GKCSV_HPP