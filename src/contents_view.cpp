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
 * @file contents_view.cpp
 * @author Phobos Aryn'dythyrn D'thorga <phobos.gekko@gmail.com>
 * @date 2016-12-13
 * @note <http://doc.qt.io/qt-5/qtwidgets-itemviews-simpledommodel-example.html>
 *       <http://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html>
 * @brief The model definition for the object, 'contentsView', within the 'mainwindow.ui' designer file.
 */

#include "contents_view.hpp"
#include "cmnroutines.hpp"
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <QHash>

namespace sys = boost::system;
namespace fs = boost::filesystem;

/**
 * @brief GekkoFyre::GkTreeItem::GkTreeItem
 * @param data
 * @param parentItem
 */
GekkoFyre::GkTreeItem::GkTreeItem(const QList<QVariant> &data, GekkoFyre::GkTreeItem *parentItem)
{
    m_parentItem = parentItem;
    m_itemData = data;
}

/**
 * @brief GekkoFyre::GkTreeItem::~GkTreeItem
 */
GekkoFyre::GkTreeItem::~GkTreeItem()
{
    qDeleteAll(m_childItems);
}

/**
 * @brief GekkoFyre::GkTreeItem::appendChild adds data when the model is first constructed and is not used during normal use.
 * @param child
 */
void GekkoFyre::GkTreeItem::appendChild(GekkoFyre::GkTreeItem *child)
{
    m_childItems.append(child);
}

/**
 * @brief GekkoFyre::GkTreeItem::child allows the model to obtain information about any child items.
 * @param row
 * @return
 */
GekkoFyre::GkTreeItem *GekkoFyre::GkTreeItem::child(int row)
{
    return m_childItems.value(row);
}

/**
 * @brief GekkoFyre::GkTreeItem::childCount allows the model to obtain information about any child items.
 * @return
 */
int GekkoFyre::GkTreeItem::childCount() const
{
    return m_childItems.count();
}

/**
 * @brief GekkoFyre::GkTreeItem::columnCount provides information about the number of columns associated with the item,
 * and the data in each column can be obtained with the data() function.
 * @return
 */
int GekkoFyre::GkTreeItem::columnCount() const
{
    return m_itemData.count();
}

/**
 * @brief GekkoFyre::GkTreeItem::data provides the data for the given item(s) on a per-column basis.
 * @param column
 * @return
 */
QVariant GekkoFyre::GkTreeItem::data(int column) const
{
    return m_itemData.value(column);
}

/**
 * @brief GekkoFyre::GkTreeItem::row is used to obtain the item's row number.
 * @return
 */
int GekkoFyre::GkTreeItem::row() const
{
    if (m_parentItem) {
        return m_parentItem->m_childItems.indexOf(const_cast<GkTreeItem*>(this));
    }

    return 0;
}

/**
 * @brief GekkoFyre::GkTreeItem::parentItem is used to obtain the item's 'parent item'.
 * @return
 */
GekkoFyre::GkTreeItem *GekkoFyre::GkTreeItem::parentItem()
{
    return m_parentItem;
}

/**
 * @brief GekkoFyre::GkTreeModel::GkTreeModel
 * @param data
 * @param parent
 */
GekkoFyre::GkTreeModel::GkTreeModel(const QString &unique_id, QObject *parent): QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << tr("Title");
    rootItem = new GkTreeItem(rootData);
    setupModelData(unique_id, rootItem);
}

/**
 * @brief GekkoFyre::GkTreeModel::~GkTreeModel ensures that the root item and all of its descendents are deleted when
 * the model is destroyed.
 */
GekkoFyre::GkTreeModel::~GkTreeModel()
{
    delete rootItem;
}

/**
 * @brief GekkoFyre::GkTreeModel::data
 * @param index
 * @param role
 * @return
 */
QVariant GekkoFyre::GkTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    GkTreeItem *item = static_cast<GkTreeItem*>(index.internalPointer());
    return item->data(index.column());
}

/**
 * @brief GekkoFyre::GkTreeModel::flags ensures that the view knows the model is "read-only".
 * @param index
 * @return
 */
Qt::ItemFlags GekkoFyre::GkTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return 0;
    }

    return QAbstractItemModel::flags(index);
}

/**
 * @brief GekkoFyre::GkTreeModel::headerData returns data that we conveniently stored in the root item.
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant GekkoFyre::GkTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return rootItem->data(section);
    }

    return QVariant();
}

/**
 * @brief GekkoFyre::GkTreeModel::index provides indexes for views and delegates to use when accessing data. Indexes
 * are created for other components when they are referenced by their row and column numbers, and their parent model
 * index.
 * @param row
 * @param column
 * @param parent
 * @return
 */
QModelIndex GekkoFyre::GkTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    GkTreeItem *parentItem;

    if (!parent.isValid()) {
        parentItem = rootItem;
    } else {
        parentItem = static_cast<GkTreeItem*>(parent.internalPointer());
    }

    GkTreeItem *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    } else {
        return QModelIndex();
    }
}

/**
 * @brief GekkoFyre::GkTreeModel::parent
 * @param index
 * @return
 */
QModelIndex GekkoFyre::GkTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    GkTreeItem *childItem = static_cast<GkTreeItem*>(index.internalPointer());
    GkTreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

/**
 * @brief GekkoFyre::GkTreeModel::rowCount
 * @param parent
 * @return
 */
int GekkoFyre::GkTreeModel::rowCount(const QModelIndex &parent) const
{
    GkTreeItem *parentItem;
    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {
        parentItem = rootItem;
    } else {
        parentItem = static_cast<GkTreeItem*>(parent.internalPointer());
    }

    return parentItem->childCount();
}

/**
 * @brief GekkoFyre::GkTreeModel::columnCount
 * @param parent
 * @return
 */
int GekkoFyre::GkTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return static_cast<GkTreeItem*>(parent.internalPointer())->columnCount();
    } else {
        return rootItem->columnCount();
    }
}

/**
 * @brief GekkoFyre::GkTreeModel::setupModelData populates the model's internal data structure.
 * @note <http://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-treemodel-cpp.html>
 *       <http://www.boost.org/doc/libs/1_62_0/libs/filesystem/doc/reference.html#class-path>
 * @param lines provides the data to be read and dissected.
 * @param parent is the parent item to all children in the QTreeView.
 */
void GekkoFyre::GkTreeModel::setupModelData(const QString &unique_id, GekkoFyre::GkTreeItem *parent)
{
    /**
 * 1. Load XML history file into memory.
 * 2. Separate out the individual elements of 'GekkoFyre::GkTorrent::TorrentFile'.
 * 3. Break down each file-path by individual directory and then finally, file-name.
 * 4. Calculate the 'column number' for each individual directory and ultimately, file.
 * 5. Add all this data to a 'std::ostringstream' which is then processed by the text interpreter below
 * the routines explained above.
 */

    /**
     * #######################
     * # Directory processor #
     * #######################
     */

    std::vector<GekkoFyre::GkTorrent::TorrentInfo> gk_ti = GekkoFyre::CmnRoutines::readTorrentInfo(false);
    std::ostringstream oss_data;
    for (size_t i = 0; i < gk_ti.size(); ++i) {
        if (gk_ti.at(i).unique_id == unique_id.toStdString()) {
            int column = 0;
            QList<GekkoFyre::GkTorrent::ContentsView> columnData; // <col, data>
            std::vector<GekkoFyre::GkTorrent::TorrentFile> gk_tf_vec = gk_ti.at(i).files_vec;

            for (size_t j = 0; j < gk_tf_vec.size(); ++j) {
                GekkoFyre::GkTorrent::TorrentFile gk_tf_element = gk_tf_vec.at(j);

                fs::path boost_path(gk_tf_element.file_path);
                for (auto &indice: boost_path) {
                    GekkoFyre::GkTorrent::ContentsView cv;
                    ++column;
                    cv.column = column;
                    cv.name = QString::fromStdString(indice.string());
                    cv.root = QString::fromStdString(boost_path.parent_path().string());
                    columnData.push_back(cv);
                }

                column = 0;
            }

            /**
             * 1. Check if the root-directory for a path has already been added and if not, add it.
             *      1a. You can detect a root-directory by its column number being '1'.
             * 2. Add the files contained under that directory and then see if there's any sub-directories, whilst
             * inserting the files into the list of already added children.
             *      2a. If a sub-directory is detected, add it with the appropriate tabbing and insert it into the list
             *      of already added children.
             *      2b. Go back to (2).
             *      2c. When there's no more sub-directories, go to (3).
             * 3. Check to see if there are any other root-directories left and if so, go back to (1).
             *      3a. When there are no more root-directories remaining, break out of the loops and proceed
             *      to the text processor. I've written the code in this fashion because I literally could not
             *      understand the example code in its entirety. You, dear reader, are invited to improve upon
             *      this.
             */

            int repetition = 0;
            QList<QString> root_alreadyProc;
            QList<QString> child_alreadyProc;
            QHash<QString, QString> child_toBeAdded; // <key, value>
            QString curr_root;
            gk_repeat:;
            for (int j = 0; j < columnData.size(); ++j) {
                QString root = columnData.at(j).root.toString();
                QString child = columnData.at(j).name.toString();
                QString leaf = QDir(root).dirName();
                if (columnData.at(j).column == (1 + repetition)) {
                    if (!root_alreadyProc.contains(root)) {
                        for (int k = 0; k < ((columnData.at(j).column - 1) + repetition); ++k) {
                            oss_data << "    ";
                        }

                        oss_data << leaf.toStdString() << std::endl;

                        root_alreadyProc.push_back(root);
                        child_alreadyProc.push_back(root);
                        curr_root = root;
                        break;
                    }
                }
            }

            if (!curr_root.isEmpty()) {
                for (int j = 0; j < columnData.size(); ++j) {
                    QString child = columnData.at(j).name.toString();
                    if (curr_root == columnData.at(j).root) {
                        QString root = columnData.at(j).root.toString();
                        if (!child_alreadyProc.contains(child)) {
                            child_toBeAdded.insertMulti(root, child);
                            child_alreadyProc.push_back(child);
                        }
                    }
                }
            }

            if (!curr_root.isEmpty()) {
                QList<QString> children = child_toBeAdded.values(curr_root);
                for (int j = 0; j < children.size(); ++j) {
                    fs::path boost_path(children.at(j).toStdString());
                    for (int k = 0; k < columnData.size(); ++k) {
                        if (columnData.at(k).root == curr_root && children.at(j) == columnData.at(k).name &&
                            boost_path.has_extension()) {
                            for (int o = 1; o <= ((columnData.at(k).column - 1) + repetition); ++o) {
                                oss_data << "    ";
                            }
                        }
                    }

                    if (boost_path.has_extension()) {
                        oss_data << children.at(j).toStdString() << std::endl;
                    }
                }
            }

            int roots_left = 0;
            for (int j = 0; j < columnData.size(); ++j) {
                QString root = columnData.at(j).root.toString();
                if (columnData.at(j).column > (repetition + 1)) {
                    if (!root_alreadyProc.contains(root)) {
                        ++roots_left;
                    }
                }
            }

            if (roots_left > 0) {
                ++repetition;
                goto gk_repeat;
            }

            break;
        }
    }

    /**
     * ####################
     * # Text interpreter #
     * ####################
     */

    QList<GkTreeItem *> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    QString data = QString::fromStdString(oss_data.str());
    QStringList lines = data.split(QString("\n"));
    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].at(position) != ' ') {
                break;
            }

            position++;
        }

        QString lineData = lines[number].mid(position).trimmed();

        if (!lineData.isEmpty()) {
            // Read the column data from the rest of the line.
            QStringList columnStrings = lineData.split("\t", QString::SkipEmptyParts);
            QList<QVariant> columnData;
            for (int column = 0; column < columnStrings.count(); ++column) {
                columnData << columnStrings[column];
            }

            if (position > indentations.last()) {
                // The last child of the current parent is now the new parent unless the current parent has no children.
                if (parents.last()->childCount() > 0) {
                    parents << parents.last()->child(parents.last()->childCount() - 1);
                    indentations << position;
                }
            } else {
                while (position < indentations.last() && parents.count() > 0) {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            parents.last()->appendChild(new GkTreeItem(columnData, parents.last()));
        }

        ++number;
    }
}
