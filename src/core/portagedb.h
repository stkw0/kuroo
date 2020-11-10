/**************************************************************************
*   Copyright (C) 2004 by karye                                           *
*   karye@users.sourceforge.net                                           *
*   From Amarok code                                                      *
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

#ifndef PORTAGEDB_H
#define PORTAGEDB_H

#include <QDir>            //stack allocated
#include <QObject>         //baseclass
#include <QQueue>       //baseclass
#include <QSemaphore>	//stack allocated
#include <QStringList>     //stack allocated

#include <sqlite3.h>

class Package;
class DbConnection;
class DbConnectionPool;

class DbConfig
{};

class SqliteConfig : public DbConfig
{
public:
	SqliteConfig( const QString& /* dbfile */ );

	const 				QString dbFile() const { return m_dbfile; }

private:
	QString m_dbfile;
};

class DbConnection
{
public:
	DbConnection( DbConfig* /* config */ );
	virtual ~DbConnection() = 0;

	virtual QStringList query( const QString& /* statement */) = 0;
	virtual QString		singleQuery( const QString& /* statement */) = 0;
	virtual int 		insert( const QString& /* statement */) = 0;
	bool 	isInitialized() const { return m_initialized; }
	virtual bool 		isConnected() const = 0;
	virtual const 	QString lastError() const { return "None"; }

protected:
	bool 				m_initialized;
	DbConfig*			m_config;
};

class SqliteConnection : public DbConnection
{
public:
	SqliteConnection( SqliteConfig* /* config */ );
	~SqliteConnection();

	QStringList 		query( const QString& /* statement */ );
	QString		 	singleQuery( const QString& /* statement */ );
	int 			insert( const QString& /* statement */ );
	inline bool 		isConnected()const { return true; }

private:
	static void 		sqlite_rand( sqlite3_context *context, int /*argc*/, sqlite3_value ** /*argv*/ );
	static void 		sqlite_power( sqlite3_context *context, int argc, sqlite3_value **argv );

	sqlite3* 			m_db;
};

class DbConnectionPool : QQueue<DbConnection*>
{
public:
	DbConnectionPool();
	~DbConnectionPool();

	const 				DbConfig *getDbConfig() const { return m_dbConfig; }
	void 				createDbConnections();

	DbConnection*			getDbConnection();
    void 				putDbConnection( DbConnection* conn );

	QString 			escapeString( const QString&) const;

private:
	static const int 	POOL_SIZE = 10;
	QSemaphore 			m_semaphore;
	DbConfig*			m_dbConfig;
};

/**
 * @class KurooDB
 * @short All database related as connections, queries...
 */
class KurooDB : public QObject
{
Q_OBJECT

public:
	KurooDB( QObject *m_parent = 0 );
	~KurooDB();

	void 					destroy();

	/**
	 * Check db integrity and create new db if necessary.
	 */
	QString 				init( QObject *parent = 0 );
	inline QString 			escapeString( const QString& string) const { return m_dbConnPool->escapeString(string); }

	/**
	* This method returns a static DbConnection for components that want to use
	* the same connection for the whole time. Should not be used anywhere else
	* but in CollectionReader.
	*
	* @return static DbConnection
	*/
	DbConnection 			*getStaticDbConnection();

	/**
	* Returns the DbConnection back to connection pool.
	*
	* @param conn DbConnection to be returned
	*/
	void 					returnStaticDbConnection( DbConnection *conn );

	//sql helper methods
	QStringList 			query( const QString& statement, DbConnection *conn = NULL );
	QString		 			singleQuery( const QString& statement, DbConnection *conn = NULL );
	int 					insert( const QString& statement, DbConnection *conn = NULL );

	//table management methods
	bool					isCacheEmpty();
	bool 					isPortageEmpty();
	bool					isQueueEmpty();
	bool					isUpdatesEmpty();
	bool 					isHistoryEmpty();
	bool 					isValid();
	void 					createTables( DbConnection *conn = NULL );

	// Kuroo main
	QString					getKurooDbMeta( const QString& meta );
	void					setKurooDbMeta( const QString& meta, const QString& data );
	void					backupDb();
	void					restoreBackup();

	//////////////////////////////////////////////////////////////////////////////
	// Queries for Portage
	//////////////////////////////////////////////////////////////////////////////
	const QStringList		allCategories();
	const QStringList		allSubCategories();
	const QStringList		portageCategories( int filter, const QString& text );
	const QStringList		portageSubCategories( const QString& categoryId, int filter, const QString& text );
	const QStringList		portagePackagesBySubCategory( const QString& categoryId, const QString& subCategoryId, int filter, const QString& text );
	const QString		 	packageId( const QString& package );


	//////////////////////////////////////////////////////////////////////////////
	// Queries for allPackages
	//////////////////////////////////////////////////////////////////////////////
	const QString 			packagePath( const QString& id );
	const QStringList 		packageVersionsInstalled( const QString& idPackage );
	const QStringList 		packageVersionsInfo( const QString& idPackage );
	const QString			versionSize( const QString& idPackage, const QString& version );
	const QStringList		packageHardMaskInfo( const QString& id );
	const QString		 	package( const QString& id );
	const QString		 	category( const QString& id );


	///////////////////////////////////////////////////////////////////////////////
	// Queries for portage files
	///////////////////////////////////////////////////////////////////////////////
	const QStringList	 	packageHardMaskAtom( const QString& id );
	const QStringList		packageUserMaskAtom( const QString& id );
	const QStringList	 	packageUnMaskAtom( const QString& id );
	const QString			packageKeywordsAtom( const QString& id );
	const QString			packageUse( const QString& id );

	bool 					isPackageUnMasked( const QString& id );
	bool 					isPackageUnTesting( const QString& id );
	bool 					isPackageAvailable( const QString& id );

	void					setPackageUse( const QString& id, const QString& useFlags );
	void					setPackageUnTesting( const QString& id );
	void					setPackageUnMasked( const QString& id );
	void					setPackageUnMasked( const QString& id, const QString& version );
	void					setPackageUserMasked( const QString& id );
	void					setPackageAvailable( const QString& id );

	void					clearPackageUnTesting( const QString& id );
	void					clearPackageUnMasked( const QString& id );
	void					clearPackageUserMasked( const QString& id );
	void					clearPackageAvailable( const QString& id );


	//////////////////////////////////////////////////////////////////////////////
	// Miscellanious queries
	//////////////////////////////////////////////////////////////////////////////
	const QStringList 		allQueuePackages();
	const QStringList		allQueueId();
	const QStringList 		allHistory();
	const QStringList 		allMergeHistory();
	const QStringList 		allStatistic();

	void					resetUpdates();
	void					resetInstalled();
	void					resetQueue();
	void					addEmergeInfo( const QString& einfo );
	void					addBackup( const QString& source, const QString& destination );

private:
	QObject*				m_parent;

	DbConnectionPool		*m_dbConnPool;
	bool					m_monitor;
};

#endif /* KUROODB_H */
