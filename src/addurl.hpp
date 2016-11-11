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

#ifndef ADDURL_HPP
#define ADDURL_HPP

#include "cmnroutines.hpp"
#include <QDialog>
#include <QString>

namespace Ui {
class AddURL;
}

class AddURL : public QDialog
{
    Q_OBJECT

public:
    explicit AddURL(QWidget *parent = 0);
    ~AddURL();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_file_dest_toolButton_clicked();
    void on_file_import_toolButton_clicked();
    void on_url_dest_toolButton_clicked();

signals:
    void sendDetails(const std::string &fileName, const double &fileSize, const int &downloaded,
                     const double &progress, const int &upSpeed, const int &downSpeed,
                     const GekkoFyre::DownloadStatus &status, const std::string &url,
                     const std::string &destination);

private:
    Ui::AddURL *ui;
    GekkoFyre::CmnRoutines *routines;

    QString browseForDir();
};

#endif // ADDURL_HPP
