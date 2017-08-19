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

GekkoFyre::GkCsvReader::GkCsvReader(const int &column_count, const std::string &csv_data)
{
    cols_count = column_count;
    if (!csv_data.empty()) {
        csv_raw_data << csv_data;
    }
}

bool GekkoFyre::GkCsvReader::has_column(const std::string &name)
{
    auto rows = read_rows();
    if (!rows.empty()) {
        for (const auto &row: rows) {
            // Key is the row number and value is the row of raw data
            auto cols = split_values(std::stringstream(row.second));
            if (row.first == 0) {
                // Determine headers
                for (const auto &col: cols) {
                    if (col.second == name) {
                        return true;
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
 * @return A std::unordered_map where the key is the row number, and the value is the row of raw data.
 */
std::unordered_map<int, std::string> GekkoFyre::GkCsvReader::read_rows()
{
    std::unordered_map<int, std::string> lines;
    if (!csv_raw_data.str().empty()) {
        if (search_string(csv_raw_data.str(), "\r\n")) {
            csv_raw_data.str().erase(std::remove(csv_raw_data.str().begin(), csv_raw_data.str().end(), '\r'),
                                     csv_raw_data.str().end());
        } else if (!search_string(csv_raw_data.str(), "\n")) {
            throw std::invalid_argument("Unable to find any suitable line-endings for the given CSV data!");
        }

        if (rows_count != 0) {
            rows_count = 0;
        }

        std::string row;
        while (std::getline(csv_raw_data, row, '\n')) {
            ++rows_count;
            lines.insert(std::make_pair(rows_count, row));
        }
    }

    return lines;
}

/**
 * @brief GekkoFyre::GkCsvReader::parse_csv will parse the CSV data line-by-line and separate out the columns before
 * inserting them into an std::unordered_map, ready for immediate use or ideally, further processing.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-10
 */
void GekkoFyre::GkCsvReader::parse_csv()
{
    auto rows = read_rows();
    if (!rows.empty()) {
        csv_data.clear();
        for (const auto &row: rows) {
            // Key is the row number and value is the row of raw data
            auto cols = split_values(std::stringstream(row.second));
            if (row.first == 0) {
                // Determine headers
                for (const auto &col: cols) {
                    for (const auto &header: headers) { // Check that the given headers match what is found in the CSV data
                        if (col.second != header) {
                            std::cerr << "Unattributed column found, \"" << col.second << "\" whilst parsing CSV data! Ignoring..." << std::endl;
                        }
                    }
                }
            } else {
                for (const auto &col: cols) {
                    csv_data.insert(std::make_pair(row.first, std::make_pair(col.first, col.second)));
                }
            }
        }
    }
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
    auto rows = read_rows();
    if (!rows.empty()) {
        for (const auto &row: rows) {
            // Key is the row number and value is the row of raw data
            auto cols = split_values(std::stringstream(row.second));
            if (row.first == 0) {
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
 * @return Outputs a std::unordered_map with the key being the column number and the value being the information
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

/**
 * @brief GekkoFyre::GkCsvReader::force_cache_reload forces a reload of the CSV data cache.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-08-13
 * @return Whether the operation succeeded or not.
 */
bool GekkoFyre::GkCsvReader::force_cache_reload()
{
    parse_csv();
    rows_parsed = 0;
    already_run = false;
    return !csv_data.empty();
}
