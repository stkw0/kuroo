/***************************************************************************
 *	Copyright (C) 2005 by Karye												*
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

#include "common.h"
#include "cacheportagejob.h"

#include <fstream>
#include <string>

#include <QDebug>
#include <QDir>
#include "settings.h"
#include <ThreadWeaver/Job>
#include <ThreadWeaver/JobPointer>
#include <ThreadWeaver/QObjectDecorator>
#include <ThreadWeaver/Thread>

/**
 * @class CachePortageJob
 * @short Thread to cache package information from the Portage directory to speed up portage refreshing.
 *
 * Portage cache is scanned for package sizes, and stored in portage cache map and in the database.
 */
CachePortageJob::CachePortageJob()
    : ThreadWeaver::QObjectDecorator(new CachePortageJobImpl())
{}

CachePortageJobImpl::CachePortageJobImpl() : ThreadWeaver::Job(),
	m_db( KurooDBSingleton::Instance()->getStaticDbConnection() )
{
}

CachePortageJobImpl::~CachePortageJobImpl()
{
	KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
}

/**
 * Scan for package size found in digest files.
 */
void CachePortageJobImpl::run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* )
{
	DEBUG_LINE_INFO;
	if ( !m_db->isConnected() ) {
		qCritical() << "Creating cache. Can not connect to database";
		return;// false;
	}

	int count( 0 );
	QMap <QString, QString> mapCache;
	QDir dCategory, dPackage;
	dCategory.setFilter( QDir::Dirs | QDir::NoSymLinks );
	dCategory.setSorting( QDir::Name );

	// Get a count of total packages for proper progress
	QString packageCount = KurooDBSingleton::Instance()->singleQuery( "SELECT data FROM dbInfo WHERE meta = 'packageCount' LIMIT 1;", m_db );
    /*if ( packageCount == "0" )
		setProgressTotalSteps( 35000 );
	else
        setProgressTotalSteps( packageCount.toInt() );*/
    //setStatus( "CachePortage", i18n("Collecting package information...") );

	// Get list of categories in Portage and Overlays
	QStringList pathList = KurooConfig::dirPortage().split(" ");
	const QStringList pathOverlays = KurooConfig::dirPortageOverlay().split(" ");
	foreach ( QString path, pathOverlays ) {
		pathList += path;
	}

	// Scan Portage cache
	for ( QStringList::Iterator itPath = pathList.begin(), itPathEnd = pathList.end(); itPath != itPathEnd; ++itPath ) {
		if ( !dCategory.cd( *itPath ) ) {
			qWarning() << "Creating cache. Can not access " << *itPath;
			continue;
		}

		QStringList categoryList = dCategory.entryList();
		QStringList::Iterator itCategoryEnd = categoryList.end();
		for ( QStringList::Iterator itCategory = categoryList.begin(); itCategory != itCategoryEnd; ++itCategory ) {
			//TODO: Filter out distfiles, eclass, licences, metadata, packages, profiles, scripts
			if ( *itCategory == "." || *itCategory == ".." )
				continue;

            /*// Abort the scan
			if ( isAborted() ) {
				qWarning() << "Creating cache. Aborted!";
				setStatus( "CachePortage", i18n("Caching aborted.") );
				return false;
            }*/

			// Get list of packages in this category
			dPackage.setFilter( QDir::Files | QDir::NoSymLinks );
			dPackage.setSorting( QDir::Name );
			if ( dPackage.cd( *itPath + "/metadata/md5-cache/" + *itCategory ) ) {
				QStringList packageList = dPackage.entryList();
				QStringList::Iterator itPackageEnd = packageList.end();
				for ( QStringList::Iterator itPackage = packageList.begin(); itPackage != itPackageEnd; ++itPackage ) {

					if ( *itPackage == "." || *itPackage == ".." )
						continue;

                    /*// Abort the scan
					if ( isAborted() ) {
						qWarning() << "Creating cache. Aborted!";
						setStatus( "CachePortage", i18n("Caching aborted.") );
						return false;
                    }*/
					QString package = *itCategory + "/" + *itPackage;

                    QStringList parts = parsePackage( *itPackage );
					if ( !parts.isEmpty() ) {
						QString packageName = parts[1];

//TODO: /files/digest-* doesn't seem to exist anymore, maybe we could read from 'Manifest' instead?
						// Get package size
						mapCache.insert( package, "12345" );
						/*QString path = *itPath + "/" + *itCategory + "/" + packageName + "/files/digest-" + *itPackage;
						QFile file( path );
						if ( file.open( QIODevice::ReadOnly ) ) {
							std::ifstream in( path );
							std::string word;
							while ( in >> word );
							mapCache.insert( package, word );
							file.close();
						}
						else
							qWarning() << "Creating cache. Reading: " << path;*/
					}
					else
						qWarning() << "Creating cache. Can not parse: " << *itPackage;

					// Post scan count progress
                    /*if ( (++count % 100) == 0 )
                        setProgress( count );*/
				}
			}
			else
				qWarning() << "Creating cache. Can not access " << *itPath << "/metadata/cache/" << *itCategory;

		}
	}
	KurooDBSingleton::Instance()->query( QString("UPDATE dbInfo SET data = '%1' WHERE meta = 'packageCount';")
	                                     .arg( count ), m_db );

	// Store cache in DB
	KurooDBSingleton::Instance()->query( "DELETE FROM cache;", m_db );
	KurooDBSingleton::Instance()->query( "BEGIN TRANSACTION;", m_db );
	QMap< QString, QString >::iterator itMapEnd = mapCache.end();
	for ( QMap< QString, QString >::iterator itMap = mapCache.begin(); itMap != itMapEnd; ++itMap )
		KurooDBSingleton::Instance()->insert( QString("INSERT INTO cache (package, size) VALUES ('%1', '%2');").
		                                      arg( itMap.key() ).arg( itMap.value() ), m_db );

	KurooDBSingleton::Instance()->query("COMMIT TRANSACTION;", m_db );

    //setStatus( "CachePortage", i18n("Done.") );
    //setProgress( 0 );
	DEBUG_LINE_INFO;
// 	return true;
// }
//
// void CachePortageJob::completeJob()
// {
	SignalistSingleton::Instance()->cachePortageComplete();
}

