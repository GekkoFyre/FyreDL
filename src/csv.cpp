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

bool GekkoFyre::GkCsvReader::already_looped;
QMultiMap<int, int> GekkoFyre::GkCsvReader::proc_cols;

void GekkoFyre::GkCsvReader::read_row()
{
    return;
}

bool GekkoFyre::GkCsvReader::has_column(const std::string &name)
{
    if (key) {
        for (const auto &header: headers) {
            if (header == name) {
                return true;
            }
        }
    } else {
        auto rows = read_rows(csv_raw_data.str());
        if (!rows.empty()) {
            for (const auto &row: rows) {
                // Key is the row number and value is the row of raw data
                auto cols = split_values(std::stringstream(row.second));
                if (row.first == 1) {
                    // Determine headers
                    for (const auto &col: cols) {
                        if (col.second == name) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

/**
 * @brief GekkoFyre::GkCvsReader::read_rows reads each row of the CSV data and outputs them into a STL container.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-09
 * @return A std::map where the key is the row number, and the value is the row of raw data.
 */
std::map<int, std::string> GekkoFyre::GkCsvReader::read_rows(std::string raw_data)
{
    std::map<int, std::string> lines;
    if (!raw_data.empty()) {
        if (search_string(raw_data, "\r\n")) {
            raw_data.erase(std::remove(raw_data.begin(), raw_data.end(), '\r'),
                                     raw_data.end());
        } else if (!search_string(raw_data, "\n")) {
            throw std::invalid_argument("Unable to find any suitable line-endings for the given CSV data!");
        }

        if (rows_count != 0) {
            rows_count = 0;
        }

        std::string row;
        std::stringstream istring;
        istring << raw_data;
        while (std::getline(istring, row, '\n')) {
            ++rows_count;
            lines.insert(std::make_pair(rows_count, row));
        }

        if (!key) {
            rows_count = (rows_count - 1); // Remove the column containing the headers
        }
    }

    return lines;
}

/**
 * @brief GekkoFyre::GkCsvReader::parse_csv will parse the CSV data line-by-line and separate out the columns before
 * inserting them into an std::map, ready for immediate use or ideally, further processing.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-10
 */
std::multimap<int, std::pair<int, std::string>> GekkoFyre::GkCsvReader::parse_csv()
{
    auto rows = read_rows(csv_raw_data.str());
    if (!rows.empty()) {
        csv_data.clear();
        std::multimap<int, std::pair<int, std::string>> output;
        size_t start_row = 0;
        if (!key) {
            start_row = 1;
        }

        for (size_t i = start_row; i < rows.size(); ++i) {
            std::string csv;
            int col = 0;
            std::stringstream tmp;
            tmp << extract_values(rows).at(i);

            while (std::getline(tmp, csv, ',')) {
                if (col >= cols_count) {
                    col = 0;
                }

                ++col;
                output.insert(std::make_pair(((i + 1) - start_row), std::make_pair(col, csv)));
            }
        }

        return output;
    }

    return std::multimap<int, std::pair<int, std::string>>();
}

/**
 * @brief GekkoFyre::GkCvsReader::determine_column will determine the given column number for a header.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-09
 * @param header The header you wish to determine the column number for.
 * @return The column number for a given header. If integer '-1' is returned, the header was not found.
 */
int GekkoFyre::GkCsvReader::determine_column(const std::string &header)
{
    auto rows = read_rows(csv_raw_data.str());
    if (!rows.empty()) {
        for (const auto &row: rows) {
            // Key is the row number and value is the row of raw data
            auto cols = split_values(std::stringstream(row.second));
            if (row.first == 1) {
                // Determine headers
                for (const auto &col: cols) {
                    if (col.second == header) {
                        return col.first;
                    }
                }
            }
        }
    }

    return -1;
}

/**
 * @brief GekkoFyre::GkCvsReader::split_values splits a single row into its individual columns.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-09
 * @param raw_csv_line The raw line of CSV data to parse.
 * @return Outputs a std::map with the key being the column number and the value being the information
 * in that column.
 */
std::map<int, std::string> GekkoFyre::GkCsvReader::split_values(std::stringstream raw_csv_line)
{
    std::string csv;
    std::map<int, std::string> output;
    int col = 0;
    while (std::getline(raw_csv_line, csv, ',')) {
        ++col;
        output.insert(std::make_pair(col, csv));
    }

    return output;
}

void GekkoFyre::GkCsvReader::read_row_helper(int col_no, int row_no, std::string &val)
{
    if (col_no == 0 || row_no == 0) {
        return;
    }

    if (!csv_data.empty()) {
        static std::string prev_val;
        while (col_no <= cols_count) {
            for (auto col: csv_data) {
                if (!proc_cols.contains(row_no, col_no)) {
                    if ((col.first == row_no) && (col.second.first == col_no)) {
                        if (std::strcmp(prev_val.c_str(), col.second.second.c_str()) == 0) {
                            return;
                        }

                        prev_val = col.second.second;
                        proc_cols.insertMulti(row_no, col_no);
                        val = prev_val;
                        return;
                    }
                } else {
                    return;
                }
            }
        }
    }

    return;
}

/**
 * @brief GekkoFyre::GkCsvReader::force_cache_reload forces a reload of the CSV data cache.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-13
 * @return Whether the operation succeeded or not.
 */
bool GekkoFyre::GkCsvReader::force_cache_reload()
{
    already_looped = false;
    proc_cols.clear();
    csv_data.clear();
    csv_data = parse_csv();
    return !csv_data.empty();
}
