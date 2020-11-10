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

#include <QHeaderView>
#include <QMouseEvent>

#include "common.h"
#include "queuelistview.h"
#include "queuelistmodel.h"
#include "queuelistitem.h"
#include "queuelistdelegate.h"

const int diffTime = 10;

QueueListView::QueueListView(QWidget *parent)
 : QTreeView(parent)
{
	QueueListModel *m = new QueueListModel(this);
	setModel(m);
	setItemDelegateForColumn(5, new QueueListDelegate(this));
	QHeaderView *hh = header();
	hh->setStretchLastSection(true);
	hh->setResizeMode(QHeaderView::ResizeToContents);
	hh->setResizeMode(5, QHeaderView::Stretch);

	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setAlternatingRowColors(true);
}

QueueListView::~QueueListView()
{}

void QueueListView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	foreach(const QModelIndex index, deselected.indexes())
	{
		if (index.column() == 0 && index.data().canConvert(QVariant::String))
		{
			for(int j = 0; j < m_selectedPackages.count(); j++)
				if (m_selectedPackages[j]->name() == index.data().toString())
					m_selectedPackages.removeAt(j);
		}
	}
	foreach(const QModelIndex index, selected.indexes())
	{
		if (index.column() == 0 && index.data().canConvert(QVariant::String))
		{
			QueueListItem *item = static_cast<QueueListItem*>(index.internalPointer());
			m_selectedPackages << item;
		}
	}

	emit selectionChangedSignal();
}

QList<QueueListItem*> QueueListView::selectedPackages() const
{
	return m_selectedPackages;
}

QueueListItem* QueueListView::currentPackage()
{
	return static_cast<QueueListItem*>(currentIndex().internalPointer());
}

QueueListItem* QueueListView::packageItemById(const QString& id)
{
	//QMap will insert a default-constructed value if it doesn't already exist
	if (m_packageIndex.contains(id)) {
		return m_packageIndex[id];
	} else {
		return NULL;
	}
}

QStringList QueueListView::selectedPackagesByIds()
{
	QStringList ret;
	foreach(QueueListItem *item, m_selectedPackages)
		ret << item->id();

	return ret;
}

/**
 * Populate queue with packages from db.
 * @param bool hasCheckedQueue: whether packageList is the result of emerge pretend or just added by user manually.
 */
void QueueListView::insertPackageList( bool hasCheckedQueue )
{
	DEBUG_LINE_INFO;
	QueueListItem* item;
	QList<QueueListItem*> items;
	QList<QueueListItem*> orphans;
	m_sumSize = 0;

	reset();//FIXME: I don't know why this is labeled fixme
	m_selectedPackages.clear();
	m_packageIndex.clear();

	// Get list of update packages with info
	const QStringList packageList = KurooDBSingleton::Instance()->allQueuePackages();
	//int packageCount = packageList.size() / 7;
	QListIterator<QString> it( packageList );
	while( it.hasNext() ) {
		QString id = it.next();
		QString category = it.next();
		QString name = it.next();
		QString status = it.next();
		QString idDepend = it.next();
		QString size = it.next();
		QString version = it.next();

		// Get package emerge duration from statistics
		int duration = HistorySingleton::Instance()->packageTime( category + "/" + name ).toInt() + diffTime;

		// If version get size
		if ( size.isEmpty() || size == "0" )
			size = KurooDBSingleton::Instance()->versionSize( id, version );
		else
			size = formatSize( size );

		item = new QueueListItem(name, id, category, status.toInt(), duration, this);

		if ( !idDepend.isEmpty() && idDepend != "0" ) {
			item->setParentId(idDepend);

			if (m_packageIndex.contains(idDepend))
				m_packageIndex[idDepend]->appendChild(item);
			else
				orphans << item;
		}

		for(int i = 0; i < orphans.count(); i++)
		{
			QueueListItem *orphan = orphans.at(i);
			if (orphan->parentId() == item->id())
				item->appendChild(orphans.takeAt(i));
		}

		// Add version to be emerged
		if ( version.isEmpty() )
			item->setVersion(i18n("na") );
		else
			item->setVersion(version);

		// Add emerge duration
		if ( duration == diffTime )
			item->setDuration(-1);
		else
			item->setDuration(duration);

		// Add download size = tarball size
		if ( size.isEmpty() )
			item->setSize(i18n("na"));
		else {
			item->setSize(size);
			addSize(size);
		}

		item->setPretended( hasCheckedQueue );
		item->setIsComplete(QueueSingleton::Instance()->hasCompleted(id));

		indexPackage( id, item );

		// Inform all other listviews that this package is in queue
		QueueSingleton::Instance()->insertInCache( id );

		//Fix bug #3163827 order of queue items
		items.prepend( item );
	}

	// Cannot have current changed for only one package so emit manually
	/*if ( packageCount == 1 )
		emit currentChanged( 0 );*/

	dynamic_cast<QueueListModel*>(model())->setPackages(items);
	SignalistSingleton::Instance()->packageQueueChanged();
}

/**
 * Get total emerge duration in format hh:mm:ss.
 @return totalDuration
 */
long QueueListView::totalDuration()
{
	long totalSeconds = 0;
	QList<QueueListItem*> packages = dynamic_cast<QueueListModel*>(model())->packages();
	foreach(QueueListItem *item, packages)
	{
		foreach(QueueListItem *child, item->children())
		{
			if (!child->isComplete()) {
				totalSeconds += child->remainingDuration();
			}
		}
		if (!item->isComplete())
		{
			totalSeconds += item->remainingDuration();
		}
	}
	return totalSeconds;
}

/**
 * All packages in listview by name - no children
 * @return packageList
 */
const QStringList QueueListView::allPackagesNoChildren()
{
	QStringList packageList;
	QList<QueueListItem*> packages = dynamic_cast<QueueListModel*>(model())->packages();
	foreach(QueueListItem *item, packages)
	{
		if (item->parentItem() == NULL)
		{
			packageList << item->category() + "/" + item->name();
		}
	}

	return packageList;
}

const QStringList QueueListView::allId() const
{
	QStringList packageList;
	foreach(QString id, m_packageIndex.keys())
	{
		packageList << id;
	}

	return packageList;
}

QList<QueueListItem*> QueueListView::allPackages() const
{
	return m_packageIndex.values();
}

void QueueListView::nextPackage( const bool& isPrevious )
{
	if ( isVisible() ) {
		QModelIndex item;
		if ( isPrevious )
			item = moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);
		else
			item = moveCursor(QAbstractItemView::MoveDown, Qt::NoModifier);

		if (item.isValid()) {
			scrollTo(item);
			setCurrentIndex(item);
		}
	}
}

/**
 * Format package size nicely
 * @param size
 * @return total		as "xxx kB"
 */
const QString QueueListView::formatSize( const QString& sizeString )
{
	KLocale *loc = KLocale::global();
	QString total;
	QString tmp = sizeString;
	int size = tmp.remove(",").toInt();

	if ( size == 0 )
		total = "0 kB ";
	else
		total = loc->formatNumber( (double)size, 0 ) + " KiB ";

	return total;
}

/**
 * Add this package size to total.
 * @param size
 */
void QueueListView::addSize( const QString& size )
{
	QString packageSize( size );
	packageSize = packageSize.remove( QRegExp("\\D") );
	m_sumSize += packageSize.toInt() * 1024;
}

/**
 * Register package in index and check if in the queue.
 * @param id
 * @param item
 */
void QueueListView::indexPackage( const QString& id, QueueListItem *item )
{
	if ( !id.isEmpty() ) {
		m_packageIndex.insert( id, item );
		m_packageIndex[id]->setPackageIndex( m_packageIndex.count() );
	}
}

void QueueListView::mouseDoubleClickEvent(QMouseEvent *event)
{
	QModelIndex index = indexAt(event->pos());
	if (!index.isValid())
		return;

	QueueListItem *item = static_cast<QueueListItem*>(index.internalPointer());
	if (!item)
		return;

	emit itemDoubleClicked(item);
}

void QueueListView::hasStarted(const QString& id)
{
	slotPackageStart(id);
}

void QueueListView::slotPackageStart(const QString& id)
{
	m_currentEmergingId = id;
	if (m_packageIndex.contains(id))
		m_packageIndex[id]->setHasStarted(true);
}

void QueueListView::slotPackageProgress()
{
	if (m_currentEmergingId != "" && m_packageIndex.contains(m_currentEmergingId))
		m_packageIndex[m_currentEmergingId]->oneStep();
}

void QueueListView::slotPackageComplete(const QString& id)
{
	if (id == m_currentEmergingId)
	{
		if (m_packageIndex.contains(id))
			m_packageIndex[id]->setIsComplete(true);
		m_currentEmergingId = "";
	}
}

