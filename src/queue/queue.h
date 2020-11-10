/***************************************************************************
*	Copyright (C) 2004 by karye												*
*	karye@users.sourceforge.net												*
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

#ifndef QUEUE_H
#define QUEUE_H

#include <QMap>
#include <QObject>

#include "package.h"

class QTimer;

/**
 * @class Queue
 * @short Object for packages to be emerged = installation queue.
 */
class Queue : public QObject
{
Q_OBJECT

public:
	Queue( QObject *m_parent = 0 );
	~Queue();

	void					init( QObject *parent = 0 );
	void					emergePackageStart( const QString& package/*, int order, int total */);
	void					emergePackageComplete( const QString& package/*, int order, int total */);
	bool					isQueueBusy();
	void					clearCache();
	int						size();
	void					insertInCache( const QString& id );
	void					deleteFromCache( const QString& id );
	bool					isQueued( const QString& id );
	void					reset();
	void					refresh( bool hasCheckedQueue );
	void					removePackageIdList( const QStringList& packageIdList );
	void					addPackageList( const EmergePackageList &packageList );
	void					addPackageIdList( const QStringList& packageIdList );
	void					installQueue( const QStringList& packageList );
	void					setRemoveInstalled( bool removeInstalled );
	void					pauseEmerge();
	void					unpauseEmerge();
	bool					hasCompleted(const QString& id);

public slots:
	void					slotOneStep();
	void					slotClearQueue();

signals:
	void					signalQueueChanged( bool hasCheckedQueue );
	void					signalPackageAdvance();
	void					signalPackageStart( const QString& id );
	void					signalPackageComplete( const QString& id );

private:
	QObject					*m_parent;

	// Queue is busy emerging
	bool					m_isQueueBusy;

	// Cache containing packages in Queue
	QMap<QString, bool>		m_queueCache;

	// Timer for installation progress
	QTimer*					m_internalTimer;

	// Clear queue after installation?
	bool					m_removeInstalled;
};

#endif
