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
#include "kurooinit.h"
#include "introdlg.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>

#include <QDebug>
#include <QDir>

#include <KUser>
#include <KMessageBox>
#include <KProcess>

/**
 * @class KurooInit
 * @short KurooInit checks that kuroo environment is correctly setup.
 *
 * And launch intro wizard whenever a new version of kuroo is installed.
 * Set ownership for directories and files to portage:portage.
 * Check that user is in portage group.
 */
KurooInit::KurooInit( QObject *parent )
	: QObject( parent ), wizardDialog( 0 )
{
	qDebug() << "Initializing Kuroo Environment";
	getEnvironment();

	checkEtcFiles();

	// Run intro if new version is installed or no DirHome directory is detected.
	QDir d( kurooDir );
	if ( KurooConfig::version() != KurooConfig::hardVersion() || !d.exists() || KurooConfig::wizard() ) {
		firstTimeWizard();
	} else {
		if ( !KUser().isSuperUser() ) {
			checkUser();
		}
	}

	// Get portage groupid to set directories and files owned by portage
	struct group* portageGid = getgrnam( QFile::encodeName("portage") );
	struct passwd* portageUid = getpwnam( QFile::encodeName("portage") );

	// Setup kuroo environment
	if ( KurooConfig::init() ) {
		KurooConfig::setSaveLog( false );

		// Create DirHome dir and set permissions so common user can run Kuroo
		if ( !d.exists() ) {
			if ( !d.mkdir( kurooDir ) ) {
				KMessageBox::error( 0, i18n("<qt>Could not create kuroo home directory.<br/>"
											"You must start Kuroo with kdesu first time for a secure initialization.<br/>"
											"Please try again!</qt>"), i18n("Initialization") );
				exit(0);
			} else {
				chmod( kurooDir.toAscii(), 0770 );
				KIO::chown( QUrl::fromLocalFile(kurooDir), portageGid->gr_name, portageUid->pw_name);
			}

			d.setCurrent( kurooDir );
		}
	}

	// Check that backup directory exists and set correct permissions
	QString backupDir = kurooDir + "backup";
	if ( !d.cd( backupDir ) ) {
		if ( !d.mkdir( backupDir ) ) {
			KMessageBox::error( 0, i18n("<qt>Could not create kuroo backup directory.<br/>"
										"You must start Kuroo with kdesu first time for a secure initialization.<br/>"
										"Please try again!</qt>"), i18n("Initialization") );
			exit(0);
		}
		else {
			chmod( backupDir.toAscii(), 0770 );
			KIO::chown( QUrl::fromLocalFile(backupDir), portageGid->gr_name, portageUid->pw_name);
		}
	}

	KurooConfig::setVersion( KurooConfig::hardVersion() );
	KurooConfig::self()->save();

	// Initialize the log
	QString logFile = LogSingleton::Instance()->init( this );
	if ( !logFile.isEmpty() ) {
		chmod( logFile.toAscii(), 0660 );
		KIO::chown( QUrl::fromLocalFile(logFile), portageGid->gr_name, portageUid->pw_name );
	}

	// Initialize the database
	QString databaseFile = KurooDBSingleton::Instance()->init( this );
	QString database = kurooDir + KurooConfig::databas();
	QString dbVersion = KurooDBSingleton::Instance()->getKurooDbMeta( "kurooVersion" );

	// Check for conflicting db design or new install
	if ( KurooConfig::version().section( "_db", 1, 1 ) != dbVersion ) {

		// Backup history if there's old db version
		if ( !dbVersion.isEmpty() ) {
			KurooDBSingleton::Instance()->backupDb();
			remove( database.toAscii() );
			qWarning() << QString("Database structure is changed. Deleting old version of database %1").arg( database );

			// and recreate with new structure
			KurooDBSingleton::Instance()->init( this );
		}

		KurooDBSingleton::Instance()->setKurooDbMeta( "kurooVersion", KurooConfig::version().section( "_db", 1, 1 ) );
	}

	// Give permissions to portage:portage to access the db also
	chmod( databaseFile.toAscii(), 0660 );
	KIO::chown( QUrl::fromLocalFile(databaseFile), portageGid->gr_name, portageUid->pw_name );

    // Initialize singletons objects
	SignalistSingleton::Instance()->init( this );
	EmergeSingleton::Instance()->init( this );
	EtcUpdateSingleton::Instance()->init( this );
	HistorySingleton::Instance()->init( this );
	PortageSingleton::Instance()->init( this );
	QueueSingleton::Instance()->init( this );
	PortageFilesSingleton::Instance()->init( this );
	FileWatcherSingleton::Instance()->init( this );

	//Load packages in case /etc/portage.* changed
	PortageFilesSingleton::Instance()->loadPackageFiles();
}

KurooInit::~KurooInit()
{
	KurooConfig::setInit( false );
}

/**
 * Run "emerge --info" to collect system info like "ACCEPT_KEYWORDS" and "CONFIG_PROTECT".
 */
void KurooInit::getEnvironment()
{
	DEBUG_LINE_INFO;
	eProc = new KProcess();
	*eProc << "emerge" << "--info";
	eProc->setOutputChannelMode( KProcess::OnlyStdoutChannel );
	connect(eProc, static_cast<void (KProcess::*)(int)>(&KProcess::finished), this, &KurooInit::slotEmergeInfo);
	connect(eProc, &KProcess::readyReadStandardOutput, this, &KurooInit::slotCollectOutput);
	eProc->start();
}

void KurooInit::slotCollectOutput()
{
	QByteArray line;
	while( !( line = eProc->readLine() ).isEmpty() )
		m_emergeInfoLines += QString( line );
}

void KurooInit::slotEmergeInfo()
{
	qDebug() << "Parsing emerge --info";
	foreach( QString line, m_emergeInfoLines ) {
		if ( line.startsWith( "Portage 2.0" ) ) {
			KurooConfig::setPortageVersion21( false );
		} else {
			KurooConfig::setPortageVersion21( true );
		}

		if ( line.startsWith( "ACCEPT_KEYWORDS=" ) ) {
			QString arch = line.section( "\"", 1, 1 );

			// When testing we have two keywords, only pick one
			if( arch.contains( "~" ) ) {
				arch = arch.section( "~", 1, 1 );
			}

			KurooConfig::setArch( arch );
		}

		if ( line.startsWith( "CONFIG_PROTECT=" ) )
			KurooConfig::setConfigProtectList( line.section( "\"", 1, 1 ) );
	}

	qDebug() << "KurooConfig::arch()=" << KurooConfig::arch();

	//KurooConfig::writeConfig();
	DEBUG_LINE_INFO;
}

/**
 * Run wizard to inform user of latest changes and collect user settings like kuroo DirHome directory,
 * and overlay location.
 * If user aborts the wizard it will be relaunched again next time.
 */
void KurooInit::firstTimeWizard()
{
	IntroDlg wizardDialog;
	qDebug() << "Running Wizard";
	if( wizardDialog.exec() == QDialog::Accepted ) {
		KurooConfig::setWizard( false );
	} else {
		exit(0); //is this the correct way to exit ?
	}
	KurooConfig::setInit( true );
}

/**
 * Control if user is in portage group.
 */
void KurooInit::checkUser()
{
	QStringList userGroups = KUser().groupNames();
	foreach( QString user, userGroups ) {
		if ( user == "portage" )
			return;
	}

	KMessageBox::error( 0, i18n("You don't have enough permissions to run kuroo.\nPlease add yourself into portage group!"),
						   i18n("User permissions") );
	exit(0);
}

void KurooInit::checkEtcFiles()
{
	QStringList paths;
	paths	<< "FilePackageUserUnMask" << "/etc/portage/package.unmask"
		<< "FilePackageKeywords" << "/etc/portage/package.keywords"
		<< "FilePackageUserMask" << "/etc/portage/package.mask"
		<< "FilePackageUserUse" << "/etc/portage/package.use";

	for(int i = 0; i < paths.count(); ++i)
	{
		QStringList list;
		QString configOption = paths[i];
		QString path = paths[++i];
		QFileInfo fInfo(path);

		if (!fInfo.exists())
		{
			QFile file(path);
			file.open(QIODevice::ReadWrite);
			file.close();
			list << path;
		}
		else if (fInfo.isDir())
		{
			QDir dir(path);
			dir.setFilter(QDir::Files | QDir::NoSymLinks);
			QFileInfoList fiList = dir.entryInfoList();
			for (int j = 0; j < fiList.size(); ++j)
			{
				QFileInfo fileInfo = fiList.at(j);
				//qDebug() << "Found" << fileInfo.absoluteFilePath();
				list << fileInfo.absoluteFilePath();
			}
			if (list.isEmpty())
			{
				QFile file(path.append("/all"));
				file.open(QIODevice::ReadWrite);
				file.close();
				list << path;
			}
		}
		else if (fInfo.isFile())
		{
			list << path;
		}

		//qDebug() << configOption << ":" << list;
		//TODO:check if default file exists.
		if (configOption == "FilePackageUserUnMask")
		{
			KurooConfig::setFilePackageUserUnMask(list);
			if (KurooConfig::defaultFilePackageUserUnMask() == ""/* || !QFileInfo::exists(KurooConfig::defaultFilePackageUserUnMask())*/)
				KurooConfig::setDefaultFilePackageUserUnMask(list[0]);
		}
		if (configOption == "FilePackageKeywords")
		{
			KurooConfig::setFilePackageKeywords(list);
			if (KurooConfig::defaultFilePackageKeywords() == ""/* || !QFileInfo::exists(KurooConfig::defaultFilePackageUserUnMask())*/)
				KurooConfig::setDefaultFilePackageKeywords(list[0]);
		}
		if (configOption == "FilePackageUserMask")
		{
			KurooConfig::setFilePackageUserMask(list);
			if (KurooConfig::defaultFilePackageUserMask() == ""/* || !QFileInfo::exists(KurooConfig::defaultFilePackageUserUnMask())*/)
				KurooConfig::setDefaultFilePackageUserMask(list[0]);
		}
		if (configOption == "FilePackageUserUse")
		{
			KurooConfig::setFilePackageUserUse(list);
			if (KurooConfig::defaultFilePackageUserUse() == ""/* || !QFileInfo::exists(KurooConfig::defaultFilePackageUserUnMask())*/)
				KurooConfig::setDefaultFilePackageUserUse(list[0]);
		}

	}
}
