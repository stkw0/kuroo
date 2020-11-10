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

#include <QMap>
#include <QTimer>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/JobPointer>
#include <ThreadWeaver/Thread>
#include <ThreadWeaver/ThreadWeaver>

#include "common.h"

/**
* @class AddQueuePackageIdListJob
* @short Thread for adding packages to the queue in db.
*/
class AddQueuePackageIdListJob : public ThreadWeaver::Job
{
public:
	AddQueuePackageIdListJob( const QStringList& packageIdList ) : Job(),
		m_packageIdList( packageIdList ) {}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {
		DbConnection* const m_db = KurooDBSingleton::Instance()->getStaticDbConnection();
		KurooDBSingleton::Instance()->singleQuery(	"CREATE TEMP TABLE queue_temp ( "
													"id INTEGER PRIMARY KEY AUTOINCREMENT, "
													"idPackage INTEGER, "
													"idDepend INTEGER, "
													"use VARCHAR(255), "
													"size VARCHAR(32), "
													"version VARCHAR(32) ); "
													//"installed BOOL NOT NULL DEFAULT 0 );"
													, m_db );
		KurooDBSingleton::Instance()->insert( "INSERT INTO queue_temp SELECT * FROM queue;", m_db );
		KurooDBSingleton::Instance()->singleQuery( "BEGIN TRANSACTION;", m_db );

		foreach( QString id,  m_packageIdList )
			KurooDBSingleton::Instance()->insert( QString( "INSERT INTO queue_temp (idPackage, idDepend) VALUES ('%1', '0');" ).arg(id), m_db );

		KurooDBSingleton::Instance()->singleQuery( "COMMIT TRANSACTION;", m_db );

		// Move content from temporary table to installedPackages
		KurooDBSingleton::Instance()->singleQuery( "DELETE FROM queue;", m_db );
		KurooDBSingleton::Instance()->insert( "INSERT INTO queue SELECT * FROM queue_temp;", m_db );
		KurooDBSingleton::Instance()->singleQuery( "DROP TABLE queue_temp;", m_db );

		KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
		QueueSingleton::Instance()->refresh( false );
	}

private:
	const QStringList m_packageIdList;
};

/**
* @class RemoveQueuePackageIdListJob
* @short Thread for removing packages from the queue in db.
*/
class RemoveQueuePackageIdListJob : public ThreadWeaver::Job
{
public:
	RemoveQueuePackageIdListJob( const QStringList& packageIdList ) : Job(),
		m_packageIdList( packageIdList ) {
		}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {
		DbConnection* const m_db = KurooDBSingleton::Instance()->getStaticDbConnection();
		foreach ( QString id, m_packageIdList )
		{
			KurooDBSingleton::Instance()->singleQuery( QString( "DELETE FROM queue WHERE ( idPackage = '%1' OR idDepend = '%2' );" )
														.arg(id).arg(id), m_db );
			QueueSingleton::Instance()->deleteFromCache(id);
		}
		KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
// 		return;
// 	}
//
// 	virtual void completeJob() {
		QueueSingleton::Instance()->refresh( false );
	}

private:
	const QStringList m_packageIdList;
};

/**
* @class AddResultsPackageListJob
* @short Thread for adding packages to results in db. Used by emerge.
*/
class AddResultsPackageListJob : public ThreadWeaver::Job
{
public:
	AddResultsPackageListJob( const EmergePackageList &packageList ) : Job(),
		m_packageList( packageList ) {}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {
		DbConnection* const m_db = KurooDBSingleton::Instance()->getStaticDbConnection();

		// Collect end-user packages
		QMap<QString, int> endUserPackageMap;
		const QStringList endUserPackageList = KurooDBSingleton::Instance()->query( "SELECT idPackage FROM queue WHERE idDepend = '0';", m_db );

		foreach ( QString id, endUserPackageList )
			endUserPackageMap.insert( id, 0 );

		KurooDBSingleton::Instance()->query("DELETE FROM queue;", m_db);

		//idPackage will contain the most recent package we found that was in endUserPackageList
		QString idPackage;
		// Iterate the emerge pretend package list
		EmergePackageList::ConstIterator itEnd = m_packageList.end();
		for ( EmergePackageList::ConstIterator it = m_packageList.begin(); it != itEnd; ++it ) {

			QString id = KurooDBSingleton::Instance()->singleQuery(
				"SELECT id FROM package WHERE name = '" + (*it).name + "' AND category = '" + (*it).category + "' LIMIT 1;", m_db );

			if ( id.isEmpty() ) {
				qWarning() << QString("Add result package list: Can not find id in database for package %1/%2.")
								.arg( (*it).category ).arg( (*it).name );

				KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
				return;
			}

			// We found a dependency, add it
			if ( !endUserPackageMap.contains( id ) ) {
				KurooDBSingleton::Instance()->insert( QString(
					"INSERT INTO queue (idPackage, idDepend, use, size, version) VALUES ('%1', '%2', '%3', '%4', '%5');" )
					.arg( id ).arg( idPackage ).arg( (*it).useFlags ).arg( (*it).size ).arg( (*it).version ), m_db );
			}
			else {
				idPackage = id;
				KurooDBSingleton::Instance()->insert( QString(
					"INSERT INTO queue (idPackage, idDepend, use, size, version) VALUES ('%1', '0', '%2', '%3', '%4');" )
					.arg( id ).arg( (*it).useFlags ).arg( (*it).size ).arg( (*it).version ), m_db );
			}
		}
		KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
// 		return;
// 	}
//
// 	virtual void completeJob() {
		QueueSingleton::Instance()->refresh( true );
	}

private:
	const EmergePackageList m_packageList;

};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Queue
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
* @class Queue
* @short Object for packages to be emerged = installation queue.
*/
Queue::Queue( QObject* m_parent )
	: QObject( m_parent ), m_isQueueBusy( false ), m_removeInstalled( false )
{
	// Clock timer for showing progress when emerging
	m_internalTimer = new QTimer( this );
	connect(m_internalTimer, &QTimer::timeout, this, &Queue::slotOneStep);

	// When all packages are emerged...
	connect( EmergeSingleton::Instance(), SIGNAL( signalEmergeComplete() ), this, SLOT( slotClearQueue() ) );
}

Queue::~Queue()
{}

void Queue::init( QObject *parent )
{
	m_parent = parent;
}

/**
* Forward signal to refresh queue.
*/
void Queue::refresh( bool hasCheckedQueue )
{
	//This seems to fix the big crash bug. #3166840 or #3027148
	clearCache();
	emit signalQueueChanged( hasCheckedQueue );
	SignalistSingleton::Instance()->packageQueueChanged();
}

/**
* Clear the queue.
*/
void Queue::reset()
{
	KurooDBSingleton::Instance()->resetQueue();
	refresh( false );
}

/**
* Convenience method.
*/
int Queue::size()
{
	return m_queueCache.size();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Queue cache handling
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
* Check if package is the queue.
* @param id
* @return true/false
*/
bool Queue::isQueued( const QString& id )
{
	return m_queueCache.contains( id );
}

bool Queue::hasCompleted(const QString& id)
{
	if (isQueued(id))
		return m_queueCache[id];

	return false;
}

/**
* Clear Queue.
* @param id
*/
void Queue::clearCache()
{
	m_queueCache.clear();
}

/**
* When the package is inserted in the queue register it in the cache too.
* @param id
*/
void Queue::insertInCache( const QString& id )
{
	if ( id.isEmpty() ) {
		qWarning() << "Package id is empty, skipping!";
		return;
	}
	m_queueCache.insert( id, hasCompleted(id) );
}

/**
* When the package is removed from queue remove from cache.
* @param id
*/
void Queue::deleteFromCache( const QString& id )
{
	if ( id.isEmpty() ) {
		qDebug() << "Package id is empty, skipping!";
		return;
	}
	qDebug() << "deleted from cache (" << id << ")";
	m_queueCache.remove( id );
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// package progress handling
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
* Start the package installation timer.
* @param package
*/
void Queue::emergePackageStart( const QString& package/*, int order, int total */)
{
	DEBUG_LINE_INFO;
	QString id = KurooDBSingleton::Instance()->packageId( package );
	if ( isQueued( id ) )
		m_queueCache[ id ] = false;

	m_internalTimer->start( 1000 );
	emit signalPackageStart( id );
	m_isQueueBusy = true;
}

/**
* Pause emerge
*/
void Queue::pauseEmerge()
{
	m_internalTimer->stop();
}

/**
* Unpause emerge
*/
void Queue::unpauseEmerge()
{
	m_internalTimer->start( 1000 );
}

/**
* Set package progress as 100% = complete.
* @param package
*/
void Queue::emergePackageComplete( const QString& package/*, int order, int total */)
{
	//m_isQueueBusy = false;
	m_internalTimer->stop();
	QString id = KurooDBSingleton::Instance()->packageId( package );
	if ( isQueued( id ) )
		m_queueCache[ id ] = true;

	emit signalPackageComplete( id );
}

/**
* Initialize with package id and after that emit 1sec progress signals.
* @param id
*/
void Queue::slotOneStep()
{
	emit signalPackageAdvance();
}

bool Queue::isQueueBusy()
{
	return m_isQueueBusy;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Package handling
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
* Add packages to the results table in the db
* @param packageList
*/
void Queue::addPackageList( const EmergePackageList &packageList )
{
	if ( !packageList.isEmpty() )
		ThreadWeaver::Queue::instance()->stream() << new AddResultsPackageListJob( packageList );
}

/**
* Remove packages from queue.
* @param packageIdList
*/
void Queue::removePackageIdList( const QStringList& packageIdList )
{
	ThreadWeaver::Queue::instance()->stream() << new RemoveQueuePackageIdListJob( packageIdList );
}

/**
* Add packages to the installation queue table in the db.
* @param packageIdList
*/
void Queue::addPackageIdList( const QStringList& packageIdList )
{
	ThreadWeaver::Queue::instance()->stream() << new AddQueuePackageIdListJob( packageIdList );
}

/**
* Launch emerge of all packages in the queue.
* @param packageIdList
*/
void Queue::installQueue( const QStringList& packageList )
{
	EmergeSingleton::Instance()->queue( packageList );
}

/**
* User wants queue to be cleared after emerge is done.
* @param removeInstalled
*/
void Queue::setRemoveInstalled( bool removeInstalled )
{
	m_removeInstalled = removeInstalled;
}

/**
* Clear installed packages from queue after emerge is done.
*/
void Queue::slotClearQueue()
{
	qDebug() << "Clearing Queue";
	// Queue is not busy anymore - off course
	m_isQueueBusy = false;

	// Make sure the timer is stopped
	m_internalTimer->stop();

	if ( m_removeInstalled ) {

		qDebug() << "Removing installed";
		// Collect only 100% complete packages
		QStringList idList;
		for ( QMap<QString, bool>::iterator itMap = m_queueCache.begin(), itMapEnd = m_queueCache.end(); itMap != itMapEnd; ++itMap ) {
			if ( itMap.value() )
				idList += itMap.key();
		}
		if ( !idList.isEmpty() )
			removePackageIdList( idList );
	}
}


