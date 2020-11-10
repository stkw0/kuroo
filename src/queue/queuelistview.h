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

#ifndef QUEUE_LIST_VIEW_H
#define QUEUE_LIST_VIEW_H

#include <QTreeView>
#include <QStringList>
#include <QMouseEvent>

class QueueListItem;

class QueueListView : public QTreeView
{
	Q_OBJECT
public:
	QueueListView(QWidget *parent = 0);
	~QueueListView();

	QList<QueueListItem*>			selectedPackages() const;
	QueueListItem*					currentPackage();
	QueueListItem*					packageItemById(const QString& id);
	QStringList						selectedPackagesByIds();

	void							insertPackageList( bool hasCheckedQueue );
	long							totalDuration();
	const QStringList				allPackagesNoChildren();

	const QStringList				allId() const;
	const QString					count() const { return QString::number( m_packageIndex.count() ); }
	void							nextPackage( const bool& isPrevious );
	void							hasStarted(const QString&);
	QList<QueueListItem*>			allPackages() const;

public slots:
	void							slotPackageStart(const QString&);
	void							slotPackageProgress();
	void							slotPackageComplete(const QString&);

signals:
	void							selectionChangedSignal();
	void							itemDoubleClicked(QueueListItem*);

protected:
	void							selectionChanged(const QItemSelection&, const QItemSelection&);
	void							mouseDoubleClickEvent(QMouseEvent*);

private:
	QList<QueueListItem*>			m_selectedPackages;
	QMap<QString, QueueListItem*>	m_packageIndex;

	int								m_sumSize;
	const QString					formatSize( const QString& sizeString );
	void							addSize( const QString& size );
	void							indexPackage( const QString& id, QueueListItem *item );
	QString							m_currentEmergingId;

};
#endif
