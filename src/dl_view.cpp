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
 */

#include "dl_view.hpp"

downloadModel::downloadModel(QObject *parent) : QAbstractTableModel(parent)
{}

/**
 * @brief downloadModel::downloadModel
 * @author       Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param vector
 * @param parent
 */
downloadModel::downloadModel(QList<std::vector<QString>> vector, QObject *parent) :
    QAbstractTableModel(parent)
{
    vectorList = vector;
}

/**
 * @brief downloadModel::rowCount returns the number of rows within the model.
 * @author       Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param parent
 * @return
 */
int downloadModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return vectorList.size();
}

/**
 * @brief downloadModel::columnCount returns the number of columns within the model.
 * @author       Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param parent
 * @return
 */
int downloadModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 8;
}

/**
 * @brief downloadModel::data returns either a "File name", "File size", "Downloaded",
 * "Progress", "Upload speed", "Download speed", "Status", or "Destination" , based on
 * the contents of the model index supplied.
 * @author      Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param index
 * @param role
 * @return
 */
QVariant downloadModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= vectorList.size() || index.row() < 0) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        std::vector<QString> vector = vectorList.at(index.row());

        for(int i = 0; i < 7; ++i) {
            if (index.column() == i) {
                return vector.at((size_t)i);
            }
        }
    }

    return QVariant();
}

/**
 * @brief downloadModel::headerData contains the label information for each individual column.
 * @author            Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant downloadModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return tr("File name");
        case 1:
            return tr("File size");
        case 2:
            return tr("Downloaded");
        case 3:
            return tr("Progress");
        case 4:
            return tr("Upload speed");
        case 5:
            return tr("Download speed");
        case 6:
            return tr("Status");
        case 7:
            return tr("Destination");
        }
    }

    return QVariant();
}

/**
 * @brief downloadModel::flags
 * @author      Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param index
 * @return
 */
Qt::ItemFlags downloadModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    return QAbstractTableModel::flags(index);
}

/**
 * @brief downloadModel::setData
 * @author      Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param index
 * @param value
 * @param role
 * @return
 */
bool downloadModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // This inserts data into the table, item-by-item and not row-by-row. This means that to fill a row,
    // setData() must be called several times, as there are several columns in total for each row. It is
    // important to emit the dataChanged() signal as it tells all connected views to update their displays.
    if (index.isValid() && role == Qt::DisplayRole) {
        int row = index.row();

        std::vector<QString> v = vectorList.value(row);
        if (index.column() == 0) {
            v.insert((v.begin() + 0), value.toString());
        } else if (index.column() == 1) {
            v.insert((v.begin() + 1), value.toString());
        } else if (index.column() == 2) {
            v.insert((v.begin() + 2), value.toString());
        } else if (index.column() == 3) {
            v.insert((v.begin() + 3), value.toString());
        } else if (index.column() == 4) {
            v.insert((v.begin() + 4), value.toString());
        } else if (index.column() == 5) {
            v.insert((v.begin() + 5), value.toString());
        } else if (index.column() == 6) {
            v.insert((v.begin() + 6), value.toString());
        } else if (index.column() == 7) {
            v.insert((v.begin() + 7), value.toString());
        } else {
            return false;
        }

        vectorList.replace(row, v);
        emit(dataChanged(index, index));

        return true;
    }

    return false;
}

/**
 * @brief downloadModel::insertRows
 * @author         Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param position
 * @param rows
 * @param index
 * @return
 */
bool downloadModel::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, (position + rows - 1));

    for (int row = 0; row < rows; ++row) {
        std::vector<QString> vector;
        vectorList.insert(position, vector);
    }

    endInsertRows();
    return true;
}

/**
 * @brief downloadModel::removeRows
 * @author         Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @param position
 * @param rows
 * @param index
 * @return
 */
bool downloadModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; ++row) {
        vectorList.removeAt(position);
    }

    endRemoveRows();
    return true;
}

/**
 * @brief downloadModel::getList
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @return
 */
QList<std::vector<QString>> downloadModel::getList()
{
    return vectorList;
}
