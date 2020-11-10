/**************************************************************************
*	Copyright (C) 2004 by karye												*
*	karye@users.sourceforge.net												*
*	From Amarok code.														*
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

#include <math.h>		//DbConnection::sqlite_power()
#include <time.h>		//query()
#include <unistd.h>		//usleep()
#include <stdlib.h>		//RAND_MAX
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>

#include <QFile>
#include <QTextStream>

#include <kio/job.h>
#include <kio/netaccess.h>
#include <KRandom>

/**
 * @class KurooDB
 * @short Handle database connections and queries.
 */
KurooDB::KurooDB ( QObject *m_parent )
		: QObject ( m_parent )
{}

KurooDB::~KurooDB()
{
	destroy();
}

/**
 * Check db integrity and create new db if necessary.
 * Set write permission for regular user.
 * @return database file
 */
QString KurooDB::init( QObject *parent )
{
	m_parent = parent;

	m_dbConnPool = new DbConnectionPool();
	DbConnection *dbConn = m_dbConnPool->getDbConnection();
	m_dbConnPool->putDbConnection( dbConn );

	if ( !dbConn->isInitialized() || !isValid() )
		createTables();

	m_dbConnPool->createDbConnections();

	return kurooDir + KurooConfig::databas();
}

DbConnection *KurooDB::getStaticDbConnection()
{
// 	qDebug() << "KurooDB::getStaticDbConnection-------------";
	return m_dbConnPool->getDbConnection();
}

void KurooDB::returnStaticDbConnection( DbConnection *conn )
{
// 	qDebug() << "--------------KurooDB::returnStaticDbConnection ";
	m_dbConnPool->putDbConnection ( conn );
}

void KurooDB::destroy()
{
	delete m_dbConnPool;
}

/**
 * Executes a SQL query on the already opened database
 * @param statement SQL program to execute. Only one SQL statement is allowed.
 * @return		The queried data, or QStringList() on error.
 */
QStringList KurooDB::query( const QString& statement, DbConnection *conn )
{
// 	qDebug() << "Query-start:" << statement;
// 	clock_t start = clock();

	DbConnection *dbConn;
	if ( conn != NULL )
		dbConn = conn;
	else
		dbConn = m_dbConnPool->getDbConnection();

	QStringList values = dbConn->query ( statement );

	if ( conn == NULL )
		m_dbConnPool->putDbConnection ( dbConn );

// 	clock_t finish = clock();
// 	const double duration = (double) ( finish - start ) / CLOCKS_PER_SEC;
// 	qDebug() << "Query-end:" << statement;

	return values;
}

/**
 * Executes a SQL query on the already opened database
 * @param statement SQL program to execute. Only one SQL statement is allowed.
 * @return		The queried data, or QStringList() on error.
 */
QString KurooDB::singleQuery( const QString& statement, DbConnection *conn )
{
// 	qDebug() << "Query-start:" << statement;
// 	clock_t start = clock();

	DbConnection *dbConn;
	if ( conn != NULL )
		dbConn = conn;
	else
		dbConn = m_dbConnPool->getDbConnection();

	QString value = dbConn->singleQuery ( statement );

	if ( conn == NULL )
		m_dbConnPool->putDbConnection ( dbConn );

// 	clock_t finish = clock();
// 	const double duration = (double) ( finish - start ) / CLOCKS_PER_SEC;
// 	qDebug() << "Query-end:" << statement;

	return value;
}

/**
 * Executes a SQL insert on the already opened database
 * @param statement SQL statement to execute. Only one SQL statement is allowed.
 * @return		The rowid of the inserted item.
 */
int KurooDB::insert( const QString& statement, DbConnection *conn )
{
// 	qDebug() << "insert-start: " << statement;
// 	clock_t start = clock();

	DbConnection *dbConn;
	if ( conn != NULL )
		dbConn = conn;
	else
		dbConn = m_dbConnPool->getDbConnection();

	int id = dbConn->insert ( statement );

	if ( conn == NULL )
		m_dbConnPool->putDbConnection ( dbConn );

// 	clock_t finish = clock();
// 	const double duration = (double) (finish - start) / CLOCKS_PER_SEC;
// 	qDebug() << "SQL-insert (" << duration << "s): " << statement;

	return id;
}


bool KurooDB::isCacheEmpty()
{
	QString values = singleQuery ( "SELECT COUNT(id) FROM cache LIMIT 0, 1;" );
	return values.isEmpty() ? true : values == "0";
}

bool KurooDB::isPortageEmpty()
{
	QString values = singleQuery ( "SELECT COUNT(id) FROM package LIMIT 0, 1;" );
	return values.isEmpty() ? true : values == "0";
}

bool KurooDB::isQueueEmpty()
{
	QString values = singleQuery ( "SELECT COUNT(id) FROM queue LIMIT 0, 1;" );
	return values.isEmpty() ? true : values == "0";
}

bool KurooDB::isUpdatesEmpty()
{
	QString values = singleQuery ( QString ( "SELECT COUNT(id) FROM package where status = '%1' LIMIT 0, 1;" )
									.arg ( PACKAGE_UPDATES_STRING ) );
	return values.isEmpty() ? true : values == "0";
}

bool KurooDB::isHistoryEmpty()
{
	QString values = singleQuery ( "SELECT COUNT(id) FROM history LIMIT 0, 1;" );
	return values.isEmpty() ? true : values == "0";
}

bool KurooDB::isValid()
{
	QString value1 = singleQuery ( "SELECT COUNT(id) FROM category LIMIT 0, 1;" );
	QString value2 = singleQuery ( "SELECT COUNT(id) FROM package LIMIT 0, 1;" );
	//A lot of times when the database gets corrupted, it has one value int it, and we know that there should
	//always be more than one.
	return ( !value1.isEmpty() && value1.toInt() != 1 && !value2.isEmpty() && value2.toInt() != 1 );
}

/**
 * Create all necessary tables.
 */
void KurooDB::createTables( DbConnection *conn )
{
	qDebug() << "Creating tables";
	query ( "CREATE TABLE dbInfo ( "
			"meta VARCHAR(64), "
			"data VARCHAR(64) );"
			, conn );

	query ( " INSERT INTO dbInfo (meta, data) VALUES ('syncTimeStamp', '0');", conn );
	query ( " INSERT INTO dbInfo (meta, data) VALUES ('packageCount', '0');", conn );
	query ( " INSERT INTO dbInfo (meta, data) VALUES ('scanDuration', '100');", conn );

	query ( "CREATE TABLE category ( "
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"name VARCHAR(32) UNIQUE );"
			, conn );

	query ( "CREATE TABLE subCategory ( "
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"name VARCHAR(32), "
			"idCategory INTEGER );"
			, conn );

	query ( "CREATE TABLE package ( "
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"idCategory INTEGER, "
			"idSubCategory INTEGER, "
			"category VARCHAR(32), "
			"name VARCHAR(32), "
			"description VARCHAR(255), "
			"path VARCHAR(64), "
			"status INTEGER, "
			"meta VARCHAR(255), "
			"updateVersion VARCHAR(32) ); "
			, conn );

	query ( "CREATE TABLE version ( "
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"idPackage INTEGER, "
			"name VARCHAR(32), "
			"description VARCHAR(255), "
			"homepage VARCHAR(128), "
			"licenses VARCHAR(64), "
			"useFlags VARCHAR(255), "
			"slot VARCHAR(32), "
			"size VARCHAR(32), "
			"status INTEGER, "
			"keywords VARCHAR(32) );"
			, conn );

	query ( "CREATE TABLE queue ( "
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"idPackage INTEGER, "
			"idDepend INTEGER, "
			"use VARCHAR(255), "
			"size VARCHAR(32), "
			"version VARCHAR(32) );"
			, conn );

	query ( "CREATE TABLE history ( "
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"package VARCHAR(32), "
			"timestamp VARCHAR(10), "
			"time INTEGER, "
			"einfo BLOB, "
			"emerge BOOL );"
			, conn );

	query ( "CREATE TABLE mergeHistory ( "
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"timestamp VARCHAR(10), "
			"source VARCHAR(255), "
			"destination VARCHAR(255) );"
			, conn );

	query ( " CREATE TABLE statistic ( "
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"package VARCHAR(32), "
			"time INTEGER, "
			"count INTEGER );"
			, conn );

	query ( "CREATE TABLE cache ( "
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"package VARCHAR(32), "
			"size VARCHAR(10) );"
			, conn );

	query ( "CREATE TABLE packageHardMask ( "
			"idPackage INTEGER, "
			"dependAtom VARCHAR(255), "
			"comment BLOB );"
			, conn );

	query ( "CREATE TABLE packageUserMask ( "
			"idPackage INTEGER UNIQUE, "
			"dependAtom VARCHAR(255), "
			"comment BLOB );"
			, conn );

	query ( "CREATE TABLE packageUnmask ( "
			"idPackage INTEGER UNIQUE, "
			"dependAtom VARCHAR(255), "
			"comment BLOB );"
			, conn );

	query ( "CREATE TABLE packageKeywords ( "
			"idPackage INTEGER UNIQUE, "
			"keywords VARCHAR(255) );"
			, conn );

	query ( "CREATE TABLE packageUse ( "
			"idPackage INTEGER UNIQUE, "
			"use VARCHAR(255) );"
			, conn );

	query ( "CREATE INDEX index_name_package ON package(name);", conn );
	query ( "CREATE INDEX index_category_package ON package(category);", conn );
	query ( "CREATE INDEX index_history_timestamp ON history(timestamp);", conn );
	query ( "CREATE INDEX index_package_id ON package(id);", conn );
	query ( "CREATE INDEX index_version_packageId ON version(packageId);", conn );
}


//////////////////////////////////////////////////////////////////////////////
// Database management
//////////////////////////////////////////////////////////////////////////////

/**
 * Backup to file data which can not be recreated, fex history.einfo and mergeHistory.source/destination
 */
void KurooDB::backupDb()
{
	const QStringList historyData = query ( "SELECT timestamp, einfo FROM history WHERE einfo > ''; " );
	if ( !historyData.isEmpty() )
	{
		QFile file ( kurooDir + KurooConfig::fileHistoryBackup() );
		if ( file.open ( QIODevice::WriteOnly ) )
		{
			QTextStream stream ( &file );
			QStringList::const_iterator i = historyData.begin();
			QStringList::const_iterator e = historyData.end();
			while ( i != e )
			{
				QString timestamp = *i++;
				QString einfo = *i++;
				stream << timestamp << ":" << einfo << "\n";
			}
			file.close();
		}
		else
			qCritical() << QString ( "Creating backup of history. Writing: %1." ).arg ( KurooConfig::fileHistoryBackup() );
	}

	const QStringList mergeData = query ( "SELECT timestamp, source, destination FROM mergeHistory;" );
	if ( !mergeData.isEmpty() )
	{
		QFile file ( kurooDir + KurooConfig::fileMergeBackup() );
		if ( file.open ( QIODevice::WriteOnly ) )
		{
			QTextStream stream ( &file );
			QStringList::const_iterator i = mergeData.begin();
			QStringList::const_iterator e = mergeData.end();
			//foreach ( mergeData )
			while ( i != e )
			{
				QString timestamp = *i++;
				QString source = *i++;
				QString destination = *i++;
				stream << timestamp << ":" << source << ":" << destination << "\n";
			}
			file.close();
		}
		else
			qCritical() << QString ( "Creating backup of history. Writing: %1." ).arg ( KurooConfig::fileMergeBackup() );
	}
}

/**
 * Restore data to tables history and mergeHistory
 */
void KurooDB::restoreBackup()
{
	/*// Restore einfo into table history
	QFile file ( GlobalSingleton::Instance()->kurooDir() + KurooConfig::fileHistoryBackup() );
	QTextStream stream ( &file );
	QStringList lines;
	if ( !file.open ( QIODevice::ReadOnly ) )
		qCritical() << QString ( "Restoring backup of history. Reading: %1." ).arg ( KurooConfig::fileHistoryBackup() );
	else
	{
		while ( !stream.atEnd() )
			lines += stream.readLine();
		file.close();
	}

	QRegExp rxHistoryLine ( "(\\d+):((?:\\S|\\s)*)" );
	for ( QStringList::Iterator it = lines.begin(), end = lines.end(); it != end; ++it )
	{
		if ( ! ( *it ).isEmpty() && rxHistoryLine.exactMatch ( *it ) )
		{
			QString timestamp = rxHistoryLine.cap ( 1 );
			QString einfo = rxHistoryLine.cap ( 2 );
			singleQuery ( "UPDATE history SET einfo = '" + escapeString ( einfo ) + "' WHERE timestamp = '" + timestamp + "';" );
		}
	}

	// Restore source and destination into table mergeHistory
	file.setName ( GlobalSingleton::Instance()->kurooDir() + KurooConfig::fileMergeBackup() );
	stream.setDevice ( &file );
	lines.clear();
	if ( !file.open ( QIODevice::ReadOnly ) )
		qCritical() << QString ( "Restoring backup of history. Reading: %1." ).arg ( KurooConfig::fileMergeBackup() );
	else
	{
		while ( !stream.atEnd() )
			lines += stream.readLine();
		file.close();
	}

	QRegExp rxMergeLine ( "(\\d+):((?:\\S|\\s)*):((?:\\S|\\s)*)" );
	for ( QStringList::Iterator it = lines.begin(), end = lines.end(); it != end; ++it )
	{
		if ( ! ( *it ).isEmpty() && rxMergeLine.exactMatch ( *it ) )
		{
			QString timestamp = rxMergeLine.cap ( 1 );
			QString source = rxMergeLine.cap ( 2 );
			QString destination = rxMergeLine.cap ( 3 );
			singleQuery ( "INSERT INTO mergeHistory (timestamp, source, destination) "
						  "VALUES ('" + timestamp + "', '" + source + "', '" + destination + "');" );
		}
	}*/
}


//////////////////////////////////////////////////////////////////////////////
// Queries for kuroo
//////////////////////////////////////////////////////////////////////////////

/**
 * Return db meta data.
 */
QString KurooDB::getKurooDbMeta ( const QString& meta )
{
	return singleQuery ( "SELECT data FROM dbInfo WHERE meta = '" + meta + "' LIMIT 1;" );
}

/**
 * Set db meta data.
 * @param version
 */
void KurooDB::setKurooDbMeta ( const QString& meta, const QString& data )
{
	if ( singleQuery ( "SELECT COUNT(meta) FROM dbInfo WHERE meta = '" + meta + "' LIMIT 1;" ) == "0" )
		insert ( "INSERT INTO dbInfo (meta, data) VALUES ('" + meta + "', '"+ data + "');" );
	else
		singleQuery ( "UPDATE dbInfo SET data = '" + data + "' WHERE meta = '" + meta + "';" );
}

//////////////////////////////////////////////////////////////////////////////
// Queries for portage
//////////////////////////////////////////////////////////////////////////////

/**
 * Return all categories, eg app in app-portage/kuroo.
 * "0" is appended in front of the list so as not to miss the first category when categories are entered in reverse order in listview.
 */
const QStringList KurooDB::allCategories()
{
	QStringList resultList ( "0" );
	resultList += query ( "SELECT name FROM category; " );
	return resultList;
}

/**
 * Return all subcategories, eg portage in app-portage/kuroo.
 */
const QStringList KurooDB::allSubCategories()
{
	return query ( "SELECT idCategory, id, name FROM subCategory ORDER BY name; " );
}

/**
 * Return all categories which have packages matching the filter and the text.
 * @param filter	packages status as PACKAGE_AVAILABLE, PACKAGE_INSTALLED or PACKAGE_UPDATES
 * @param text		search string
 */
const QStringList KurooDB::portageCategories( int filter, const QString& text )
{
	QString filterQuery, textQuery;
//	int len;

	// Allow for multiple words match
	QString textString = escapeString ( text.simplified() );
	QStringList textStringList = textString.split(" ");

	// Concatenate all search words
	if ( !textStringList.isEmpty() )
	{
		while ( !textStringList.isEmpty() )
		{
			if (textStringList.first().length() > 0)
			{
				textQuery += " AND (meta LIKE '%" + textStringList.takeFirst() + "%')";
			}
			else
				textStringList.takeFirst();
		}
		/*len = textQuery.length();
		if (len > 0)
			textQuery = "( " + textQuery.right ( len - 5 ) + " ) ";
		else
			textQuery = "";*/
	}

	switch ( filter )
	{
		case PACKAGE_AVAILABLE:
			filterQuery = " WHERE package.status & " + PACKAGE_ALL_STRING;
			break;

		case PACKAGE_INSTALLED:
			filterQuery = " WHERE package.status & " + PACKAGE_INSTALLED_UPDATES_OLD_STRING;
			break;

		case PACKAGE_UPDATES:
			filterQuery = " WHERE package.status & " + PACKAGE_UPDATES_STRING;
	}

	return query ( "SELECT DISTINCT idCategory FROM package " + filterQuery + textQuery + " ; " );
}

/**
 * Return all subcategories which have packages matching the filter and the text in this category.
 * @param categoryId 	category id
 * @param filter		packages status as PACKAGE_AVAILABLE, PACKAGE_INSTALLED or PACKAGE_UPDATES
 * @param text			search string
 */
const QStringList KurooDB::portageSubCategories( const QString& categoryId, int filter, const QString& text )
{
	QString filterQuery, textQuery;
	QStringList resultList ( categoryId );
	int len;

	// Allow for multiple words match
	QString textString = escapeString ( text.simplified() );
	QStringList textStringList = textString.split(" ");

	// Concatenate all search words
	if ( !textStringList.isEmpty() )
	{
		while ( !textStringList.isEmpty() )
		{
			textQuery += " AND meta LIKE '%" + textStringList.first() + "%' ";
			textStringList.pop_front();
		}
		len = textQuery.length();
		textQuery = " AND ( " + textQuery.right ( len - 5 ) + " ) ";
	}

	if ( categoryId != "0" )
	{

		switch ( filter )
		{
			case PACKAGE_AVAILABLE:
				filterQuery = " AND package.status & " + PACKAGE_ALL_STRING;
				break;

			case PACKAGE_INSTALLED:
				filterQuery = " AND package.status & " + PACKAGE_INSTALLED_UPDATES_OLD_STRING;
				break;

			case PACKAGE_UPDATES:
				filterQuery = " AND package.status & " + PACKAGE_UPDATES_STRING;
		}

		resultList += query ( " SELECT DISTINCT idSubCategory FROM package WHERE idCategory = '"
							  + categoryId + "'" + filterQuery + textQuery + " ; " );
	}

	// Add meta-subcategory when query is successful
	if ( resultList.size() > 1 )
		resultList += "0";

	return resultList;
}

/**
 * Return all packages which are matching the filter and the text in this category-subcategory.
 * @param categoryId	category id
 * @param subCategoryId	subcategory id
 * @param filter		packages status as PACKAGE_AVAILABLE, PACKAGE_INSTALLED or PACKAGE_UPDATES
 * @param text			search string
 */
const QStringList KurooDB::portagePackagesBySubCategory( const QString& categoryId, const QString& subCategoryId, int filter, const QString& text )
{
	qDebug() << "filter = " << filter;
	QString filterQuery, textQuery;
//	int len;

	// Allow for multiple words match
	QString textString = escapeString( text.simplified() );
	QStringList textStringList = textString.split(" ");

	// Concatenate all search words
	if ( !textStringList.isEmpty() )
	{
		while ( !textStringList.isEmpty() )
		{
			if (textStringList.first().length() > 0)
				textQuery += " AND (meta LIKE '%" + textStringList.first() + "%')";
			textStringList.pop_front();
		}
		/*len = textQuery.length();
		if (len > 0)
			textQuery = " AND ( " + textQuery.right( len - 5 ) + " ) ";
		else
			textQuery = "";*/
	}

	switch ( filter )
	{
		case PACKAGE_AVAILABLE:
			filterQuery = " AND package.status & " + PACKAGE_ALL_STRING;
			break;

		case PACKAGE_INSTALLED:
			filterQuery = " AND package.status & " + PACKAGE_INSTALLED_UPDATES_OLD_STRING;
			break;

		case PACKAGE_UPDATES:
			filterQuery = " AND package.status & " + PACKAGE_UPDATES_STRING;
	}

	if ( categoryId == "0" )
	{

		if ( subCategoryId == "0" )
		{
			switch ( filter )
			{
				case PACKAGE_AVAILABLE:
					filterQuery = " WHERE package.status & " + PACKAGE_ALL_STRING;
					break;

				case PACKAGE_INSTALLED:
					filterQuery = " WHERE package.status & " + PACKAGE_INSTALLED_UPDATES_OLD_STRING;
					break;

				case PACKAGE_UPDATES:
					filterQuery = " WHERE package.status & " + PACKAGE_UPDATES_STRING;
			}

			return query( " SELECT id, name, category, description, status, updateVersion "
							" FROM package "
							+ filterQuery + textQuery + " ORDER BY name DESC;" );
		}
		else
		{

			return query( " SELECT id, name, category, description, status, updateVersion "
							" FROM package "
							" WHERE idSubCategory = '" + subCategoryId + "'"
							+ filterQuery + textQuery + " ORDER BY name DESC;" );
		}
	}
	else
	{
		if ( subCategoryId == "0" )
		{

			return query( " SELECT id, name, category, description, status, updateVersion "
							" FROM package "
							" WHERE idCategory = '" + categoryId + "'"
							+ filterQuery + textQuery + " ORDER BY name DESC;" );
		}
		else
		{

			return query( " SELECT id, name, category, description, status, updateVersion "
							" FROM package "
							" WHERE idCategory = '" + categoryId + "'"
							" AND idSubCategory = '" + subCategoryId + "'"
							+ filterQuery + textQuery + " ORDER BY name DESC;" );
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
// Query for packages
//////////////////////////////////////////////////////////////////////////////

/**
 * Return package name, eg kuroo in app-portage/kuroo.
 * @param id
 */
const QString KurooDB::package( const QString& id )
{
	QString name = singleQuery( "SELECT name FROM package WHERE id = '" + id + "' LIMIT 1;" );

	if ( !name.isEmpty() )
		return name;
	else
		qWarning() << QString ( "Can not find package in database for id %1." ).arg ( id );

	return QString::null;
}

/**
 * Return category name, eg app-portage in app-portage/kuroo.
 * @param id
 */
const QString KurooDB::category( const QString& id )
{
	QString category = singleQuery( "SELECT category FROM package WHERE id = '" + id + "' LIMIT 1;" );

	if ( !category.isEmpty() )
		return category;
	else
		qWarning() << QString ( "Can not find category in database for id %1." ).arg ( id );

	return QString::null;
}

/**
 * Return package when searching by category-subcategory and name.
 * @param category		category-subcategory
 * @param name
 */
const QString KurooDB::packageId( const QString& package )
{
	QStringList parts = parsePackage( package );
	if ( !parts.isEmpty() )
	{
		QString category = parts[0];
		QString name = parts[1];
		QString id = singleQuery( "SELECT id FROM package WHERE name = '" + name + "' AND category = '" + category + "' LIMIT 1;" );

		if ( !id.isEmpty() )
			return id;
		else
			qWarning() << QString( "Can not find id in database for package %1/%2." ).arg( category ).arg( name );

	}
	else
		qWarning() << "Querying for package id. Can not parse: " << package;

	return QString::null;
}

/**
 * Return all versions for this package.
 * @param id
 */
const QStringList KurooDB::packageVersionsInstalled( const QString& idPackage )
{
	return query( " SELECT name FROM version WHERE idPackage = '" + idPackage + "'"
					" AND status = '" + PACKAGE_INSTALLED_STRING + "'"
					" ORDER BY version.name;" );
}

/**
 * Return all versions and their info for this package.
 * @param id
 */
const QStringList KurooDB::packageVersionsInfo( const QString& idPackage )
{
	return query( " SELECT name, description, homepage, status, licenses, useFlags, slot, keywords, size "
					" FROM version WHERE idPackage = '" + idPackage + "'"
					" ORDER BY version.name;" );
}

/**
 * Return the size of this package version.
 * @param idPackage
 * @param version
 */
const QString KurooDB::versionSize( const QString& idPackage, const QString& version )
{
	return singleQuery( " SELECT size, status FROM version WHERE idPackage = '" + idPackage + "'"
						 " AND name = '" + version + "' LIMIT 1;" );
}

/**
 * Return hardmask dependAtom and the gentoo dev comment.
 * @param id
 */
const QStringList KurooDB::packageHardMaskInfo( const QString& id )
{
	return query( "SELECT dependAtom, comment FROM packageHardMask WHERE idPackage = '" + id + "' LIMIT 1;" );
}

/**
 * Return path, eg where to find the package: in Portage or in any overlay.
 * @param idPackage
 * @param version
 */
const QString KurooDB::packagePath( const QString& id )
{
	return singleQuery( "SELECT path FROM package WHERE id = '" + id + "' LIMIT 1;" );
}

/**
 * Return package hardmask depend atom.
 * @param id
 */
const QStringList KurooDB::packageHardMaskAtom( const QString& id )
{
	return query( "SELECT dependAtom FROM packageHardMask WHERE idPackage = '" + id + "';" );
}

/**
 * Return package user-mask depend atom.
 * @param id
 */
const QStringList KurooDB::packageUserMaskAtom( const QString& id )
{
	return query( "SELECT dependAtom FROM packageUserMask WHERE idPackage = '" + id + "';" );
}

/**
 * Return package unmask depend atom.
 * @param id
 */
const QStringList KurooDB::packageUnMaskAtom( const QString& id )
{
	return query( "SELECT dependAtom FROM packageUnmask WHERE idPackage = '" + id + "';" );
}

/**
 * Return package keyword atom.
 * @param id
 */
const QString KurooDB::packageKeywordsAtom( const QString& id )
{
	return singleQuery( "SELECT keywords FROM packageKeywords WHERE idPackage = '" + id + "' LIMIT 1;" );
}

const QString KurooDB::packageUse( const QString& id )
{
	return singleQuery( "SELECT use FROM packageUse where idPackage = '" + id + "' LIMIT 1;" );
}

/**
 * Is the package in package.keywords?
 * @param id
 */
bool KurooDB::isPackageUnTesting( const QString& id )
{
	QString keywords = singleQuery( "SELECT keywords FROM packageKeywords where idPackage = '" + id + "' LIMIT 1;" );
	if ( keywords.contains( QRegExp( "(~\\*)|(~" + KurooConfig::arch() + ")" ) ) )
		return true;
	else
		return false;
}

/**
 * Is the package available in package.keywords?
 * @param id
 */
bool KurooDB::isPackageAvailable( const QString& id )
{
	QString keywords = singleQuery( "SELECT keywords FROM packageKeywords where idPackage = '" + id + "' LIMIT 1;" );
	if ( keywords.contains( QRegExp( "(\\-\\*)|(\\-" + KurooConfig::arch() + ")" ) ) )
		return true;
	else
		return false;
}

/**
 * Is the package in package.unmask? @fixme: better way to check for existens.
 * @param id
 */
bool KurooDB::isPackageUnMasked( const QString& id )
{
	return !query( "SELECT dependAtom FROM packageUnmask where idPackage = '" + id + "';" ).isEmpty();
}

/**
 * Add use flags for this package.
 * @param id
 */
void KurooDB::setPackageUse( const QString& id, const QString& useFlags )
{
	singleQuery( "REPLACE INTO packageUse (idPackage, use) VALUES ('" + id + "', '" + useFlags + "');" );
}

void KurooDB::setPackageUnMasked( const QString& id )
{
	singleQuery( "REPLACE INTO packageUnmask (idPackage, dependAtom) VALUES ('" + id + "', "
				  "'" + category( id ) + "/" + package( id ) + "');" );
}

/**
 * Add package in package.unmask. @fixme: check category and package?
 * @param id
 */
void KurooDB::setPackageUnMasked( const QString& id, const QString& version )
{
	singleQuery( "REPLACE INTO packageUnmask (idPackage, dependAtom) VALUES ('" + id + "', "
				  "'=" + category( id ) + "/" + package( id ) + "-" + version + "');" );
}

/**
 * Add package in package.mask. @fixme: check category and package?
 * @param id
 */
void KurooDB::setPackageUserMasked( const QString& id )
{
	singleQuery( "REPLACE INTO packageUserMask (idPackage, dependAtom) VALUES ('" + id + "', "
				  "'" + category( id ) + "/" + package( id ) + "');" );
}

/**
 * Set package as testing, eg add keyword ~*.
 * @param id
 */
void KurooDB::setPackageUnTesting( const QString& id )
{
	QString keywords = packageKeywordsAtom( id );

	// Aready testing skip!
	if ( keywords.contains( QRegExp( "(~\\*)|(~" + KurooConfig::arch() + ")" ) ) )
		return;

	if ( keywords.isEmpty() )
		insert( "INSERT INTO packageKeywords (idPackage, keywords) VALUES ('" + id + "', '~*');" );
	else
		singleQuery( "UPDATE packageKeywords SET keywords = '" + keywords + " ~*' WHERE idPackage = '" + id + "';" );
}

/**
 * Set package as available, eg add keywords '-* -arch'
 */
void KurooDB::setPackageAvailable( const QString& id )
{
	QString keywords = packageKeywordsAtom( id );

	// Already available skip!
	if ( keywords.contains( QRegExp( "(\\-\\*)|(\\-" + KurooConfig::arch() + ")" ) ) )
		return;

	if ( keywords.isEmpty() )
		insert( "INSERT INTO packageKeywords (idPackage, keywords) VALUES ('" + id + "', '-* -" + KurooConfig::arch() + "');" );
	else
		singleQuery( "UPDATE packageKeywords SET keywords = '" + keywords + " -* -" + KurooConfig::arch() + "' WHERE idPackage = '" + id + "';" );
}

/**
 * Clear testing keyword from package.
 * @param id
 */
void KurooDB::clearPackageUnTesting( const QString& id )
{
	QString keywords = packageKeywordsAtom( id );

	// If only testing keywords - remove it, else set only available keywords
	if ( !keywords.contains( QRegExp( "(\\-\\*)|(\\-" + KurooConfig::arch() + ")" ) ) )
		singleQuery( "DELETE FROM packageKeywords WHERE idPackage = '" + id + "';" );
	else
		singleQuery( "UPDATE packageKeywords SET keywords = '-* -" + KurooConfig::arch() + "' WHERE idPackage = '" + id + "';" );
}

/**
 * Removing available keywords for package.
 * @param id
 */
void KurooDB::clearPackageAvailable( const QString& id )
{
	QString keywords = packageKeywordsAtom( id );

	// If only available keywords - remove it, else set only testing keyword
	if ( !keywords.contains( QRegExp( "(~\\*)|(~" + KurooConfig::arch() + ")" ) ) )
		singleQuery( "DELETE FROM packageKeywords WHERE idPackage = '" + id + "';" );
	else
		singleQuery( "UPDATE packageKeywords SET keywords = '~*' WHERE idPackage = '" + id + "';" );;
}

/**
 * Clear package from package.unmask.
 * @param id
 */
void KurooDB::clearPackageUnMasked( const QString& id )
{
	singleQuery( "DELETE FROM packageUnmask WHERE idPackage = '" + id + "';" );
}

/**
 * Clear package from package.mask.
 * @param id
 */
void KurooDB::clearPackageUserMasked( const QString& id )
{
	singleQuery( "DELETE FROM packageUserMask WHERE idPackage = '" + id + "';" );
}


//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////

/**
 * Return all packages in the queue.
 */
const QStringList KurooDB::allQueuePackages()
{
	return query( " SELECT package.id, package.category, package.name, "
					" package.status, queue.idDepend, queue.size, queue.version "
					" FROM queue, package "
					" WHERE queue.idPackage = package.id "
					" ORDER BY queue.idDepend;" );
}

/**
 * Return all packages in the queue.
 */
const QStringList KurooDB::allQueueId()
{
	return query( "SELECT idPackage FROM queue;" );
}

/**
 * Return all history.
 */
const QStringList KurooDB::allHistory()
{
	return query( "SELECT timestamp, package, time, einfo FROM history ORDER BY id DESC;" );
}

/**
 * Return all etc-update history.
 */
const QStringList KurooDB::allMergeHistory()
{
	return query( "SELECT timestamp, source, destination FROM mergeHistory ORDER BY id DESC;" );
}

/**
 * Return all package statistics.
 */
const QStringList KurooDB::allStatistic()
{
	return query( "SELECT package, time, count FROM statistic ORDER BY id ASC;" );
}

/**
 * Clear all updates.
 */
void KurooDB::resetUpdates()
{
	singleQuery( "UPDATE package SET updateVersion = '' WHERE updateVersion != '';" );
}

/**
 * Clear all installed.
 */
void KurooDB::resetInstalled()
{
	singleQuery( "UPDATE package set installed = '" + PACKAGE_AVAILABLE_STRING + "';" );
}

void KurooDB::resetQueue()
{
	singleQuery( "DELETE FROM queue;" );
}

void KurooDB::addEmergeInfo( const QString& einfo )
{
	singleQuery( QString ( "UPDATE history SET einfo = '%1' WHERE id = (SELECT MAX(id) FROM history);" ).arg ( escapeString ( einfo ) ) );
}

/**
 * Insert etc-update file names.
 * @param source
 * @param destination
 */
void KurooDB::addBackup( const QString& source, const QString& destination )
{
	QDateTime currentTime( QDateTime::currentDateTime() );
	insert( QString ( "INSERT INTO mergeHistory ( timestamp, source, destination ) VALUES ('%1', '%2', '%3');" )
			 .arg( QString::number( currentTime.toTime_t() ) ).arg( source ).arg( destination ) );
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////

/**
 */
DbConnection::DbConnection( DbConfig* config )
		: m_config ( config )
{}

DbConnection::~DbConnection()
{}

/**
 * Sqlite methods
 */
SqliteConnection::SqliteConnection( SqliteConfig* config )
		: DbConnection( config )
{
	const QString path = QString( kurooDir + KurooConfig::databas() ).toLocal8Bit();

	// Open database file and check for correctness
	m_initialized = false;
	QFile file( path );
	if ( file.open( QIODevice::ReadOnly ) ) {
		QString format( file.readLine( 50 ) );

		if ( !format.startsWith( "SQLite format 3" ) )
			qWarning() << "Database versions incompatible. Removing and rebuilding database.";
		else
		{
			if ( sqlite3_open( path.toAscii(), &m_db ) != SQLITE_OK )
			{
				qWarning() << "Database file corrupt. Removing and rebuilding database.";
				sqlite3_close( m_db );
			}
			else
				m_initialized = true;
		}
	}

	if ( !m_initialized )
	{
		// Remove old db file; create new
		QFile::remove( path );
		if ( sqlite3_open( path.toAscii(), &m_db ) == SQLITE_OK )
			m_initialized = true;
	}
	else
	{
		if ( sqlite3_create_function( m_db, "rand", 0, SQLITE_UTF8, NULL, sqlite_rand, NULL, NULL ) != SQLITE_OK )
			m_initialized = false;
		if ( sqlite3_create_function( m_db, "power", 2, SQLITE_UTF8, NULL, sqlite_power, NULL, NULL ) != SQLITE_OK )
			m_initialized = false;
	}

	//optimization for speeding up SQLite
	query( "PRAGMA default_synchronous = OFF;" );
}

SqliteConnection::~SqliteConnection()
{
	if ( m_db )
		sqlite3_close( m_db );
}

QStringList SqliteConnection::query( const QString& statement )
{
	QStringList values;
	int error;
	const char* tail;
	sqlite3_stmt* stmt;

	//compile SQL program to virtual machine
	error = sqlite3_prepare( m_db, statement.toUtf8(), statement.length(), &stmt, &tail );

	if ( error != SQLITE_OK )
	{
		qWarning() << " sqlite3_compile error: " << sqlite3_errmsg( m_db ) << " on query: " << statement;
		values = QStringList();
	}
	else
	{
		int busyCnt( 0 );
		int number = sqlite3_column_count( stmt );

		//execute virtual machine by iterating over rows
		while ( true )
		{
			error = sqlite3_step( stmt );

			if ( error == SQLITE_BUSY )
			{
				if ( busyCnt++ > 99 )
				{
					qWarning() << "Busy-counter has reached maximum. Aborting this sql statement!";
					break;
				}
				::usleep( 1000000 ); // Sleep 1000 msec
				qDebug() << "sqlite3_step: BUSY counter: " << busyCnt << " on query: " << statement;
				continue;
			}

			if ( error == SQLITE_MISUSE ) {
				qWarning() << "sqlite3_step: MISUSE on query: " << statement;
				break;
			}

			if ( error == SQLITE_DONE || error == SQLITE_ERROR )
				break;

			//iterate over columns
			for ( int i = 0; i < number; i++ )
				values << QString::fromUtf8( ( const char* ) sqlite3_column_text( stmt, i ) );
		}

		//deallocate vm ressources
		sqlite3_finalize( stmt );

		if ( error != SQLITE_DONE )
		{
			qWarning() << "sqlite_step error: " << sqlite3_errmsg( m_db ) << " on query: " << statement;
			values = QStringList();
		}
	}

	return values;
}

QString SqliteConnection::singleQuery( const QString& statement )
{
	QString value;
	int error;
	const char* tail;
	sqlite3_stmt* stmt;

	//compile SQL program to virtual machine
	error = sqlite3_prepare( m_db, statement.toUtf8(), statement.length(), &stmt, &tail );

	if ( error != SQLITE_OK )
		qWarning() << "sqlite3_compile error: " << sqlite3_errmsg( m_db ) << " on query: " << statement;
	else
	{
		int busyCnt( 0 );

		//execute virtual machine
		while ( true )
		{
			error = sqlite3_step( stmt );

			if ( error == SQLITE_BUSY )
			{
				if ( busyCnt++ > 99 )
				{
					qWarning() << "Busy-counter has reached maximum. Aborting this sql statement!";
					break;
				}
				::usleep( 1000000 ); // Sleep 1000 msec
				qDebug() << "sqlite3_step: BUSY counter: " << busyCnt << " on query: " << statement;
				continue;
			}

			if ( error == SQLITE_MISUSE ) {
				qWarning() << "sqlite3_step: MISUSE on query: " << statement;
				break;
			}

			if ( error == SQLITE_DONE || error == SQLITE_ERROR )
				break;

			value = QString::fromUtf8( ( const char* ) sqlite3_column_text ( stmt, 0 ) );
		}

		//deallocate vm ressources
		sqlite3_finalize( stmt );

		if ( error != SQLITE_DONE )
		{
			qWarning() << "sqlite_step error: " << sqlite3_errmsg( m_db ) << " on query: " << statement;
			value = QString::null;
		}
	}

	return value;
}

int SqliteConnection::insert( const QString& statement )
{
	int error;
	const char* tail;
	sqlite3_stmt* stmt;

	//compile SQL program to virtual machine
	error = sqlite3_prepare( m_db, statement.toUtf8(), statement.toUtf8().length(), &stmt, &tail );

	if ( error != SQLITE_OK )
		qWarning() << "sqlite3_compile error: " << sqlite3_errmsg( m_db ) << " on insert: " << statement;
	else
	{
		int busyCnt( 0 );

		//execute virtual machine by iterating over rows
		while ( true )
		{
			error = sqlite3_step( stmt );

			if ( error == SQLITE_BUSY )
			{
				if ( busyCnt++ > 99 )
				{
					qWarning() << "Busy-counter has reached maximum. Aborting this sql statement!";
					break;
				}
				::usleep( 1000000 ); // Sleep 1000 msec
				qDebug() << "sqlite3_step: BUSY counter: " << busyCnt << " on insert: " << statement;
				continue;
			}

			if ( error == SQLITE_MISUSE ) {
				qWarning() << "sqlite3_step: MISUSE on insert: " << statement;
				break;
			}

			if ( error == SQLITE_DONE || error == SQLITE_ERROR )
				break;
		}

		//deallocate vm ressources
		sqlite3_finalize( stmt );

		if ( error != SQLITE_DONE )
		{
			qWarning() << "sqlite_step error: " << sqlite3_errmsg( m_db ) << " on insert: " << statement;
			return 0;
		}
	}
	return sqlite3_last_insert_rowid( m_db );
}

// this implements a RAND() function compatible with the MySQL RAND() (0-param-form without seed)
void SqliteConnection::sqlite_rand( sqlite3_context *context, int /*argc*/, sqlite3_value ** /*argv*/ )
{
	sqlite3_result_double( context, static_cast<double>( KRandom::random() ) / ( RAND_MAX + 1.0 ) );
}

// this implements a POWER() function compatible with the MySQL POWER()
void SqliteConnection::sqlite_power( sqlite3_context *context, int argc, sqlite3_value **argv )
{
	Q_ASSERT ( argc==2 );
	if ( sqlite3_value_type( argv[0] ) == SQLITE_NULL || sqlite3_value_type( argv[1] ) == SQLITE_NULL )
	{
		sqlite3_result_null( context );
		return;
	}
	double a = sqlite3_value_double( argv[0] );
	double b = sqlite3_value_double( argv[1] );
	sqlite3_result_double( context, pow( a,b ) );
}

SqliteConfig::SqliteConfig( const QString& dbfile )
		: m_dbfile( dbfile )
{}

/**
 * Connections pool with thread support
 */
DbConnectionPool::DbConnectionPool() : m_semaphore( POOL_SIZE )
{
	m_semaphore.acquire( POOL_SIZE ); //+=
	DbConnection *dbConn;
	m_dbConfig = new SqliteConfig( KurooConfig::databas() );
	dbConn = new SqliteConnection( static_cast<SqliteConfig*>( m_dbConfig ) );

	enqueue( dbConn );
	m_semaphore.release(1); //--
}

DbConnectionPool::~DbConnectionPool()
{
	m_semaphore.acquire( POOL_SIZE );
	bool vacuum = true;

	while( !isEmpty() )
	{
		DbConnection *conn = dequeue();
		if ( vacuum )
		{
			vacuum = false;
			qDebug() << "Running VACUUM";
			conn->query( "VACUUM;" );
		}
		delete conn;
	}
	delete m_dbConfig;
}

void DbConnectionPool::createDbConnections()
{
	for ( int i = 0; i < POOL_SIZE - 1; i++ )
	{
		DbConnection *dbConn;
		dbConn = new SqliteConnection( static_cast<SqliteConfig*>( m_dbConfig ) );
		enqueue( dbConn );
		m_semaphore.release(1);
	}
	qDebug() << "Create. Available db connections: " << m_semaphore.available();
}

DbConnection *DbConnectionPool::getDbConnection()
{
//	qDebug() << "Get a DB connection from" << m_semaphore.available() << "available.";
	m_semaphore.acquire(1);
	return dequeue();
}

void DbConnectionPool::putDbConnection( DbConnection *conn )
{
//	qDebug() << "Put a DB connection in" << m_semaphore.available() << "available.";
	enqueue( conn );
	m_semaphore.release(1);
}

QString DbConnectionPool::escapeString( const QString& str ) const
{
	QString result=str;
	return result.replace( '\'', "''" );
}

