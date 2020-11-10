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

#include "common.h"
#include "message.h"
#include "etcupdate.h"

#include <assert.h>
#include <errno.h>

#include <QObject>
#include <KMessageBox>
#include <kio/job.h>
#include <KF5/KCoreAddons/KProcess>
#include <KDirWatch>

/**
* @class EtcUpdate
* @short Handles etc-updates.
*
* Runs etc-update to collect the list of etc-files that needs merging.
* Launches the exernal diff-tool for selected files.
*/
//EtcUpdate::EtcUpdate( QObject* m_parent, const char* name )
//	: QObject( m_parent, name )
//{}

//EtcUpdate::~EtcUpdate()
//{
//	delete eProc;
//	eProc = 0;
//}

void EtcUpdate::init( QObject *parent )
{
	DEBUG_LINE_INFO;
	m_parent = parent;

	eProc = new KProcess();
	connect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &EtcUpdate::slotCleanupDiff);

	m_mergingFile = new KDirWatch( this );
	connect(m_mergingFile, &KDirWatch::dirty, this, &EtcUpdate::slotChanged);
}

/**
* Scan for new configuration files.
*/
void EtcUpdate::slotEtcUpdate()
{
	DEBUG_LINE_INFO;
	if ( KurooConfig::etcUpdateTool().isEmpty() )
		KMessageBox::information( 0, i18n( "Please specify merge tool in settings!" ), i18n( "Kuroo" ) );
	else {
		m_etcFilesList.clear();
		m_backupFilesList.clear();

		// First collect old merged files
		m_configProtectList = QStringList( kurooDir + QString("backup/configuration") );

		// Then scan for new unmerged files
		m_configProtectList += KurooConfig::configProtectList().split( " " );

		qDebug() << m_configProtectList;

		// Launch scan slave
		slotFinished();
	}
}

/**
* Scan each confProtect dirs.
*/
void EtcUpdate::slotFinished(/*KJob* j*/)
{
	if ( m_configProtectList.count() > 0 ) {
		m_configProtectDir = m_configProtectList.takeFirst();
		//The Slave was causing a 'kuroo(5827)/kio (KIOJob) KIO::SlaveInterface::dispatch: error  111   "/var/cache/kuroo/backup/configuration"' message
		//so don't do it if the dir doesn't exist
		if (QFile::exists( m_configProtectDir )) {
			KIO::ListJob* job = KIO::listRecursive( QUrl::fromLocalFile( m_configProtectDir ), KIO::HideProgressInfo, true);
			connect(job, &KIO::ListJob::entries, this, &EtcUpdate::slotListFiles);
			connect( job, SIGNAL( result( KJob* ) ), SLOT( slotFinished(KJob*) ) );
		}
		else
			slotFinished();
	}
	else
		emit signalScanCompleted();
}

/**
* Collect new configuration files.
*/
void EtcUpdate::slotListFiles( KIO::Job*, const KIO::UDSEntryList& entries )
{
	QString configFile;
	foreach( KIO::UDSEntry entry, entries ) {
		configFile = entry.stringValue( KIO::UDSEntry::UDS_NAME );

		if ( configFile.contains( QRegExp( "\\d{8}_\\d{4}/" ) ) && !configFile.endsWith( ".orig" ) ) {
			m_backupFilesList += m_configProtectDir + "/" + configFile;
		} else if ( !m_configProtectDir.startsWith( "/var/cache/kuroo" ) && configFile.contains( "._cfg" ) )
		{
			m_etcFilesList += m_configProtectDir + "/" + configFile;
		}
	}
}

/**
* Launch diff tool with first etc-file in list.
*/
void EtcUpdate::runDiff( const QString& source, const QString& destination/*, const bool& isNew */)
{
	struct stat st;

	if ( !source.isEmpty() ) {
		m_changed = false;
		m_source = source;
		m_destination = destination;
		QString backupPath = kurooDir + "backup/configuration/";

		// Check for etc-files warnings
		QString etcWarning;
		const QStringList etcFilesWarningsList = KurooConfig::etcFiles().split( " " );
		foreach( QString file, etcFilesWarningsList ) {
			if ( file == m_destination ) {
				etcWarning = i18n( "<font color=red>Warning!<br/>%1 has been edited by you.</font><br/>", m_destination );
			}
		}

		eProc->close();
		eProc->clearProgram();
		*eProc << "kompare" << m_source << m_destination;

		/*if ( isNew )
			*eProc << "-o" << m_destination;
		else
			*eProc << "-o" << backupPath + "merging";
		*/
		eProc->start();

		if ( eProc->state() == QProcess::NotRunning ) {
			LogSingleton::Instance()->writeLog( i18n( "%1 didn't start.", KurooConfig::etcUpdateTool() ), ERROR );
			KMessageBox::sorry( 0, i18n( "%1 could not start!", KurooConfig::etcUpdateTool() ), i18n( "Kuroo" ) );
		}
		else {
			LogSingleton::Instance()->writeLog( i18n( "Merging changes in \'%1\'.", m_destination ), KUROO );

			// get the original file mode
			memset(&st, 0, sizeof(st));
			m_mergedMode = -1;
			if( !(stat( m_destination.toAscii(), &st ) < 0) ) {
				m_mergedMode = (int)st.st_mode;
			}

			// Watch for changes
			m_mergingFile->addFile( m_destination );

			// Make temporary backup of original conf file
			KIO::file_copy( QUrl::fromLocalFile( m_destination ), QUrl::fromLocalFile( backupPath + "merging.orig" ) , m_mergedMode, KIO::Overwrite | KIO::HideProgressInfo );
		}
	}
}

void EtcUpdate::slotChanged()
{
	m_changed = true;
}

/**
* After diff tool completed, close all.
* @param proc
*/
void EtcUpdate::slotCleanupDiff()
{
	//Unregister the watcher
	m_mergingFile->removeFile( m_destination );

	if ( m_changed ) {

		QDateTime dt = QDateTime::currentDateTime();
		QString backupPath = kurooDir + "backup/configuration/";
		QString backupPathDir = backupPath + dt.toString( "yyyyMMdd_hhmm" ) + "/";
		QDir d( backupPathDir );
		if ( !d.exists() ) {
			QDir bc( backupPath );
			if( !bc.exists() )
				bc.mkdir( backupPath );
			d.mkdir( backupPathDir );
		}

		// Make backup of original file
		QString destination = m_destination;
		destination.replace( "/", ":" );

		//Change this to a move instead of copy so we don't leave the temp file around
		KIO::file_move( QUrl::fromLocalFile( backupPath + "merging.orig" ), QUrl::fromLocalFile( backupPathDir + destination + ".orig" ), m_mergedMode, KIO::Overwrite | KIO::HideProgressInfo );
		KIO::file_copy( QUrl::fromLocalFile( m_destination ), QUrl::fromLocalFile( backupPathDir + destination ), m_mergedMode, KIO::Overwrite );

		//This is only necessary because it seems that kdiff3 rewrites the mode.
		KIO::chmod( QUrl::fromLocalFile( m_destination ), m_mergedMode );

		KIO::file_delete( QUrl::fromLocalFile( m_source ) );

		LogSingleton::Instance()->writeLog( i18n( "Deleting \'%1\'. Backup saved in %2.", m_source, kurooDir + "backup" ), KUROO );

		KurooDBSingleton::Instance()->addBackup( m_source, m_destination );
		emit signalEtcFileMerged();
	}
}

