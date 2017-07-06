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
 * @file dl_view.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-09
 * @brief The model definition for the object, 'downloadView', within the 'mainwindow.ui' designer file.
 */

#include "dl_view.hpp"
#include "default_var.hpp"

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
    return MN_COL_COUNT;
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

        for(int i = 0; i <= (MN_COL_COUNT - 1); ++i) {
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
        case MN_FILENAME_COL:
            return tr("Item name");
        case MN_FILESIZE_COL:
            return tr("File size");
        case MN_DOWNLOADED_COL:
            return tr("Downloaded");
        case MN_PROGRESS_COL:
            return tr("Progress");
        case MN_UPSPEED_COL:
            return tr("U/load rate");
        case MN_DOWNSPEED_COL:
            return tr("D/load rate");
        case MN_SEEDERS_COL:
            return tr("Seeders");
        case MN_LEECHERS_COL:
            return tr("Leechers");
        case MN_STATUS_COL:
            return tr("Status");
        case MN_DESTINATION_COL:
            return tr("Dest.");
        case MN_URL_COL:
            return tr("URL");
        case MN_HIDDEN_UNIQUE_ID:
            return tr("Hidden-Unique-ID");
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
        if (index.column() == MN_FILENAME_COL) {
            v.insert((v.begin() + MN_FILENAME_COL), value.toString());
        } else if (index.column() == MN_FILESIZE_COL) {
            v.insert((v.begin() + MN_FILESIZE_COL), value.toString());
        } else if (index.column() == MN_DOWNLOADED_COL) {
            v.insert((v.begin() + MN_DOWNLOADED_COL), value.toString());
        } else if (index.column() == MN_PROGRESS_COL) {
            v.insert((v.begin() + MN_PROGRESS_COL), value.toString());
        } else if (index.column() == MN_UPSPEED_COL) {
            v.insert((v.begin() + MN_UPSPEED_COL), value.toString());
        } else if (index.column() == MN_DOWNSPEED_COL) {
            v.insert((v.begin() + MN_DOWNSPEED_COL), value.toString());
        } else if (index.column() == MN_SEEDERS_COL) {
            v.insert((v.begin() + MN_SEEDERS_COL), value.toString());
        } else if (index.column() == MN_LEECHERS_COL) {
            v.insert((v.begin() + MN_LEECHERS_COL), value.toString());
        } else if (index.column() == MN_STATUS_COL) {
            v.insert((v.begin() + MN_STATUS_COL), value.toString());
        } else if (index.column() == MN_DESTINATION_COL) {
            v.insert((v.begin() + MN_DESTINATION_COL), value.toString());
        } else if (index.column() == MN_URL_COL) {
            v.insert((v.begin() + MN_URL_COL), value.toString());
        } else if (index.column() == MN_HIDDEN_UNIQUE_ID) {
            v.insert((v.begin() + MN_HIDDEN_UNIQUE_ID), value.toString());
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
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-10
 * @param position
 * @param rows
 * @param index
 * @return
 */
bool downloadModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, (position + rows - 1));

    for (int row = 0; row < rows; ++row) {
        vectorList.removeAt(position);
    }

    endRemoveRows();
    return true;
}

/**
 * @brief downloadModel::updateCol updates the column, within a row, to visually reflect the new data given by 'value'.
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-11
 * @param index The QModelIndex to be updated, specifically, the row and column.
 * @param value The new data to be reflected by the update.
 * @param col Used for verification purposes. Kind of redundant.
 * @return Whether the operation was a success or not.
 */
bool downloadModel::updateCol(const QModelIndex &index, const QVariant &value, const int &col)
{
    if (index.isValid()) {
        if (col >= 0) {
            int row = index.row();

            std::vector<QString> v = vectorList.value(row);
            if (index.column() == col) {
                v.at((size_t)col) = value.toString();
            }

            vectorList.replace(row, v);
            emit(dataChanged(index, index));
            return true;
        } else {
            throw std::invalid_argument(tr("'col' is less than zero! Please restart the application "
                                           "and try again.").toStdString());
        }
    }

    return false;
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

/**
 * @brief downloadDelegate::downloadDelegate
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-06
 * @param parent
 */
downloadDelegate::downloadDelegate(QObject *parent) : QAbstractItemDelegate(parent)
{}

downloadDelegate::~downloadDelegate()
{}

/**
 * @brief downloadDelegate::paint
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-06
 * @note <http://doc.qt.io/qt-5/qabstractitemdelegate.html>
 * @param painter
 * @param option
 * @param index
 */
void downloadDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == MN_PROGRESS_COL) {
        int progress = index.data().toInt();

        QStyleOptionProgressBar progressBarOption;
        progressBarOption.rect = option.rect;
        progressBarOption.minimum = 0;
        progressBarOption.maximum = 100;
        progressBarOption.progress = progress;
        progressBarOption.text = QString::number(progress) + "%";
        progressBarOption.textVisible = true;


        QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
        return;
    } else {
        QStyleOptionViewItem viewItemOption;
        viewItemOption.rect = option.rect;
        viewItemOption.font = QFont(option.font, painter->device());
        viewItemOption.palette = option.palette;

        painter->drawText(viewItemOption.rect, index.data().toString());

        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &viewItemOption, painter);
        return;
    }
}

/**
 * @brief downloadDelegate::sizeHint
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2017-07-06
 * @param option
 * @param index
 * @return
 */
QSize downloadDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize();
}
