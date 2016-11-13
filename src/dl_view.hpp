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
 * @file cmnroutines.hpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief The model definition for the object, 'downloadView', within the 'mainwindow.ui' designer file.
 * @note <https://forum.qt.io/topic/56299/how-to-draw-and-correctly-animate-progress-bar-in-qtableview/8>
 *       <http://doc.qt.io/qt-5/qabstractitemdelegate.html>
 *       <http://stackoverflow.com/questions/3656306/how-to-put-an-image-and-a-qprogressbar-inside-a-qtableview>
 */

#ifndef DL_VIEW_HPP
#define DL_VIEW_HPP

#include <QObject>
#include <QList>
#include <QString>
#include <QAbstractTableModel>
#include <QVariant>
#include <vector>

// http://doc.qt.io/qt-5/qtwidgets-itemviews-addressbook-example.html
class downloadModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    downloadModel(QObject *parent = 0);
    downloadModel(QList<std::vector<QString>> vector, QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) Q_DECL_OVERRIDE;
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) Q_DECL_OVERRIDE;
    bool updateCol(const QModelIndex &index, const QVariant &value, const int &col);

    QList<std::vector<QString>> getList();

private:
    // http://stackoverflow.com/questions/23870396/qt-list-clear-does-it-destroy-the-objects
    QList<std::vector<QString>> vectorList;
};

#endif // DL_VIEW_HPP
