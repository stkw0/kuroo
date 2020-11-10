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

#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/JobPointer>
#include <ThreadWeaver/Queue>
#include <ThreadWeaver/Thread>
#include <QTextStream>

#include "common.h"
#include "portagefiles.h"
#include "LoadPackageHardMaskJob.h"
#include "LoadPackageKeywordsJob.h"
#include "LoadPackageUseJob.h"
#include "LoadPackageUserHardMaskJob.h"
#include "LoadPackageUserUnMaskJob.h"

bool mergeDirIntoFile( QString dirPath ) {
	//DEBUG_LINE_INFO;
	QDir mergeDir( dirPath );
	//TODO make sure this doesn't exist before we enter
	QFile tempFile( dirPath + ".temp" );
	QTextStream tempStream( &tempFile );
	if( !tempFile.open( QIODevice::WriteOnly ) ) {
		//qDebug() << "Opened " << tempFile.fileName() << " for writing.";
		//TODO handle failure
		return false;
	}

	QList<QFileInfo> infos = mergeDir.entryInfoList();
	QStringList lines;
	foreach( QFileInfo fi, infos ) {
		//qDebug() << "Processing " << fi.filePath();
		if( fi.isDir() ) {
			//qDebug() << fi.filePath() << " is a dir.";
			if( fi.filePath().endsWith( "/." ) || fi.filePath().endsWith( "/.." ) ) {
				//qDebug() << fi.filePath() << " is ., skipping.";
			} else {
				//qDebug() << "Would recurse into " << fi.filePath();
				//TODO handle failure
				if( !mergeDirIntoFile( fi.filePath() ) ) {
					return false;
				}
			}
		}

		QFile entryFile( fi.absoluteFilePath() );
		QTextStream streamFile( &entryFile );
		if ( !entryFile.open( QIODevice::ReadOnly ) ) {
			//qCritical() << "Parsing " << fi.filePath();
		} else {
			while ( !streamFile.atEnd() )
				lines += streamFile.readLine();
			entryFile.close();
		}

		//Save the file as we go
		foreach( QString line, lines ) {
			tempStream << line << "\n";
		}

		if( !entryFile.remove() ) {
			//TODO handle failure
			return false;
		}
	}
	tempFile.close();
	//By the time we get out of here the directory should be empty, or else. . .
	if( mergeDir.rmdir( dirPath ) ) {
		//TODO handle failure
		return false;
	}

	//And write the new file in it's place
	KIO::file_move( QUrl::fromLocalFile(dirPath + ".temp"), QUrl::fromLocalFile(dirPath), -1, KIO::Overwrite | KIO::HideProgressInfo );
	return true;
}

/**
* @class: SavePackageKeywordsJob
* @short: Thread for loading packages unmasked by user.
*/
class SavePackageKeywordsJob : public ThreadWeaver::Job
{
public:
	SavePackageKeywordsJob() : Job() {}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {

		const QStringList lines = KurooDBSingleton::Instance()->query(
			"SELECT package.category, package.name, packageKeywords.keywords FROM package, packageKeywords "
			"WHERE package.id = packageKeywords.idPackage;" );
		qDebug() << "Found" << lines;
		if ( lines.isEmpty() ) {
			qWarning() << QString("No package keywords found. Saving to %1 aborted!")
				.arg( KurooConfig::defaultFilePackageKeywords() );
			return;
		}

		QFile file( KurooConfig::defaultFilePackageKeywords() );
		QTextStream stream( &file );

		if ( !file.open( QIODevice::WriteOnly ) ) {
			qCritical() << QString("Writing: %1.").arg( KurooConfig::defaultFilePackageKeywords() );
			return;
		}

		QStringListIterator it( lines );
		while( it.hasNext() ) {
			QString category = it.next();
			QString package = it.next();
			QString keywords = it.next();
			if ( !package.isEmpty() )
				stream << category << "/" << package << " " << keywords << "\n";
		}

		file.close();
//		return;
// 	}
//
// 	virtual void completeJob() {
		PortageFilesSingleton::Instance()->refresh( PACKAGE_KEYWORDS_SAVED );
	}
};

/**
* @class: SavePackageUserMaskJob
* @short: Thread for saving packages unmasked by user.
*/
class SavePackageUserMaskJob : public ThreadWeaver::Job
{
public:
	SavePackageUserMaskJob() : Job() {}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {

		const QStringList lines = KurooDBSingleton::Instance()->query( "SELECT dependAtom FROM packageUserMask;" );
		if ( lines.isEmpty() ) {
			qWarning() << QString("No user mask depend atom found. Saving to %1 aborted!")
				.arg( KurooConfig::defaultFilePackageUserMask() );
			return;
		}

		QFile file( KurooConfig::defaultFilePackageUserMask() );
		QTextStream stream( &file );
		if ( !file.open( QIODevice::WriteOnly ) ) {
			qCritical() << QString("Writing: %1.").arg( KurooConfig::defaultFilePackageUserMask() );
			return;
		}

		foreach( QString line, lines )
			stream << line << "\n";

		file.close();
// 		return;
// 	}
//
// 	virtual void completeJob() {
		PortageFilesSingleton::Instance()->refresh( PACKAGE_USER_MASK_SAVED );
	}
};

/**
* @class: SavePackageUserMaskJob
* @short: Thread for saving packages unmasked by user.
*/
class SavePackageUserUnMaskJob : public ThreadWeaver::Job
{
public:
	SavePackageUserUnMaskJob() : Job() {}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {

		const QStringList lines = KurooDBSingleton::Instance()->query( "SELECT dependAtom FROM packageUnMask ;" );
		if ( lines.isEmpty() ) {
			qWarning() << QString("No user unmask depend atom found. Saving to %1 aborted!")
				.arg( KurooConfig::defaultFilePackageUserUnMask() );
			return;
		}

		QFile file( KurooConfig::defaultFilePackageUserUnMask() );
		QTextStream stream( &file );
		if ( !file.open( QIODevice::WriteOnly ) ) {
			qCritical() << QString("Writing: %1.").arg( KurooConfig::defaultFilePackageUserUnMask() );
			return;
		}

		foreach ( QString line, lines )
			stream << line << "\n";

		file.close();
// 		return;
// 	}
//
// 	virtual void completeJob() {
		PortageFilesSingleton::Instance()->refresh( PACKAGE_USER_UNMASK_SAVED );
	}
};

/**
* @class: SavePackageUseJob
* @short: Thread for saving packages use-setting by user.
*/
class SavePackageUseJob : public ThreadWeaver::Job
{
public:
	SavePackageUseJob() : Job() {}

	virtual void run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* ) {

		const QStringList lines = KurooDBSingleton::Instance()->query(
			"SELECT package.category, package.name, packageUse.use FROM package, packageUse "
			"WHERE package.id = packageUse.idPackage;" );
		if ( lines.isEmpty() ) {
			qWarning() << QString("No package use found. Saving to %1 aborted!").arg( KurooConfig::defaultFilePackageUserUse() );
			return;
		}

		QFile file( KurooConfig::defaultFilePackageUserUse() );
		QTextStream stream( &file );
		if ( !file.open( QIODevice::WriteOnly ) ) {
			qCritical() << QString("Writing: %1.").arg( KurooConfig::defaultFilePackageUserUse() );
			return;
		}

		QStringListIterator it( lines );
		while( it.hasNext() ) {
			QString category = it.next();
			QString package = it.next();
			QString use = it.next();
			QString tmpuse = use;
			if ( !tmpuse.remove(" ").isEmpty() )
				stream << category << "/" << package << " " << use << "\n";
		}

		file.close();
//		return;
// 	}
//
// 	virtual void completeJob() {
		PortageFilesSingleton::Instance()->refresh( PACKAGE_USER_USE_SAVED );
	}
};

/**
* Object for resulting list of packages from emerge actions.
*/
PortageFiles::PortageFiles( QObject *m_parent )
	: QObject( m_parent )
{}

PortageFiles::~PortageFiles()
{}

void PortageFiles::init( QObject *parent )
{
	m_parent = parent;
}

/**
* Forward signal to refresh results.
*/
void PortageFiles::refresh( int mask )
{
	if (LogSingleton::Instance()) {
		switch ( mask ) {
			case PACKAGE_KEYWORDS_SCANNED:
				LogSingleton::Instance()->writeLog( i18n( "Completed scanning for package keywords in %1.",
														KurooConfig::defaultFilePackageKeywords() ), KUROO );
				break;
			case PACKAGE_USER_UNMASK_SCANNED:
				LogSingleton::Instance()->writeLog(  i18n( "Completed scanning for unmasked packages in %1.",
														KurooConfig::defaultFilePackageUserUnMask() ), KUROO );
				break;
			case PACKAGE_HARDMASK_SCANNED:
				LogSingleton::Instance()->writeLog(  i18n( "Completed scanning for hardmasked packages in %1.",
														KurooConfig::filePackageHardMask() ), KUROO );
				break;
			case PACKAGE_USER_MASK_SCANNED:
				LogSingleton::Instance()->writeLog(  i18n( "Completed scanning for user masked packages in %1.",
														KurooConfig::defaultFilePackageUserMask() ), KUROO );
				break;
			case PACKAGE_KEYWORDS_SAVED:
				LogSingleton::Instance()->writeLog(  i18n( "Completed saving package keywords in %1.",
														KurooConfig::defaultFilePackageKeywords() ), KUROO );
				emit signalPortageFilesChanged();
				break;
			case PACKAGE_USER_MASK_SAVED:
				LogSingleton::Instance()->writeLog(  i18n( "Completed saving user masked packages in %1.",
														KurooConfig::defaultFilePackageUserMask() ), KUROO );
				emit signalPortageFilesChanged();
				break;
			case PACKAGE_USER_UNMASK_SAVED:
				LogSingleton::Instance()->writeLog(  i18n( "Completed saving user unmasked packages in %1.",
														KurooConfig::defaultFilePackageUserUnMask() ), KUROO );
				break;
			case PACKAGE_USER_USE_SCANNED:
				LogSingleton::Instance()->writeLog(  i18n( "Completed scanning user package use flags in %1.",
														KurooConfig::defaultFilePackageUserUse() ), KUROO );
				break;
			case PACKAGE_USER_USE_SAVED:
				LogSingleton::Instance()->writeLog(  i18n( "Completed saving user package use in %1.",
														KurooConfig::defaultFilePackageUserUse() ), KUROO );
		}
	}
}


/**
* Load all!
*/
void PortageFiles::loadPackageFiles()
{
	DEBUG_LINE_INFO;

	SignalistSingleton::Instance()->setKurooBusy(true);

	if (NULL != KurooStatusBar::instance())
		KurooStatusBar::instance()->setProgressStatus( QString::null, i18n("Loading portage files"));
	ThreadWeaver::Queue::instance()->stream() << new LoadPackageHardMaskJob();
	loadPackageUserMask();
	loadPackageUnmask();
	loadPackageKeywords();
	loadPackageUse();
	SignalistSingleton::Instance()->setKurooBusy(false);
	if (NULL != KurooStatusBar::instance())
		KurooStatusBar::instance()->setProgressStatus( QString::null, i18n("Done."));
}

void PortageFiles::loadPackageUserMask()
{
	ThreadWeaver::Queue::instance()->stream() << new LoadPackageUserMaskJob();
}

void PortageFiles::loadPackageUnmask()
{
	ThreadWeaver::Queue::instance()->stream() << new LoadPackageUserUnMaskJob();
}

void PortageFiles::loadPackageKeywords()
{
	ThreadWeaver::Queue::instance()->stream() << new LoadPackageKeywordsJob();
}

void PortageFiles::loadPackageUse()
{
	ThreadWeaver::Queue::instance()->stream() << new LoadPackageUseJob();
}

void PortageFiles::savePackageKeywords()
{
	ThreadWeaver::Queue::instance()->stream() << new SavePackageKeywordsJob();
}

void PortageFiles::savePackageUserUnMask()
{
	ThreadWeaver::Queue::instance()->stream() << new SavePackageUserUnMaskJob();
}

void PortageFiles::savePackageUserMask()
{
	ThreadWeaver::Queue::instance()->stream() << new SavePackageUserMaskJob();
}

void PortageFiles::savePackageUse()
{
	ThreadWeaver::Queue::instance()->stream() << new SavePackageUseJob();
}

