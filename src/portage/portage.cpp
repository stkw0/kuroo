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

#include <unistd.h>
#include <QTextStream>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/JobPointer>
#include <ThreadWeaver/QObjectDecorator>
#include <ThreadWeaver/Queue>
#include <ThreadWeaver/Thread>

#include "common.h"
#include "scanportagejob.h"
#include "cacheportagejob.h"
#include "scanupdatesjob.h"

class AddInstalledPackageJobImpl : public ThreadWeaver::Job
{
public:
	AddInstalledPackageJobImpl( const QString& package ) : ThreadWeaver::Job(), m_package( package ) {}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {

		QStringList parts = parsePackage( m_package );
		if ( parts.isEmpty() ) {
			qWarning() << QString("Inserting emerged package: can not match %1.").arg( m_package );
			return;
		}
		QString category = parts[0];
		QString name = parts[1];
		QString version = parts[2];

		DbConnection* const m_db = KurooDBSingleton::Instance()->getStaticDbConnection();
		QString id = KurooDBSingleton::Instance()->singleQuery( QString( "SELECT id FROM package WHERE "
			"name = '%1' AND category = '%2' LIMIT 1;").arg( name ).arg( category ), m_db );

		if ( id.isEmpty() ) {

			qWarning() << QString("Inserting emerged package: Can not find id in database for package %1/%2.")
				.arg( category ).arg( name );

			KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
//			return;
		}
		else {
			KurooDBSingleton::Instance()->query( QString( "UPDATE package SET status = '%1' WHERE id = '%2';" )
												.arg( PACKAGE_INSTALLED_STRING ).arg( id ), m_db );

			KurooDBSingleton::Instance()->query( QString( "UPDATE version SET status = '%1' WHERE idPackage = '%2' AND name = '%3';" )
												.arg( PACKAGE_INSTALLED_STRING ).arg( id ).arg( version ), m_db );

			// Clean out the update string if this version was installed
			KurooDBSingleton::Instance()->query( QString( "UPDATE package SET updateVersion = '' "
														"WHERE id = '%1' AND ( updateVersion = '%2' OR updateVersion = '%3' );" )
												.arg( id ).arg( version + " (D)" ).arg( version + " (U)" ), m_db );

			KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
//			return;
		}
// 	}
//
// 	virtual void completeJob() {
		PortageSingleton::Instance()->slotPackageChanged();
	}

private:
	const QString m_package;
};
/**
* @class AddInstalledPackageJob
* @short Thread for registrating packages as installed in db.
*/
class AddInstalledPackageJob : public ThreadWeaver::QObjectDecorator
{
public:
	AddInstalledPackageJob( const QString& package )
	: ThreadWeaver::QObjectDecorator( new AddInstalledPackageJobImpl( package ) ) {}
};

class RemoveInstalledPackageJobImpl : public ThreadWeaver::Job
{
public:
	RemoveInstalledPackageJobImpl( const QString& package ) : ThreadWeaver::Job(), m_package( package ) {}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {
		QStringList parts = parsePackage( m_package );
		if ( parts.isEmpty() ) {
			qWarning() << QString("Removing unmerged package: can not match %1.").arg( m_package );
			return;
		}
		QString category = parts[0];
		QString name = parts[1];
		QString version = parts[2];

		DbConnection* const m_db = KurooDBSingleton::Instance()->getStaticDbConnection();

		QString id = KurooDBSingleton::Instance()->singleQuery( QString( "SELECT id FROM package WHERE "
			"name = '%1' AND category = '%2' LIMIT 1;").arg( name ).arg( category ), m_db );

		if ( id.isEmpty() ) {
			qWarning() << QString("Removing unmerged package: Can not find id in database for package %1/%2.")
				.arg( category ).arg( name );

			KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
//			return;
		}
		else {

			// Check how many version are installed
			QString installedVersionCount = KurooDBSingleton::Instance()->singleQuery(
				QString( "SELECT COUNT(id) FROM version WHERE idPackage = '%1' AND status = '%2' LIMIT 1;")
				.arg( id ).arg( PACKAGE_INSTALLED_STRING ), m_db );

			// Mark package as uninstalled only when one version is found
			if ( installedVersionCount == "1" ) {

				// Mark package as uninstalled
				KurooDBSingleton::Instance()->singleQuery( QString( "UPDATE package SET status = '%1' WHERE id = '%2'")
															.arg( PACKAGE_AVAILABLE_STRING ).arg( id ), m_db );

				// Delete package if "old" = not in official Portage anymore
				KurooDBSingleton::Instance()->singleQuery( QString( "UPDATE package SET status = '%1' WHERE status = '%2' AND id = '%3';" )
															.arg( PACKAGE_DELETED_STRING ).arg( PACKAGE_OLD_STRING ).arg( id ), m_db );
			}

			// And now mark this specific version as not installed
			KurooDBSingleton::Instance()->singleQuery( QString( "UPDATE version SET status = '%1' WHERE idPackage = '%2' AND name = '%3';" )
														.arg( PACKAGE_AVAILABLE_STRING ).arg( id ).arg( version ), m_db );

			KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
//			return;
		}
// 	}
//
// 	virtual void completeJob() {
		PortageSingleton::Instance()->slotPackageChanged();
	}

private:
	const QString m_package;
};

/**
* @class RemoveInstalledPackageJob
* @short Thread for removing packages as installed in db.
*/
class RemoveInstalledPackageJob : public ThreadWeaver::QObjectDecorator
{
public:
	RemoveInstalledPackageJob( const QString& package )
	: ThreadWeaver::QObjectDecorator( new RemoveInstalledPackageJobImpl( package ) ) {}
};

class CheckUpdatesPackageJobImpl : public ThreadWeaver::Job
{
public:
	CheckUpdatesPackageJobImpl( const QString& id, const QString& updateVersion, int hasUpdate )
	: ThreadWeaver::Job(),
		m_id( id ), m_updateVersion( updateVersion ), m_hasUpdate( hasUpdate ) {}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {
		DbConnection* const m_db = KurooDBSingleton::Instance()->getStaticDbConnection();

		if ( m_hasUpdate == 0 ) {
			KurooDBSingleton::Instance()->singleQuery( QString("UPDATE package SET updateVersion = '', status = '%1' WHERE id = '%2';")
														.arg( PACKAGE_INSTALLED_STRING ).arg( m_id ), m_db );
		}
		else {
			QString updateString;
			if ( m_hasUpdate > 0 )
				updateString = m_updateVersion + " (U)";
			else
				updateString = m_updateVersion + " (D)";

			KurooDBSingleton::Instance()->singleQuery( QString( "UPDATE package SET updateVersion = '%1', status = '%2' WHERE id = '%3';" )
														.arg( updateString ).arg( PACKAGE_UPDATES_STRING ).arg( m_id ), m_db );
		}

		KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
// 		return;
// 	}
//
// 	virtual void completeJob() {
		PortageSingleton::Instance()->slotChanged();
	}

private:
	const 	QString m_id, m_updateVersion;
	int 	m_hasUpdate;
};
/**
* @class CheckUpdatesPackageJob
* @short Thread for marking packages as updates or downgrades.
*/
class CheckUpdatesPackageJob : public ThreadWeaver::QObjectDecorator
{
public:
	CheckUpdatesPackageJob( const QString& id, const QString& updateVersion, int hasUpdate )
		: ThreadWeaver::QObjectDecorator( new CheckUpdatesPackageJobImpl( id, updateVersion, hasUpdate ) ) {}
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
* @class Portage
* @short Object handling the Portage tree.
*
* @todo: Register @world packages in db.
*/
Portage::Portage( QObject *m_parent )
	: QObject( m_parent )
{
	// When cache scan is done go one scanning portage for all packages
	connect( SignalistSingleton::Instance(), SIGNAL( signalCachePortageComplete() ), this, SLOT( slotScan() ) );
	// Then portage scan is completed
	connect( SignalistSingleton::Instance(), SIGNAL( signalScanPortageComplete() ), this, SLOT( slotScanCompleted() ) );

	connect( SignalistSingleton::Instance(), SIGNAL( signalScanUpdatesComplete() ), this, SLOT( slotLoadUpdates() ) );
	connect( SignalistSingleton::Instance(), SIGNAL( signalLoadUpdatesComplete() ), this, SLOT( slotChanged() ) );

	// Start refresh directly after emerge sync
	connect( SignalistSingleton::Instance(), SIGNAL( signalSyncDone() ), this, SLOT( slotSyncCompleted() ) );
}

Portage::~Portage()
{}

void Portage::init( QObject *parent )
{
	m_parent = parent;
	loadWorld();
}

/**
* Emit signal after changes in Portage.
*/
void Portage::slotChanged()
{
	// Register in db so we can check at next start if user has emerged any packages outside kuroo
	KurooDBSingleton::Instance()->setKurooDbMeta( "scanTimeStamp", QString::number( QDateTime::currentDateTime().toTime_t() ) );

	emit signalPortageChanged();
}

/**
* Reload @world when new package is installed/removed in case user not using --oneshot.
*/
void Portage::slotPackageChanged()
{
	loadWorld();
	slotChanged();
}


////////////////////////////////////////////////////////////////////////////////////////
// Portage handling...
////////////////////////////////////////////////////////////////////////////////////////

/**
* Start scan of portage packages.
* @return bool
*/
bool Portage::slotRefresh()
{
	DEBUG_LINE_INFO;
	// Update cache if empty
	if ( KurooDBSingleton::Instance()->isCacheEmpty() ) {
		SignalistSingleton::Instance()->scanStarted();
		//This seems important, not sure why it was commented out
		CachePortageJob *job = new CachePortageJob();
		connect(job, &CachePortageJob::done, this, &Portage::slotWeaverDone);
		ThreadWeaver::Queue::instance()->stream() << job;
	}
	else
		slotScan();

	return true;
}

/**
* Start emerge sync.
* @return bool
*/
bool Portage::slotSync()
{
	EmergeSingleton::Instance()->sync();
	return true;
}

/**
* Stop progressbar.
*/
void Portage::slotSyncCompleted()
{
	slotRefresh();
}

/**
* Continue with scan of portage packages.
* @return bool
*/
bool Portage::slotScan()
{
	DEBUG_LINE_INFO;

	// Wait for cache job to finish before launching the scan.
	int maxLoops( 99 );
	while ( true ) {
		if ( KurooDBSingleton::Instance()->isCacheEmpty() )
			::usleep( 100000 ); // Sleep 100 msec
		else
			break;

		if ( maxLoops-- == 0 ) {
			qWarning() << "Scanning Portage. Wait-counter has reached maximum. Attempting to scan Portage.";
			break;
		}
	}

	ScanPortageJob *job = new ScanPortageJob();
	connect(job, &ScanPortageJob::done, this, &Portage::slotWeaverDone);
	ThreadWeaver::Queue::instance()->stream() << job;

	return true;
}

/**
* After portage has completed scanning for all packages, check for updates.
*/
void Portage::slotScanCompleted()
{
	// Reset Queue with it's own cache
	QueueSingleton::Instance()->reset();

	// Now all Portage files
	PortageFilesSingleton::Instance()->loadPackageFiles();

	// Ready to roll!
	SignalistSingleton::Instance()->setKurooReady( true );
	slotChanged();

	// Go on with checking for updates
	if ( KurooDBSingleton::Instance()->isUpdatesEmpty() )
		slotRefreshUpdates();
}


////////////////////////////////////////////////////////////////////////////////////////
// World handling...
////////////////////////////////////////////////////////////////////////////////////////

/**
* Load packages in @world file. @fixme: optimize this...
*/
void Portage::loadWorld()
{
	m_mapWorld.clear();

	QFile file( KurooConfig::fileWorld() );
	if ( file.open( QIODevice::ReadOnly ) ) {
		QTextStream stream( &file );
		while ( !stream.atEnd() ) {
			QString package = stream.readLine();
			m_mapWorld[ package.trimmed() ] = QString::null;
		}
		emit signalWorldChanged();
	}
	else
		qCritical() << "Loading packages in @world. Reading: " << KurooConfig::fileWorld();
}

/**
* Check if this package in is @world file.
* @param package
* @return true/false
*/
bool Portage::isInWorld( const QString& package )
{
	return m_mapWorld.contains( package );
}

/**
* Add package to @world file.
* @param package
*/
void Portage::appendWorld( const QStringList& packageList )
{
	// Check is @world is writable
	QFile file( KurooConfig::fileWorld() );
	if ( !file.open( QIODevice::WriteOnly ) ) {
		qCritical() << "Adding packages to @world. Writing: " << KurooConfig::fileWorld();
		return;
	}

	// Make a copy of @world map
	QMap<QString, QString> map = m_mapWorld;

	// Add/update package into @world map
	foreach ( QString pkg, packageList )
		m_mapWorld.insert( pkg, QString::null );

	// Update @world file
	QTextStream stream( &file );
	for ( QMap<QString, QString>::ConstIterator it = m_mapWorld.begin(), end = m_mapWorld.end(); it != end; ++it )
		stream << it.key() << "\n";
	file.close();

	emit signalWorldChanged();
}

/**
* Remove package from @world file.
* @param package
*/
void Portage::removeFromWorld( const QStringList& packageList )
{
	// Check is @world is writable
	QFile file( KurooConfig::fileWorld() );
	if ( !file.open( QIODevice::WriteOnly ) ) {
		qCritical() << "Removing packages from @world. Writing: " << KurooConfig::fileWorld();
		return;
	}

	// Make a copy of @world map
	foreach( QString pkg, packageList )
		m_mapWorld.remove( pkg );

	// Update @world file
	QTextStream stream( &file );
	for ( QMap<QString, QString>::ConstIterator it = m_mapWorld.begin(), end = m_mapWorld.end(); it != end; ++it )
		stream << it.key() << "\n";
	file.close();

	emit signalWorldChanged();
}

////////////////////////////////////////////////////////////////////////////////////////
// Package handlling...
////////////////////////////////////////////////////////////////////////////////////////

/**
* Launch emerge pretend of packages.
* @param category
* @param packageList
*/
void Portage::pretendPackageList( const QStringList& packageIdList )
{
	QStringList packageList;
	foreach ( QString id, packageIdList )
		packageList += KurooDBSingleton::Instance()->category( id ) + "/" + KurooDBSingleton::Instance()->package( id );

	EmergeSingleton::Instance()->pretend( packageList );
}

/**
* Launch unmerge of packages
* @param category
* @param packageList
*/
void Portage::uninstallInstalledPackageList( const QStringList& packageIdList )
{
	QStringList packageList;
	foreach ( QString id, packageIdList )
		packageList += KurooDBSingleton::Instance()->category( id ) + "/" + KurooDBSingleton::Instance()->package( id );

	EmergeSingleton::Instance()->unmerge( packageList );
}

/**
* @fixme: Check for failure.
* Add package as installed in db.
* @param package
*/
void Portage::addInstalledPackage( const QString& package )
{
	AddInstalledPackageJob *job = new AddInstalledPackageJob( package );
	connect(job, &AddInstalledPackageJob::done, this, &Portage::slotWeaverDone);
	ThreadWeaver::Queue::instance()->stream() << job;
}

/**
* Remove packages from db. @todo: Check for failure.
* @param packageIdList
*/
void Portage::removeInstalledPackage( const QString& package )
{
	RemoveInstalledPackageJob *job = new RemoveInstalledPackageJob( package );
	connect(job, &RemoveInstalledPackageJob::done, this, &Portage::slotWeaverDone);
	ThreadWeaver::Queue::instance()->stream() << job;
}

/**
* Start scan of update packages.
* @return bool
*/
bool Portage::slotRefreshUpdates()
{
	EmergeSingleton::Instance()->checkUpdates();
	return true;
}

/**
* Start scan of update packages.
* @return bool
*/
bool Portage::slotLoadUpdates()
{
	SignalistSingleton::Instance()->scanStarted();
	ScanUpdatesJob *job = new ScanUpdatesJob( EmergeSingleton::Instance()->packageList() );
	connect(job, &ScanUpdatesJob::done, this, &Portage::slotWeaverDone);
	ThreadWeaver::Queue::instance()->stream() << job;
	return true;
}

/**
* Update packages when user changes package stability.
*/
void Portage::checkUpdates( const QString& id, const QString& emergeVersion, int hasUpdate )
{
	CheckUpdatesPackageJob *job = new CheckUpdatesPackageJob( id, emergeVersion, hasUpdate );
	connect(job, &CheckUpdatesPackageJob::done, this, &Portage::slotWeaverDone);
	ThreadWeaver::Queue::instance()->stream() << job;
}

void Portage::slotWeaverDone(ThreadWeaver::JobPointer job)
{
	Q_UNUSED(job);
	DEBUG_LINE_INFO;
	//WARN: Hope that QSharedPointer takes care of this so it doesn't leak memory
	//delete job;
}

