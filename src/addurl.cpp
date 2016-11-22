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
 * @file addurl.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-10
 * @brief A Qt QDialog class that handles the adding of downloads to the application.
 */

#include "addurl.hpp"
#include "ui_addurl.h"
#include "default_var.hpp"
#include "curl_multi.hpp"
#include "curl_easy.hpp"
#include "./../utils/fast-cpp-csv-parser/csv.h"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <exception>
#include <stdexcept>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

namespace fs = boost::filesystem;
AddURL::AddURL(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddURL)
{
    ui->setupUi(this);
    routines = new GekkoFyre::CmnRoutines();

    ui->url_dest_lineEdit->setText(QDir::homePath());
    ui->file_dest_lineEdit->setText("");
}

AddURL::~AddURL()
{
    delete ui;
    delete routines;
}

/**
 * @brief AddURL::on_buttonBox_accepted processes all the data when the user clicks the 'Accept' pushbutton.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11-15
 * @note <https://github.com/ben-strasser/fast-cpp-csv-parser>
 *       <https://www.mockaroo.com/>
 */
void AddURL::on_buttonBox_accepted()
{
    GekkoFyre::GkCurl::CurlInfo info;
    GekkoFyre::GkCurl::CurlInfoExt info_ext;
    GekkoFyre::GkCurl::CurlDlInfo dl_info;
    QString url_plaintext;
    QString hash_plaintext;

    if (ui->inputTabWidget->currentIndex() == 0) {
        // #################################
        // We're only importing a single URL
        // #################################
        QString file_dest = ui->url_dest_lineEdit->text();
        url_plaintext = ui->url_plainTextEdit->toPlainText();
        if (file_dest.isEmpty()) {
            QMessageBox::information(this, tr("Problem!"), tr("You must specify a destination directory."),
                                     QMessageBox::Ok);
            return AddURL::done(QDialog::Rejected);
        } else if (url_plaintext.isEmpty()) {
            QMessageBox::information(this, tr("Problem!"), tr("You must specify a URL to grab."),
                                     QMessageBox::Ok);
            return AddURL::done(QDialog::Rejected);
        } else {
            try {
                // Check that the path exists and is a /directory/.
                std::string part_dest = ui->url_dest_lineEdit->text().toStdString();
                fs::path boost_file_dest(part_dest);
                if (fs::exists(boost_file_dest) && fs::is_directory(boost_file_dest)) {
                    hash_plaintext = ui->url_hash_lineEdit->text();
                    info = GekkoFyre::CurlEasy::verifyFileExists(url_plaintext);

                    // Check that the file exists on the web-server, with a return code of '200'
                    if (info.response_code == 200) {
                        // The URL exists!
                        // Now check it for more detailed information
                        info_ext = GekkoFyre::CurlEasy::curlGrabInfo(url_plaintext);
                        std::ostringstream file_comp_path;
                        file_comp_path << part_dest << fs::path::preferred_separator
                                       << routines->extractFilename(QString::fromStdString(info_ext.effective_url)).toStdString();

                        // Save this more detailed information
                        dl_info.dlStatus = GekkoFyre::DownloadStatus::Stopped;
                        dl_info.file_loc = file_comp_path.str();
                        dl_info.ext_info.content_length = info_ext.content_length;
                        dl_info.ext_info.effective_url = info_ext.effective_url;
                        dl_info.ext_info.response_code = info_ext.response_code;
                        dl_info.ext_info.status_ok = info_ext.status_ok;
                        dl_info.cId = 0;
                        dl_info.timestamp = 0;

                        // Set default values for the hash(es) if none specified by the user
                        if (hash_plaintext.isEmpty()) {
                            dl_info.hash_type = GekkoFyre::HashType::None;
                            dl_info.hash_val_given = "";
                        } else {
                            dl_info.hash_type = GekkoFyre::HashType::CannotDetermine;
                            dl_info.hash_val_given = hash_plaintext.toStdString();
                        }

                        // Write the output to an XML file, with file-name 'CFG_HISTORY_FILE'
                        routines->writeDownloadItem(dl_info);

                        // Send any new details to the QTableView model/view routines, whereupon 'ui->downloadView' is
                        // updated with the latest data.
                        emit sendDetails(dl_info.ext_info.effective_url, dl_info.ext_info.content_length, 0, 0, 0,
                                         0, dl_info.dlStatus, dl_info.ext_info.effective_url, dl_info.file_loc);
                        return AddURL::done(QDialog::Accepted);
                    } else {
                        // We received something other than a '200' response code, so either the URL does not exist or
                        // there was another problem that had been encountered.
                        QMessageBox::information(this, tr("Error!"), QString("%1").arg(
                                QString::fromStdString(info.effective_url)), QMessageBox::Ok);
                        return AddURL::done(QDialog::Rejected);
                    }
                }
            } catch (const std::exception &e) {
                QMessageBox::warning(this, tr("Error!"), tr("%1").arg(e.what()), QMessageBox::Ok);
                return;
            }
        }
    } else {
        // ######################################
        // We're importing a mass-amount of URLs!
        // ######################################
        try {
            QString csv_file = ui->file_import_lineEdit->text();
            if (csv_file.isEmpty()) {
                QMessageBox::information(this, tr("Problem!"), tr("You must specify a target file to import!"),
                                         QMessageBox::Ok);
                return AddURL::done(QDialog::Rejected);
            } else {
                // Grab the path to the CSV file to import, if any
                fs::path csv_path(csv_file.toStdString());

                // Check that the CSV file exists
                if (!fs::exists(csv_path)) {
                    throw std::invalid_argument(tr("The file you have chosen does not exist!\n\n%1").arg(csv_file).toStdString());
                }

                // Check that the CSV file is not a directory
                if (fs::is_directory(csv_path)) {
                    throw std::invalid_argument(tr("You selected a directory, not a file!\n\n%1").arg(csv_file).toStdString());
                }

                // Process the CSV file
                io::CSVReader<CSV_NUM_COLS> in(csv_file.toStdString());
                in.read_header(io::ignore_missing_column, CSV_FIELD_URL, CSV_FIELD_DEST, CSV_FIELD_HASH); // If a column with a name is not in the file but is in the argument list, then read_row will not modify the corresponding variable.
                if (!in.has_column(CSV_FIELD_URL)) {
                    throw std::invalid_argument(tr("Error reading CSV file! Is it formatted correctly?\n\n%1")
                                                        .arg(csv_file).toStdString());
                }

                QString csv_file_dest = ui->file_dest_lineEdit->text();
                if (!in.has_column(CSV_FIELD_DEST) && csv_file_dest.isEmpty()) {
                    throw std::invalid_argument(tr("No destination provided either in dialog or CSV file!")
                                                        .toStdString());
                }

                struct CsvImport {
                    std::string url;
                    std::string dest;
                    std::string hash;
                };

                std::vector<CsvImport> csv_vec;
                std::string url, dest, hash = { "" };
                bool has_col_hash = false;
                if (in.has_column(CSV_FIELD_HASH)) { has_col_hash = true; }

                while (in.read_row(url, dest, hash)) {
                    // Handle the imported CSV data
                    CsvImport csv_import;
                    csv_import.url = url;
                    csv_import.dest = dest;

                    // Set default values for the hash(es), only, if none specified by the user
                    if (has_col_hash) {
                        csv_import.hash = hash;
                    } else {
                        csv_import.hash = "";
                    }

                    // Temporarily push the struct, CsvImport, onto the std::vector<CsvImport>()
                    csv_vec.push_back(csv_import);
                }

                // Process the now imported CSV data from std::vector<CsvImport>()
                for (size_t i = 0; i < csv_vec.size(); ++i) {
                    // Check that the file exists with a '200' return code from the web-server
                    info = GekkoFyre::CurlEasy::verifyFileExists(QString::fromStdString(csv_vec.at(i).url));
                    if (info.response_code == 200) {
                        // The URL exists!
                        // Now check it for more detailed information
                        info_ext = GekkoFyre::CurlEasy::curlGrabInfo(QString::fromStdString(csv_vec.at(i).url));

                        dl_info.dlStatus = GekkoFyre::DownloadStatus::Stopped;

                        // Make one final check and assign the appropriate values
                        if (!csv_file_dest.isEmpty()) {
                            dl_info.file_loc = csv_file_dest.toStdString();
                        } else if (in.has_column(CSV_FIELD_DEST)) {
                            dl_info.file_loc = csv_vec.at(i).dest;
                        } else {
                            throw std::invalid_argument(tr("No destination specified for: %1")
                                                                .arg(QString::fromStdString(csv_vec.at(i).url))
                                                                .toStdString());
                        }

                        if (!csv_vec.at(i).hash.empty()) {
                            dl_info.hash_type = GekkoFyre::HashType::CannotDetermine;
                            dl_info.hash_val_given = csv_vec.at(i).hash;
                        } else {
                            dl_info.hash_type = GekkoFyre::HashType::None;
                            dl_info.hash_val_given = "";
                        }

                        // Save this more detailed information
                        dl_info.ext_info.content_length = info_ext.content_length;
                        dl_info.ext_info.effective_url = info_ext.effective_url;
                        dl_info.ext_info.response_code = info_ext.response_code;
                        dl_info.ext_info.status_ok = info_ext.status_ok;
                        dl_info.cId = 0;
                        dl_info.timestamp = 0;
                    } else {
                        // The URL does not exist! It's invalid.
                        dl_info.dlStatus = GekkoFyre::DownloadStatus::Invalid;

                        if (!csv_file_dest.isEmpty()) {
                            dl_info.file_loc = csv_file_dest.toStdString();
                        } else if (in.has_column(CSV_FIELD_DEST)) {
                            std::ostringstream file_comp_path;
                            file_comp_path << csv_vec.at(i).dest << fs::path::preferred_separator
                                           << routines->extractFilename(QString::fromStdString(info_ext.effective_url)).toStdString();
                            dl_info.file_loc = file_comp_path.str();
                        }

                        if (!csv_vec.at(i).hash.empty()) {
                            dl_info.hash_type = GekkoFyre::HashType::CannotDetermine;
                            dl_info.hash_val_given = csv_vec.at(i).hash;
                        } else {
                            dl_info.hash_type = GekkoFyre::HashType::None;
                            dl_info.hash_val_given = "";
                        }

                        dl_info.ext_info.content_length = 0;
                        dl_info.ext_info.effective_url = csv_vec.at(i).url;
                        dl_info.ext_info.response_code = info.response_code;
                        dl_info.ext_info.status_ok = false;
                        dl_info.cId = 0;
                        dl_info.timestamp = 0;
                    }

                    // Write the data to an XML file, specifically 'CFG_HISTORY_FILE'
                    routines->writeDownloadItem(dl_info);

                    // Send any new details to the QTableView model/view routines, whereupon 'ui->downloadView' is
                    // updated with the latest data.
                    emit sendDetails(dl_info.ext_info.effective_url, dl_info.ext_info.content_length, 0, 0, 0,
                                     0, dl_info.dlStatus, dl_info.ext_info.effective_url, dl_info.file_loc);
                }

                // Clear the 'temporary' struct-holding std::vector<CsvImport>()
                csv_vec.clear();
                return AddURL::done(QDialog::Accepted);
            }
        } catch (const std::exception &e) {
            QMessageBox::warning(this, tr("Error!"), QString("%1").arg(e.what()), QMessageBox::Ok);
            return AddURL::done(QDialog::Rejected);
        }
    }
}

void AddURL::on_buttonBox_rejected()
{
    this->close();
    return;
}

/**
 * @brief AddURL::on_file_dest_toolButton_clicked determines the destination for mass-imported URLs.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11-15
 */
void AddURL::on_file_dest_toolButton_clicked()
{
    QString dirDest = browseForDir();
    ui->file_dest_lineEdit->setText(dirDest);
    return;
}

/**
 * @brief AddURL::on_file_import_toolButton_clicked processes a mass-import of URLs into FyreDL.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @note <http://doc.qt.io/qt-4.8/qfiledialog.html#getOpenFileName>
 * @date 2016-11-15
 */
void AddURL::on_file_import_toolButton_clicked()
{
    QString csv_dir = QFileDialog::getOpenFileName(this, tr("Choose file to import..."),
                                                   QDir::homePath(),
                                                   tr("Comma Separated Values (*.csv *.txt);;All Files (*.*)"),
                                                   nullptr, QFileDialog::DontResolveSymlinks);
    ui->file_import_lineEdit->setText(csv_dir);
    return;
}

void AddURL::on_url_dest_toolButton_clicked()
{
    QString dirDest = browseForDir();
    ui->url_dest_lineEdit->setText(dirDest);
    return;
}

QString AddURL::browseForDir()
{
    // http://doc.qt.io/qt-5/qfiledialog.html
    QString dirDest =
            QFileDialog::getExistingDirectory(this, tr("Choose destination..."), QDir::homePath(),
                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    return dirDest;
}
