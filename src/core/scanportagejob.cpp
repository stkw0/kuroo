/***************************************************************************
 *	Copyright (C) 2005 by karye												*
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
#include "scanportagejob.h"
#include "signalist.h"
#include <sqlite3.h>

#include <fstream>
#include <string>
#include <vector>

#include <KGlobal>
#include <QDir>
#include <QTextStream>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/JobPointer>
#include <ThreadWeaver/QObjectDecorator>
#include <ThreadWeaver/Thread>

/**
 * @class ScanPortageJob
 * @short Thread for scanning local portage tree for available packages.
 *
 * The packages are counted first, this to get a correct refresh progress in the gui.
 * Next portage cache in KurooConfig::dirEdbDep() is scanned for packages.
 * All packages are stored in table "package" in the database.
 */
ScanPortageJob::ScanPortageJob()
    : ThreadWeaver::QObjectDecorator( new ScanPortageJobImpl() )
{}
ScanPortageJobImpl::ScanPortageJobImpl() : ThreadWeaver::Job(),
	m_db( KurooDBSingleton::Instance()->getStaticDbConnection() )
{}

ScanPortageJobImpl::~ScanPortageJobImpl()
{
	KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );

    /*if ( isAborted() )
        SignalistSingleton::Instance()->scanAborted();*/
}

/**
 * Scan Portage cache for packages in portage tree. Inserting found packages in db.
 * @return success
 */
void ScanPortageJobImpl::run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* )
{
	DEBUG_LINE_INFO;
	int count = 0;
	QDir dCategory, dPackage;
	dCategory.setFilter( QDir::Dirs | QDir::NoSymLinks );
	dCategory.setSorting( QDir::Name );

	if ( !m_db->isConnected() ) {
		qCritical() << "Scanning Portage. Can not connect to database";
        return;
	}

	SignalistSingleton::Instance()->scanPortageStarted();

	// Load Portage cache files to speed up portage scan
	loadCache();

	// Temporary table for all categories
	KurooDBSingleton::Instance()->singleQuery(	"BEGIN TRANSACTION;", m_db );
	KurooDBSingleton::Instance()->singleQuery(	"CREATE TEMP TABLE category_temp ( "
	                                    		"id INTEGER PRIMARY KEY AUTOINCREMENT, "
	                                    		"name VARCHAR(32) UNIQUE );"
	                                    		, m_db );

	KurooDBSingleton::Instance()->singleQuery(	"CREATE TEMP TABLE subCategory_temp ( "
	                                    		"id INTEGER PRIMARY KEY AUTOINCREMENT, "
	                                    		"name VARCHAR(32), "
	                                    		"idCategory INTEGER );"
	                                    		, m_db );

	// Temporary table for all packages
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

	// Temporary table for all versions
	KurooDBSingleton::Instance()->singleQuery(	"CREATE TEMP TABLE version_temp ( "
	                                          	"id INTEGER PRIMARY KEY AUTOINCREMENT, "
	                                          	"idPackage INTEGER, "
	                                          	"name VARCHAR(32),"
	                                          	"description VARCHAR(255), "
	                                          	"homepage VARCHAR(128), "
	                                          	"licenses VARCHAR(64), "
	                                          	"useFlags VARCHAR(255),"
	                                          	"slot VARCHAR(32),"
	                                          	"size VARCHAR(32), "
	                                          	"status INTEGER, "
	                                          	"keywords VARCHAR(32) );"
	                                          	, m_db );

	// Gather all path = portage and overlays
	QStringList pathList;
	pathList += KurooConfig::dirPortage() + "/metadata/md5-cache/";
	const QStringList pathOverlays = KurooConfig::dirPortageOverlay().split(" ");
	foreach ( QString path, pathOverlays ) {
		pathList += path;
	}

	// Scan Portage cache
	for ( QStringList::Iterator itPath = pathList.begin(), itPathEnd = pathList.end(); itPath != itPathEnd; ++itPath ) {

		//qDebug() << "Scanning Portage. Reading categories from " << *itPath;
		//TODO: This is where I need to start removing deps on /var/cache/edb
		if ( !dCategory.cd( *itPath ) ) {
			//qWarning() << "Scanning Portage. Can not access " << *itPath ;
			continue;
		}

		// Get list of categories in Portage
        int idCategory=0;
		QString lastCategory;
		QStringList categoryList = dCategory.entryList();
		for ( QStringList::Iterator itCategory = categoryList.begin(), itCategoryEnd = categoryList.end(); itCategory != itCategoryEnd; ++itCategory ) {

			//Filter out distfiles, eclass, licenses, metadata, packages, profiles, scripts
			if ( ! ((*itCategory).contains( '-' ) || *itCategory == "virtual" ) )
				continue;

            /*// Abort the scan
			if ( isAborted() ) {
				qWarning() << "Scanning Portage. Portage scan aborted";
				KurooDBSingleton::Instance()->singleQuery( "ROLLBACK TRANSACTION;", m_db );
                return;
            }*/

			//qDebug() << "Scanning Portage. Reading category " << *itCategory;
			QString category = (*itCategory).section( "-", 0, 0 );
			QString subCategory = (*itCategory).section( "-", 1, 1 );

			if( lastCategory.isEmpty() ) {
				//if this is our first pass and lastCategory is empty, check the database for this category to avoid inserting a duplicate
				const QStringList lines = KurooDBSingleton::Instance()->query( QString(
							"SELECT name FROM category_temp WHERE name = '%1';").arg( category ), m_db );
				if( !lines.isEmpty() ) {
					lastCategory = category;
				}
			}
			if ( lastCategory != category )
				idCategory = KurooDBSingleton::Instance()->insert( QString(
					"INSERT INTO category_temp (name) VALUES ('%1');" ).arg( category ), m_db );

			int idSubCategory = KurooDBSingleton::Instance()->insert( QString(
				"INSERT INTO subCategory_temp (name, idCategory) VALUES ('%1', '%2');")
			.arg( subCategory ).arg( QString::number( idCategory ) ), m_db );

			// Get list of packages in this category
			dPackage.setFilter( QDir::Files | QDir::NoSymLinks );
			dPackage.setSorting( QDir::Name );

			if ( dPackage.cd( *itPath + "/" + *itCategory) ) {
				//qDebug() << "Scanning Portage. CD'ed into " << *itPath << "/" << *itCategory;

				QStringList packageList = dPackage.entryList();
				QString status, lastPackage;
				for ( QStringList::Iterator itPackage = packageList.begin(), itPackageEnd = packageList.end(); itPackage != itPackageEnd; ++itPackage ) {

					//That metadata.xml seems idempotent, it only exists in the real tree, not the cache tree
					if ( *itPackage == "." || *itPackage == ".." || *itPackage == "metadata.xml" || *itPackage == "Manifest.gz" || (*itPackage).contains("MERGING") )
						continue;

                    /*// Abort the scan
					if ( isAborted() ) {
						qWarning() << "Scanning Portage. Portage scan aborted";
						KurooDBSingleton::Instance()->singleQuery( "ROLLBACK TRANSACTION;", m_db );
                        return;
                    }*/

					//qDebug() << "Scanning Portage. Reading package " << *itPackage;
					QStringList parts = parsePackage( *itPackage );
					if ( !parts.isEmpty() ) {
						//parts[0] is category
						QString name = parts[1];
						QString version = parts[2];

						Info info( scanInfo( *itPath, *itCategory, name, version ) );

						// Insert category and db id's in portage
						if ( !m_categories.contains( *itCategory ) ) {
							m_categories[ *itCategory ].idCategory = QString::number( idCategory );
							m_categories[ *itCategory ].idSubCategory = QString::number( idSubCategory );
						}

						// Insert package in portage
						if ( !m_categories[ *itCategory ].packages.contains( name ) ) {
							m_categories[ *itCategory ].packages[ name ];
							m_categories[ *itCategory ].packages[ name ].status = PACKAGE_AVAILABLE_STRING;
							m_categories[ *itCategory ].packages[ name ].description = info.description;
							m_categories[ *itCategory ].packages[ name ].path = (*itPath).section( "/metadata/md5-cache", 0, 0 );
							//qDebug() << "Inserting package " << name << " into portage with path " << m_categories[*itCategory].packages[name].path;
						}

						// Insert version in portage
						if ( !m_categories[ *itCategory ].packages[ name ].versions.contains( version ) ) {
							m_categories[ *itCategory ].packages[ name ].versions[ version ].description = info.description;
							m_categories[ *itCategory ].packages[ name ].versions[ version ].homepage = info.homepage;
							m_categories[ *itCategory ].packages[ name ].versions[ version ].status = PACKAGE_AVAILABLE_STRING;
							m_categories[ *itCategory ].packages[ name ].versions[ version ].licenses = info.licenses;
							m_categories[ *itCategory ].packages[ name ].versions[ version ].useFlags = info.useFlags;
							m_categories[ *itCategory ].packages[ name ].versions[ version ].slot = info.slot;
							m_categories[ *itCategory ].packages[ name ].versions[ version ].size = info.size;
							m_categories[ *itCategory ].packages[ name ].versions[ version ].keywords = info.keywords;
						}

					}
					else
						qWarning() << "Scanning Portage. Scanning Portage cache: can not match package " << *itPackage;

					// Post scan count progress
					//if ( ( ++count % 100 ) == 0 )
				}
			}
			else
				qWarning() << "Scanning Portage. Can not access " << *itPath << *itCategory;

			lastCategory = category;
			SignalistSingleton::Instance()->scanProgress(++count);
		}
	}

	// Now scan installed packages, eg mark packages as installed and add "old" packages (not in Portage anymore)
	scanInstalledPackages();

	// Iterate through portage map and insert everything in db
	PortageCategories::iterator itCategoryEnd = m_categories.end();
	for ( PortageCategories::iterator itCategory = m_categories.begin(); itCategory != itCategoryEnd; ++itCategory ) {

        PortagePackages::iterator itPackageEnd = itCategory.value().packages.end();
        for ( PortagePackages::iterator itPackage = itCategory.value().packages.begin(); itPackage != itPackageEnd; ++itPackage ) {

			QString idPackage;
            QString idCategory = itCategory.value().idCategory;
            QString idSubCategory = itCategory.value().idSubCategory;

			QString category = itCategory.key();
			QString package = itPackage.key();
            QString status = itPackage.value().status;
            QString description = itPackage.value().description;
            QString path = itPackage.value().path;

			// Create meta tag containing all text of interest for searching
			QString meta = category + " " + package + " " + description;

			QString sql = QString( "INSERT INTO package_temp (idCategory, idSubCategory, category, "
			                       "name, description, status, path, meta) "
			                       "VALUES ('%1', '%2', '%3', '%4', '%5', '%6', '%7', '%8');")
				.arg( idCategory ).arg( idSubCategory ).arg( category ).arg( package )
				.arg( description ).arg( status ).arg( path ).arg( meta );

			idPackage = QString::number( KurooDBSingleton::Instance()->insert( sql, m_db ) );

            PortageVersions::iterator itVersionEnd = itPackage.value().versions.end();
            for ( PortageVersions::iterator itVersion = itPackage.value().versions.begin(); itVersion != itVersionEnd; ++itVersion ) {

				QString version = itVersion.key();
                description = itVersion.value().description;
                QString homepage = itVersion.value().homepage;
                QString status = itVersion.value().status;
                QString licenses = itVersion.value().licenses;
                QString useFlags = itVersion.value().useFlags;
                QString slot = itVersion.value().slot;
                QString size = itVersion.value().size;
                QString keywords = itVersion.value().keywords;

				QString sqlVersion = QString( "INSERT INTO version_temp "
				                              "(idPackage, name, description, homepage, size, keywords, status, licenses, useFlags, slot) "
				                              "VALUES ('%1', '%2', '%3', '%4', '%5', '%6', '%7', '%8', '%9', " )
					.arg( idPackage ).arg( version ).arg( description ).arg( homepage ).arg( size )
					.arg( keywords ).arg( status ).arg( licenses ).arg( useFlags );

				sqlVersion += QString( "'%1');" ).arg( slot );
				KurooDBSingleton::Instance()->insert( sqlVersion, m_db );
			}
		}
	}
	m_categories.clear();

	KurooDBSingleton::Instance()->singleQuery( "COMMIT TRANSACTION;", m_db );
	KurooDBSingleton::Instance()->singleQuery( QString("UPDATE dbInfo SET data = '%1' WHERE meta = 'packageCount';").arg( count ), m_db );

	// Move content from temporary table
	KurooDBSingleton::Instance()->singleQuery( "DELETE FROM category;", m_db );
	KurooDBSingleton::Instance()->singleQuery( "DELETE FROM subCategory;", m_db );
	KurooDBSingleton::Instance()->singleQuery( "DELETE FROM package;", m_db );
	KurooDBSingleton::Instance()->singleQuery( "DELETE FROM version;", m_db );

	KurooDBSingleton::Instance()->singleQuery( "BEGIN TRANSACTION;", m_db );
	KurooDBSingleton::Instance()->insert( "INSERT INTO category SELECT * FROM category_temp;", m_db );
	KurooDBSingleton::Instance()->insert( "INSERT INTO subCategory SELECT * FROM subCategory_temp;", m_db );
	KurooDBSingleton::Instance()->insert( "INSERT INTO package SELECT * FROM package_temp;", m_db );
	KurooDBSingleton::Instance()->insert( "INSERT INTO version SELECT * FROM version_temp;", m_db );
	KurooDBSingleton::Instance()->singleQuery( "COMMIT TRANSACTION;", m_db );

	KurooDBSingleton::Instance()->singleQuery( "DROP TABLE category_temp;", m_db );
	KurooDBSingleton::Instance()->singleQuery( "DROP TABLE subCategory_temp;", m_db );
	KurooDBSingleton::Instance()->singleQuery( "DROP TABLE package_temp;", m_db );
	KurooDBSingleton::Instance()->singleQuery( "DROP TABLE version_temp;", m_db );

    /*setStatus( "ScanPortage", i18n("Done.") );
    setProgressTotalSteps( 0 );*/

	SignalistSingleton::Instance()->scanPortageComplete();
}

/**
 * Collect list of installed packages.
 */
void ScanPortageJobImpl::scanInstalledPackages()
{
	QDir dCategory, dPackage;
	dCategory.setFilter( QDir::Dirs | QDir::NoSymLinks );
	dCategory.setSorting( QDir::Name );

	if ( !dCategory.cd( KurooConfig::dirDbPkg() ) )
		qWarning() << "Scanning installed packages. Can not access " << KurooConfig::dirDbPkg();

    //setStatus( "ScanInstalled", i18n("Collecting installed packages...") );

	// Get list of categories for installed packages
	QStringList categoryList = dCategory.entryList();
	QStringList::Iterator itCategoryEnd = categoryList.end();
	for ( QStringList::Iterator itCategory = categoryList.begin(); itCategory != itCategoryEnd; ++itCategory ) {

		if ( *itCategory == "." || *itCategory == ".." )
			continue;

		// Get list of packages in this category
		dPackage.setFilter( QDir::Dirs | QDir::NoSymLinks );
		dPackage.setSorting( QDir::Name );

		if ( dPackage.cd( KurooConfig::dirDbPkg() + "/" + *itCategory ) ) {
			QStringList packageList = dPackage.entryList();
			QStringList::Iterator itPackageEnd = packageList.end();
			for ( QStringList::Iterator itPackage = packageList.begin(); itPackage != itPackageEnd; ++itPackage ) {

				if ( *itPackage == "." || *itPackage == ".." || ( *itPackage ).contains("MERGING") )
					continue;

                QStringList parts = parsePackage( *itPackage );
				if ( !parts.isEmpty() ) {
					QString name = parts[1];
					QString version = parts[2];

					// Insert category if not found in portage
					if ( !m_categories.contains( *itCategory ) )
						m_categories[ *itCategory ];

					// Insert and/or mark package as installed (old is package not in portage anymore)
					if ( !m_categories[ *itCategory ].packages.contains( name ) ) {
						m_categories[ *itCategory ].packages[ name ];
						m_categories[ *itCategory ].packages[ name ].status = PACKAGE_OLD_STRING;
					}
					else
						m_categories[ *itCategory ].packages[ name ].status = PACKAGE_INSTALLED_STRING;

					// Insert old version in portage
					if ( !m_categories[ *itCategory ].packages[ name ].versions.contains( version ) )
						m_categories[ *itCategory ].packages[ name ].versions[ version ];

					// Mark version as installed
					m_categories[ *itCategory ].packages[ name ].versions[ version ].status = PACKAGE_INSTALLED_STRING;

				}
				else
					qWarning() << "Scanning installed packages. Can not match " << *itPackage;
			}
		}
		else
			qWarning() << "Scanning installed packages. Can not access " << KurooConfig::dirDbPkg() << "/" << *itCategory;
	}
    //setStatus( "ScanInstalled", i18n("Done.") );
}

/**
 * Collect info about this ebuild. Based on Jakob Petsovits code.
 * @param path		base path to portage directory (like /usr/portage/metadata/md5-cache)
 * @param category	category name (like app-portage)
 * @param name		package name (like kuroo)
 * @param version
 * @return  false if the file can't be opened, true otherwise.
 */
Info ScanPortageJobImpl::scanInfo( const QString& path, const QString& category, const QString& name, const QString& version )
{
//WARN: This won't work for anything but /usr/portage for now!
	Info info;
	QFile file( path + "/" + category + "/" + name + "-" + version );

	if ( !file.open( QIODevice::ReadOnly ) ) {
		qWarning() << "Scanning Portage cache. Failed reading: " << path << "/" << category << "/" << name << "-" << version;

		info.slot = "0";
		info.homepage = "0";
		info.licenses = "0";
		info.description = "0";
		info.keywords = "0";
		info.useFlags = "0";
		info.size = "0";
		return info;
	}

	QString line;
	QTextStream stream( &file );

	// Check portage version and read out the package info strings
// 	if ( KurooConfig::portageVersion21() ) {
//
		// We are on portage version post 2.1
		while ( !stream.atEnd() ) {
			line = stream.readLine();

			if ( line.startsWith( "LICENSE=" ) )
				info.licenses = line.section("LICENSE=", 1, 1).replace('\'', "''").replace('%', "&#37;");
			else
				if ( line.startsWith( "KEYWORDS=" ) )
					info.keywords = line.section("KEYWORDS=", 1, 1);
				else
					if ( line.startsWith( "SLOT=" ) )
						info.slot = line.section("SLOT=", 1, 1);
					else
						if ( line.startsWith( "DESCRIPTION=" ) )
							info.description = line.section("DESCRIPTION=", 1, 1).replace('\'', "''").replace('%', "&#37;");
						else
							if ( line.startsWith( "IUSE=" ) )
								info.useFlags = line.section("IUSE=", 1, 1);
							else
								if ( line.startsWith( "HOMEPAGE=" ) )
									info.homepage = line.section("HOMEPAGE=", 1, 1).replace('\'', "''").replace('%', "&#37;");
		}
//	}
//	else {
	//int lineNumber(0);
//WARN: Portage seems to have down-graded to the older style flat cache file for the cache in the repository (/usr/portage).  Other caches (layman) may behave differently
		// We are on portage version pre 2.1
// 		while ( !stream.atEnd() ) {
// 			line = stream.readLine();
// 			lineNumber++;
//
// 			// each line has a fixed meaning, as it seems.
// 			// so iterate through the lines.
// 			switch( lineNumber ) {
// 				case 1: // compile? dependency stuff
// 					break;
// 				case 2: // runtime? dependency stuff
// 					break;
// 				case 3: // the package slot
// 					info.slot = line;
// 					break;
// 				case 4: // file location, starting with mirror://
// 					break;
// 				case 5: // empty?
// 					break;
// 				case 6: // DirHome page
// 					info.homepage = line.replace('\'', "''").replace('%', "&#37;");
// 					break;
// 				case 7: // licenses
// 					info.licenses = line.replace('\'', "''").replace('%', "&#37;");
// 					break;
// 				case 8: // description
// 					info.description = line.replace('\'', "''").replace('%', "&#37;");
// 					break;
// 				case 9: // keywords
// 					info.keywords = line;
// 					break;
// 				case 10: // inherited eclasses?
// 					break;
// 				case 11: // useFlags
// 					info.useFlags = line;
// 					break;
// 				default:
// 					break;
// 			}
// 		}
//	}
	file.close();
/*
//TODO: This is too difficult at the moment, since the change to Manifest2 format in GLEP 44, we don't have files/digest-* files
//to read.  The data that we need is in the Manifest2 file, but the lines no longer say the ebuild/version number, instead
//it has a file name, so we would have to parse the ebuid to find out what lines to read from the Manifest2 file.
	// Get package size. Try in cache first.
	QString size = cacheFind( category + "/" + name + "-" + version ) ;
	if ( !size.isEmpty() ) {
		info.size = formatSize( size );
	} else {
		QString path = KurooConfig::dirPortage() + "/" + category + "/" + name + "/files/digest-" + name + "-" + version;
		file.setName( path );
		if ( file.open( QIODevice::ReadOnly ) ) {
			std::ifstream in( path );
			std::string word;
			while ( in >> word );
			file.close();
			info.size = formatSize( word );

			// Add new value into cache.
			KurooDBSingleton::Instance()->insert( QString("INSERT INTO cache (package, size) VALUES ('%1', '%2');")
			                                      .arg( name + "-" + version ).arg( word ), m_db );
		}
		else
			qCritical() << "Scanning installed packages. Reading: " << path;
	}
*/

	return info;
}

/**
 * Format package size nicely
 * @fixme: Check out KIO_EXPORT QString KIO::convertSize
 * @param size
 * @return total		as "xxx kB"
 */
QString ScanPortageJobImpl::formatSize( const QString& size )
{
	KLocale *loc = KLocale::global();
	QString total;

	uint num = ("0" + size).toInt();
	if ( num < 1024 )
		total = "1 kB ";
	else
		total = loc->formatNumber((double)(num / 1024), 0) + " kB ";

	return total;
}

/**
 * Load m_mapCache with items from DB.
 */
void ScanPortageJobImpl::loadCache()
{
	m_mapCache.clear();
	const QStringList cacheList = KurooDBSingleton::Instance()->query( "SELECT package, size FROM cache ;", m_db );
    QStringListIterator it( cacheList );
    while( it.hasNext() ) {
        QString package = it.next();
        QString size = it.next();
		m_mapCache.insert( package, size );
	}
}

/**
 * Find cached size for package.
 * @param packages
 * @return size or NULL if na
 */
QString ScanPortageJobImpl::cacheFind( const QString& package )
{
	QMap<QString, QString>::iterator it = m_mapCache.find( package ) ;
	if ( it != m_mapCache.end() )
        return it.value();
	else
		return QString::null;
}

#include "scanportagejob.moc"
