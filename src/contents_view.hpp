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

#ifndef FYREDL_CONTENTS_VIEW_HPP
#define FYREDL_CONTENTS_VIEW_HPP

#include "default_var.hpp"
#include <sstream>
#include <QObject>
#include <QList>
#include <QVariant>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QString>

namespace GekkoFyre {
class GkTreeItem {

public:
    explicit GkTreeItem(const QList<QVariant> &data, GkTreeItem *parentItem = 0);
    ~GkTreeItem();

    void appendChild(GkTreeItem *child);

    GkTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    GkTreeItem *parentItem();

private:
    QList<GkTreeItem*> m_childItems;
    QList<QVariant> m_itemData;
    GkTreeItem *m_parentItem;
};

class GkTreeModel: public QAbstractItemModel {
    Q_OBJECT

public:
    explicit GkTreeModel(const QString &unique_id, QObject *parent = 0);
    ~GkTreeModel();

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

private:
    void setupModelData(const QString &unique_id, GkTreeItem *parent);
    GkTreeItem *rootItem;
};
}

#endif // FYREDL_CONTENTS_VIEW_HPP