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

#ifndef QUEUE_LIST_MODEL
#define QUEUE_LIST_MODEL

#include <QObject>
#include <QAbstractItemModel>
#include <QList>

class PackageListItem;
class QueueListItem;
class QueueListView;

class QueueListModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	QueueListModel(QObject *parent = 0);
	~QueueListModel();

	void setPackages(QList<QueueListItem*>&);

	// Declaration of pure virtual methods to be implemented.
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex& index) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual void sort(int column, Qt::SortOrder order);

	// Some virtual methods to reimplement.

	// Used for the header.
	virtual QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

	QList<QueueListItem*> packages();

	void updateItem(QueueListItem *item, QueueListView *listView);

private:
	QList<QueueListItem*> m_packages;

	static bool packageLessThan(PackageListItem *p1, PackageListItem *p2);
	static bool packageMoreThan(PackageListItem *p1, PackageListItem *p2);
	void sortTree(QList<QueueListItem*> items, Qt::SortOrder order);
};

#endif
