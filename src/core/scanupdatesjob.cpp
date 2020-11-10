/***************************************************************************
*   Copyright (C) 2004 by karye                                           *
*   karye@users.sourceforge.net                                           *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <KApplication>
#include <KConfig>
#include <KGlobal>

#include <QDir>
#include <QStringList>
#include <QFileInfo>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/JobPointer>
#include <ThreadWeaver/QObjectDecorator>
#include <ThreadWeaver/Thread>

#include "common.h"
#include "scanupdatesjob.h"

/**
* @class ScanUpdatesJob
* @short Thread for loading 'emerge -uDpv World' output into db.
*/
ScanUpdatesJob::ScanUpdatesJob( const EmergePackageList &packageList )
	: ThreadWeaver::QObjectDecorator( new ScanUpdatesJobImpl( packageList ) )
{}
ScanUpdatesJobImpl::ScanUpdatesJobImpl( const EmergePackageList &packageList ) : ThreadWeaver::Job(),
	m_db( KurooDBSingleton::Instance()->getStaticDbConnection() ), m_packageList( packageList )
{}

ScanUpdatesJobImpl::~ScanUpdatesJobImpl()
{
	KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );

	/*if ( isAborted() )
		SignalistSingleton::Instance()->scanAborted();*/
}

/**
* Insert found updates into db.
* @return success
*/
void ScanUpdatesJobImpl::run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* )
{
	if ( !m_db->isConnected() ) {
		qCritical() << "Scanning updates. Can not connect to database";
		return;
	}

	if ( m_packageList.isEmpty() )
		qWarning() << "Scanning updates. No update package found";

	/*setStatus( "ScanUpdates", i18n("Refreshing updates view...") );
	setProgressTotalSteps( m_packageList.count() );*/
//	int count(0);

	// Temporary tables to avoid locking main table
	KurooDBSingleton::Instance()->singleQuery(	"CREATE TEMP TABLE package_temp ( "
												"id INTEGER PRIMARY KEY AUTOINCREMENT, "
												"idCategory INTEGER, "
												"idSubCategory INTEGER, "
												"category VARCHAR(32), "
												"name VARCHAR(32), "
												"description VARCHAR(255), "
												"path VARCHAR(64), "
												"status INTEGER, "
												"meta VARCHAR(255), "
												"updateVersion VARCHAR(32) );"
												, m_db );

	KurooDBSingleton::Instance()->insert( "INSERT INTO package_temp SELECT * FROM package;", m_db );
	KurooDBSingleton::Instance()->singleQuery( QString("UPDATE package_temp SET updateVersion = '', status = '%1' WHERE status = '%2';")
										.arg( PACKAGE_INSTALLED_STRING ).arg( PACKAGE_UPDATES_STRING ), m_db );
	KurooDBSingleton::Instance()->singleQuery("BEGIN TRANSACTION;", m_db);

	EmergePackageList::ConstIterator itEnd = m_packageList.end();
	for ( EmergePackageList::ConstIterator it = m_packageList.begin(); it != itEnd; ++it ) {

		// Abort the scan
		/*if ( isAborted() ) {
			qWarning() << "Scanning updates. Scan aborted!";
			KurooDBSingleton::Instance()->singleQuery( "ROLLBACK TRANSACTION;", m_db );
			return false;
		}*/

		// count is also ordering number
		//setProgress( count++ );

		// Find id for this category in db
		QString id = KurooDBSingleton::Instance()->singleQuery( " SELECT id FROM package WHERE name = '" +
			(*it).name + "' AND category = '" + (*it).category + "' LIMIT 1;", m_db );

		if ( id.isEmpty() ) {
			qWarning() << QString("Scanning updates. Can not find id in database for package %1/%2.")
				.arg( (*it).category ).arg( (*it).name );
		}
		else {

			// Mark as update in portage, but not for new packages
			if ( !(*it).updateFlags.contains( QRegExp("\\bN\\b") ) && !id.isEmpty() ) {

				// Is the package upgrade or downgrade?
				QString updateVersion;
				if ( (*it).updateFlags.contains("D") )
					updateVersion = (*it).version + " (D)";
				else
					updateVersion = (*it).version + " (U)";

				KurooDBSingleton::Instance()->singleQuery( QString( "UPDATE package_temp SET updateVersion = '%1', status = '%2' WHERE id = '%3';" )
													.arg( updateVersion ).arg( PACKAGE_UPDATES_STRING ).arg( id ), m_db );

			}
		}
	}
	KurooDBSingleton::Instance()->singleQuery("COMMIT TRANSACTION;", m_db );

	// Move content from temporary table
	KurooDBSingleton::Instance()->singleQuery( "DELETE FROM package;", m_db );
	KurooDBSingleton::Instance()->insert( "INSERT INTO package SELECT * FROM package_temp;", m_db );
	KurooDBSingleton::Instance()->singleQuery( "DROP TABLE package_temp;", m_db );

	/*setStatus( "ScanUpdates", i18n( "Done." ) );
	setProgressTotalSteps( 0 );*/

	SignalistSingleton::Instance()->loadUpdatesComplete();
}

