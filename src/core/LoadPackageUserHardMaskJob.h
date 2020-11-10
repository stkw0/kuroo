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

#include <QTextStream>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/JobPointer>
#include <ThreadWeaver/Thread>

#include "common.h"
#include "portagefiles.h"

/**
* @class: LoadPackageUserMaskJob
* @short: Thread for loading user masked packages into db.
*/
class LoadPackageUserMaskJob : public ThreadWeaver::Job
{
public:
	LoadPackageUserMaskJob() : Job() {}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {

		// Collect all mask dependatoms from /etc/portage/package.mask
		QFileInfo fileInfo( KurooConfig::defaultFilePackageUserMask() );
		if( fileInfo.isDir() ) {
			qDebug() << KurooConfig::defaultFilePackageUserMask() << " is a dir";
			if( !mergeDirIntoFile( KurooConfig::defaultFilePackageUserMask() ) ) {
				return;
			}
		}

		QFile file( KurooConfig::defaultFilePackageUserMask() );
		QTextStream stream( &file );
		QStringList linesDependAtom;
		if ( !file.open( QIODevice::ReadOnly ) )
			qCritical() << "Parsing user package.mask. Reading: " << KurooConfig::defaultFilePackageUserMask();
		else {
			while ( !stream.atEnd() )
				linesDependAtom += stream.readLine();
			file.close();
		}

		// Something is wrong, no files found, get outta here
		if ( linesDependAtom.isEmpty() )
			return;

		//setStatus( "PackageUserMask", i18n("Collecting user masked packages...") );

		DbConnection* const m_db = KurooDBSingleton::Instance()->getStaticDbConnection();
		KurooDBSingleton::Instance()->singleQuery(	"CREATE TEMP TABLE packageUserMask_temp ( "
													"idPackage INTEGER UNIQUE, "
													"dependAtom VARCHAR(255), "
													"comment BLOB );"
													, m_db);

		KurooDBSingleton::Instance()->singleQuery( "BEGIN TRANSACTION;", m_db );

		QStringList commentLines;
		for ( QStringList::Iterator it = linesDependAtom.begin(), end = linesDependAtom.end(); it != end; ++it ) {

			// Collect comment lines above the dependatom
			if ( (*it).isEmpty() )
				commentLines.clear();
			else {
				if ( (*it).startsWith( "#" ) ) {
					commentLines += (*it).replace('\'', "''").replace('%', "&#37;").toUtf8();
				}
				else {
					PortageAtom atom( *it );
					if ( atom.isValid() ) {
						QString id = KurooDBSingleton::Instance()->singleQuery(
							"SELECT id FROM package WHERE name = '" + atom.package() + "' AND category = '" + atom.category() + "' LIMIT 1;", m_db );

						if ( id.isEmpty() )
							qWarning() << QString("Parsing user package.mask. Can not find id in database for package %1/%2.")
								.arg( atom.category() ).arg( atom.package() );
						else
							KurooDBSingleton::Instance()->insert( QString(
								"INSERT INTO packageUserMask_temp (idPackage, dependAtom, comment) "
								"VALUES ('%1', '%2', '%3');" ).arg( id ).arg( *it ).arg( commentLines.join( "\n" ) ), m_db );

					}
					else
						qWarning() << QString("Parsing user package.mask. Can not match package %1 in %2.").arg( *it )
							.arg( KurooConfig::defaultFilePackageUserMask() );
				}
			}
		}
		file.close();
		KurooDBSingleton::Instance()->singleQuery( "COMMIT TRANSACTION;", m_db );

		// Move content from temporary table to installedPackages
		KurooDBSingleton::Instance()->singleQuery( "DELETE FROM packageUserMask;", m_db );
		KurooDBSingleton::Instance()->insert( "INSERT INTO packageUserMask SELECT * FROM packageUserMask_temp;", m_db );
		KurooDBSingleton::Instance()->singleQuery( "DROP TABLE packageUserMask_temp;", m_db );

		KurooDBSingleton::Instance()->returnStaticDbConnection( m_db );
		//setStatus( "PackageUserMask", i18n("Done.") );
//         return;
// 	}
//
// 	virtual void completeJob() {
		PortageFilesSingleton::Instance()->refresh( PACKAGE_USER_MASK_SCANNED );
	}
};
