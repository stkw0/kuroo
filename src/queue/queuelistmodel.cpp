/***************************************************************************
 *	Copyright (C) 2010 by cazou88											*
 *	cazou88@users.sourceforge.net											*
 *																			*
 *	This program is free software; you can redistribute it and/or modify	*
 *	it under the terms of the GNU General Public License as published by	*
 *	the Free Software Foundation; either version 2 of the License, or		*
 *	(at your option) any later version.										*
 *																			*
 *	This program is distributed in the hope that it will be useful,			*
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of			*
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the			*
 *	GNU General Public License for more details.							*
 *																			*
 *	You should have received a copy of the GNU General Public License		*
 *	along with this program; if not, write to the							*
 *	Free Software Foundation, Inc.,											*
 *	59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.				*
 ***************************************************************************/

#include <QIcon>

#include "common.h"
#include "queuelistmodel.h"
#include "queuelistitem.h"
#include "queuelistview.h"

QueueListModel::QueueListModel(QObject *parent)
 : QAbstractItemModel(parent)
{
}

QueueListModel::~QueueListModel()
{
	while (!m_packages.isEmpty())
		delete m_packages.takeLast();
}

QList<QueueListItem*> QueueListModel::packages()
{
	return m_packages;
}

void QueueListModel::setPackages(QList<QueueListItem*>& packages)
{
	DEBUG_LINE_INFO;
	// Remove all rows
	if (rowCount() > 0)
	{
		beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
		//Fix bug #3163827 order of queue items
		//I think this bug has to do with deleting the parent before deleting the child items,
		//so if I get the order right it should go away, currently that means changing the order randomly
		while (!m_packages.isEmpty())
			delete m_packages.takeFirst();//.takeLast();
		endRemoveRows();
	}

	if (packages.count() <= 0)
		return;

	// Set packages
	//NOTE: This only puts top-level packages in the QList
	QList<QueueListItem*> tmp;
	foreach(QueueListItem* item, packages)
	{
		if (item->parentItem() == NULL)
			tmp << item;
	}
	beginInsertRows(QModelIndex(), 0, tmp.count());
	m_packages = tmp;
	endInsertRows();
}

// Implementation of pure virtual methods to be implemented.
int QueueListModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	if (parent.column() > 0)
		return 0;
	return 6;
}

QVariant QueueListModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	QueueListItem *p = static_cast<QueueListItem*>(index.internalPointer());
	switch(index.column())
	{
	case 0:
		if (role == Qt::DisplayRole)
			return QVariant(QString("%1 (%2)").arg(p->name()).arg(p->category()));
		if (role == Qt::DecorationRole && p->status() & PACKAGE_AVAILABLE)
			return QVariant(QIcon::fromTheme(QStringLiteral("kuroo_package")));
		else if (role == Qt::DecorationRole)
			return QVariant(QIcon::fromTheme(QStringLiteral("kuroo_stable")));

		break;
	case 1:
		if (role == Qt::DecorationRole && p->isInWorld())
			return QVariant(QIcon::fromTheme(QStringLiteral("kuroo_world")));

		break;
	case 2:
		if (role == Qt::DisplayRole)
			return QVariant(p->version());

		break;
	case 3:
		if (role == Qt::DisplayRole)
		{
			if (p->duration() < 0)
				return QVariant(i18n("na"));
			else
				return QVariant(formatTime(p->duration()));
		}

		break;
	case 4:
		if (role == Qt::DisplayRole)
			return QVariant(p->size());

		break;
	case 5:
		if (role == Qt::DisplayRole)
			return QVariant(QString());

		break;
	default:
		qDebug() << "Error: invalid column!";
		break;
	}

	return QVariant();
}

QModelIndex QueueListModel::index(int row, int column, const QModelIndex& parent) const
{
	if (row < 0)
		return QModelIndex();

	if (!parent.isValid() )
	{
		if (row < m_packages.count()) {
			return createIndex(row, column, m_packages.at(row));
		} else {
			return QModelIndex();
		}
	}

	QueueListItem *item = static_cast<QueueListItem*>(parent.internalPointer());
	if (item && row < item->children().count())
	{
		item = item->children().at(row);
		return createIndex(row, column, item);
	}
	else
		return QModelIndex();
}

bool QueueListModel::hasChildren(const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		//if the parent is valid then check it's children count
		QueueListItem *item = static_cast<QueueListItem*>(parent.internalPointer());
		if (item)
			return (item->children().count() > 0);
		else
			return false;
	}
	else
		//otherwise it's a root, so return true
		return true;
}

QModelIndex QueueListModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return QModelIndex();
	}
	QueueListItem *item = static_cast<QueueListItem*>(index.internalPointer());
	if (item == NULL || item->parentItem() == NULL)
		return QModelIndex();

	return createIndex(item->parentItem()->packageIndex(), 0, item->parentItem());
	//This was the old code, which occassionally crashed on item->parentItem()->parentItem()->children()
	int i = 0;
	QList<QueueListItem*> parents;
	if (item->parentItem()->parentItem() == NULL)
		parents = m_packages;
	else
		parents = item->parentItem()->parentItem()->children();

	foreach(QueueListItem *p, parents)
	{
		i++;
		if (p == item->parentItem())
			break;
	}
	return createIndex(i, 0, item->parentItem());
}

int QueueListModel::rowCount(const QModelIndex& parent) const
{
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		return m_packages.count();

	QueueListItem *item = static_cast<QueueListItem*>(parent.internalPointer());
	return item->children().count();
}

QVariant QueueListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return QVariant();

	switch (section)
	{
	case 0:
		if (role == Qt::DisplayRole)
			return QVariant(i18n("Package"));
		break;
	case 1:
		if (role == Qt::DecorationRole)
			return QVariant(QIcon::fromTheme(QStringLiteral("kuroo_world_column")));
		break;
	case 2:
		if (role == Qt::DisplayRole)
			return QVariant(i18n("Version"));
		break;
	case 3:
		if (role == Qt::DisplayRole)
			return QVariant("Duration");
		break;
	case 4:
		if (role == Qt::DisplayRole)
			return QVariant("Size");
		break;
	case 5:
		if (role == Qt::DisplayRole)
			return QVariant("Progress");
		break;
	default:
		qDebug() << "Error: invalid column!";
		break;
	}

	return QVariant();
}

void QueueListModel::updateItem(QueueListItem *item, QueueListView *listView)
{
	QModelIndexList mList = persistentIndexList();
	foreach(QModelIndex index, mList)
	{
		if (index.isValid() && index.internalPointer() == item && index.column() == 5)
		{
			listView->update(index);
			return;
		}
	}

}

bool QueueListModel::packageLessThan(PackageListItem *p1, PackageListItem *p2)
{
	return p1->name() < p2->name();
}

bool QueueListModel::packageMoreThan(PackageListItem *p1, PackageListItem *p2)
{
	return p1->name() > p2->name();
}

void QueueListModel::sort(int column, Qt::SortOrder order)
{
	if (column != 0)
		return;

	sortTree(m_packages, order);
}

void QueueListModel::sortTree(QList<QueueListItem*> items, Qt::SortOrder order)
{
	foreach(QueueListItem *item, items)
	{
		sortTree(item->children(), order);
	}

	if (order == Qt::AscendingOrder)
		qSort(items.begin(), items.end(), packageLessThan);
	else
		qSort(items.begin(), items.end(), packageMoreThan);

}
