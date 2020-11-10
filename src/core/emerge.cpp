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
#include "statusbar.h"
#include "settings.h"

#include <QTextCodec>
#include <QDebug>

#include <KMessageBox>
#include <KUser>
//#include <kdeversion.h>

#include <signal.h>
#include <KLocalizedString>

/**
 * @class Emerge
 * @short All Gentoo emerge command.
 *
 * Handles emerge, unmerge, check-for-updates, sync...
 */
Emerge::Emerge( QObject* m_parent )
	: QObject( m_parent ), m_completedFlag( false ), m_importantMessagePackage( QString::null ), m_packageMessage( QString::null )
{
	eProc = new KProcess();
	//use merged mode because emerge seems to output everything on stdout when there's any error (like a slot conflict)
	//including all the useful information
	eProc->setOutputChannelMode( KProcess::MergedChannels );
	ioRevdepRebuild = NULL;
	eClean1 = NULL;
}

Emerge::~Emerge()
{
	disconnect(eProc, SIGNAL(finished(int, QProcess::ExitStatus)));
	delete eProc;
	eProc = 0;
}

void Emerge::init( QObject *parent )
{
	m_parent = parent;
	m_backupComplete = false;
	m_backingUp = false;
}

/**
 * Send text to stdIn.
 * @param text
 */
void Emerge::inputText( const QString& text )
{
	if ( eProc->state() == QProcess::Running ) {
		eProc->write( text.toUtf8() );
		LogSingleton::Instance()->writeLog( text, KUROO );
	}
	else
		LogSingleton::Instance()->writeLog( i18n( "Can not process input! Emerge is not running." ), ERROR );
}

/**
 * Abort the emerge process.
 * @return success
 */
bool Emerge::stop()
{
	if ( eProc->state() == QProcess::Running && kill( eProc->pid(), 9 /*FIXME: magic number*/ ) ) {
		qWarning() << "Emerge process killed!";
		return true;
	}
	else
		return false;
}

/**
 * Checks if emerge is running.
 * @return true if emerging, false otherwise.
 */
bool Emerge::isRunning() const { return eProc->state() == QProcess::Running; }

/**
 * Emerge list of packages.
 * @param packageList
 * @return success
 */
bool Emerge::queue( const QStringList& packageList )
{
	if( KurooConfig::backupPkg() && !m_backupComplete ) {
		m_backingUp = true;
		Emerge::quickpkg( packageList );
		return true;
	}
	else {
		m_backupComplete = false;
		m_blocks.clear();
		m_importantMessage.clear();
		m_unmasked.clear();
		m_lastEmergeList = packageList;
		m_etcUpdateCount = 0;

		m_emergePackageList.clear();
		eProc->close();
		eProc->clearProgram();
		*eProc << "emerge" << "--nospinner" << "--columns" << "--color=n";

		// Add emerge options and packages
		foreach( QString package, packageList ) {
			*eProc << package;
		}

		SignalistSingleton::Instance()->setKurooBusy( true );
		eProc->start();

		if ( eProc->state() == QProcess::NotRunning ) {
			LogSingleton::Instance()->writeLog( i18n("\nError: Emerge didn't start. "), ERROR );
			cleanupQueue();

			return false;
		}

		connect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
		connect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupQueue);

		if( KurooConfig::enableEclean() && !skipHousekeeping())
			m_doeclean = true;
		if( KurooConfig::revdepEnabled() && !skipHousekeeping())
			m_dorevdeprebuild = true;
		m_pausable = true;

		LogSingleton::Instance()->writeLog( i18n( "\nEmerge %1 started...", packageList.join(" ") ), KUROO );
		qDebug() << i18n( "\nEmerge %1 started...", packageList.join(" ") );
		KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Installing packages in queue..." ) );
		KurooStatusBar::instance()->startTimer();

		return true;
	}
}


/**
 * Pause the eproc
 */
void Emerge::slotPause()
{
	if ( !(eProc->state() == QProcess::Running) || !m_pausable ) {
		LogSingleton::Instance()->writeLog( i18n( "\nEmerge is not running or cannot be paused..." ), ERROR );
	}
	else {
		KurooStatusBar::instance()->pauseTimers();
		QueueSingleton::Instance()->pauseEmerge();
		kill( eProc->pid(), SIGSTOP );
		m_isPaused = true;
	}
}

/**
 * Unpause the eproc
 */
void Emerge::slotUnpause()
{
	if ( !m_isPaused ) {
		LogSingleton::Instance()->writeLog( i18n( "\nEmerge is not paused..." ), ERROR );
	}
	KurooStatusBar::instance()->setProgressStatus("Emerge", i18n( "Installing packages in queue..." ) );
	KurooStatusBar::instance()->unpauseTimers();
	QueueSingleton::Instance()->unpauseEmerge();
	kill( eProc->pid(), SIGCONT );
	m_isPaused = false;
}

/**
 * Quickpkg something
 * @param packageList
 * @return success
 */
bool Emerge::quickpkg( const QStringList& packageList )
{
	bool ret = false;
	m_lastEmergeList = packageList;

	eProc->close();
	eProc->clearProgram();
	*eProc << "quickpkg";

	// Add the packages
	foreach( QString package, packageList ) {
		*eProc << package;
	}

	eProc->start();
	connect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);

	if ( m_backingUp )
		connect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotBackupComplete);
	else
		connect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupQueue);

	m_backingUp = false;

	SignalistSingleton::Instance()->setKurooBusy( true );

	if ( eProc->state() == QProcess::NotRunning ) {
		LogSingleton::Instance()->writeLog( i18n( "\nError: Quickpkg didn't start. " ), ERROR );
		cleanupQueue();
	}
	else {
		LogSingleton::Instance()->writeLog( i18n( "Quickpkg %1 started...", packageList.join(" ") ), KUROO );
		KurooStatusBar::instance()->setProgressStatus( "Quickpkg", i18n( "Building binary packages..." ) );
		KurooStatusBar::instance()->startTimer();
		ret = true;
	}

	return ret;
}

/**
 * Emerge pretend list of packages.
 * @param packageList
 * @return success
 */
bool Emerge::pretend( const QStringList& packageList )
{
	m_blocks.clear();
	m_importantMessage = QString::null;
	m_unmasked = QString::null;
	m_lastEmergeList = packageList;
	m_etcUpdateCount = 0;

	m_emergePackageList.clear();
	eProc->close();
	eProc->clearProgram();

	*eProc << "emerge" << "--nospinner" << "--color=n" << "--columns" << "-pv";

	// Add argument for each of the attached packages
	foreach( QString package, packageList ) {
		*eProc << package;
	}

	eProc->start();
	connect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	connect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupPretend);
	SignalistSingleton::Instance()->setKurooBusy( true );
	LogSingleton::Instance()->writeLog( i18n( "\nEmerge pretend %1 started...", packageList.join(" ") ), KUROO );
	KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Checking installation queue..." ) );
	KurooStatusBar::instance()->startProgress();
	return true;
}

/**
 * Unmerge list of packages.
 * @param packageList
 * @return success
 */
bool Emerge::unmerge( const QStringList& packageList )
{
	m_blocks.clear();
	m_etcUpdateCount = 0;
	m_importantMessage.clear();
	m_emergePackageList.clear();

	eProc->close();
	eProc->clearProgram();
	*eProc << "emerge" << "--unmerge" << "--color=n" << "--nospinner";

	// Add argument for each of the attached packages
	foreach( QString package, packageList ) {
		*eProc << package;
	}

	eProc->start();
	connect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	connect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupUnmerge);
	SignalistSingleton::Instance()->setKurooBusy( true );
	LogSingleton::Instance()->writeLog( i18n( "\nUnmerge %1 started...", packageList.join(" ") ), KUROO );
	KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Uninstalling packages..." ) );
	KurooStatusBar::instance()->startProgress();
	return true;
}

/**
 * Synchronize Portage tree.
 * @return success
 */
bool Emerge::sync()
{
	m_blocks.clear();
	m_importantMessage = QString::null;
	m_etcUpdateCount = 0;
	m_emergePackageList.clear();

	eProc->close();
	eProc->clearProgram();
	*eProc << "emerge" << "--sync" << "--quiet" << "--color=n" << "--nospinner";

	eProc->start();

	connect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	connect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupSync);
	SignalistSingleton::Instance()->setKurooBusy( true );
	LogSingleton::Instance()->writeLog( i18n( "\nEmerge synchronize Portage Tree started..." ), KUROO );
	KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Synchronizing portage tree..." ) );
	KurooStatusBar::instance()->setTotalSteps( KurooDBSingleton::Instance()->getKurooDbMeta( "syncDuration" ).toInt() );
	return true;
}

/**
 * Check for updates of @world and system.
 * @return success
 */
bool Emerge::checkUpdates()
{
	m_blocks.clear();
	m_importantMessage = QString::null;
	m_etcUpdateCount = 0;
	m_emergePackageList.clear();

	eProc->close();
	eProc->clearProgram();
	*eProc << "emerge" << "-pvu" << "--color=n" << "--nospinner" << "--columns";

	// Add deep if checked in gui
	if ( KurooConfig::updateDeep() )
		*eProc << "-D";

	if( KurooConfig::updateNewUse() )
		*eProc << "-N";

	if( KurooConfig::updateBuilddeps() )
		*eProc << "--with-bdeps=y";

	*eProc << "@world";

	eProc->start();
	if (eProc->state() == QProcess::NotRunning)
		return false;
	connect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	connect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupCheckUpdates);
	SignalistSingleton::Instance()->scanUpdatesStarted();
	LogSingleton::Instance()->writeLog( i18n( "\nEmerge check package updates started..." ), KUROO );
	//KurooStatusBar::instance()->startProgress();
	return true;
}

/**
 * Parse emerge process output for messages and packages.
 */
void Emerge::slotEmergeOutput()
{
	QRegExp rxPackage = rxEmerge();
	QByteArray data = eProc->readAllStandardOutput();
	QList<QByteArray> lines;
	if (!data.contains('\n'))
	{
		m_buffer.append(data);
		return;
	}
	else
	{
		lines = data.split('\n');
		lines[0] = m_buffer.append(lines[0]);
		m_buffer = lines.takeLast();
	}
	foreach (QByteArray baline, lines) {
		QString line(baline);
		int logDone = 0;

		////////////////////////////////////////////////////////////////////////////////
		// Cleanup emerge output - remove damn escape sequences
		////////////////////////////////////////////////////////////////////////////////
		line.replace( QRegExp("\\x0007"), "\n" );
		int pos = 0;
		QRegExp rx( "(\\x0008)|(\\x001b\\[32;01m)|(\\x001b\\[0m)|(\\x001b\\[A)|(\\x001b\\[73G)|"
				"(\\x001b\\[34;01m)|(\\x001b\\]2;)|(\\x001b\\[39;49;00m)|(\\x001b\\[01m.)" );
		while ( ( pos = rx.indexIn(line) ) != -1 )
			line.replace( pos, rx.matchedLength(), QString::null );

		if ( line.isEmpty() )
			continue;

		////////////////////////////////////////////////////////////////////////////
		// Parse out package and info
		////////////////////////////////////////////////////////////////////////////
		if ( rxPackage.indexIn( line ) > -1 ) {
			EmergePackage emergePackage;
			emergePackage.updateFlags = rxPackage.cap(1);
			emergePackage.package = rxPackage.cap(2);
			emergePackage.category = rxPackage.cap(3);
			emergePackage.name = rxPackage.cap(4);
			emergePackage.version = rxPackage.cap(5);
			emergePackage.installedVersion = rxPackage.cap(6);
			emergePackage.useFlags = rxPackage.cap(7).simplified();
			emergePackage.size = rxPackage.cap(8);
			//qDebug() << emergePackage.size;
			//Fix bug #3163827 order of queue items
			m_emergePackageList.prepend( emergePackage );
		}

		////////////////////////////////////////////////////////////////////////
		// Parse emerge output for correct log output
		////////////////////////////////////////////////////////////////////////
		QString lineLower = line.toLower();
		if ( lineLower.contains( QRegExp("^>>>|^!!!") ) ) {

			if ( lineLower.contains( QRegExp("^>>> completed installing") ) ) {
				m_completedFlag = true;
// 				m_importantMessagePackage = line.section( "Completed installing ", 1, 1 ).section( " ", 0, 0 ) + ":<br>";
			}
			else
				if ( lineLower.contains( QRegExp("^>>> regenerating") ) )
					m_completedFlag = false;

			if ( lineLower.contains( QRegExp("^!!! error") ) ) {
				LogSingleton::Instance()->writeLog( line, ERROR );
				logDone++;
			}
			else
// 				if ( lineLower.contains( "etc-update" ) ) {
// 					LogSingleton::Instance()->writeLog( line, ERROR );
// 					logDone++;
// 				}
// 				else
					if ( lineLower.contains( QRegExp("^!!!") ) ) {
						LogSingleton::Instance()->writeLog( line, ERROR );
// 						m_importantMessage += line + "<br>";
						logDone++;
					}
					else
						if ( logDone == 0 && lineLower.contains( QRegExp(
							"(^>>> (merging|unmerge|unmerging|clean|unpacking source|extracting|completed|regenerating))|"
							"(^ \\* IMPORTANT)|(^>>> unmerging in)")) ) {
							LogSingleton::Instance()->writeLog( line, EMERGE );
							logDone++;
						}

		}
		else
			if ( lineLower.contains( "please tell me" ) ) {
				LogSingleton::Instance()->writeLog( line, EMERGE );
				logDone++;
			}
			else
				if ( lineLower.contains( "root access required" ) ) {
					LogSingleton::Instance()->writeLog( i18n( "Root access required!" ), ERROR );
					logDone++;
				}
				else
					if ( lineLower.contains( "no ebuilds to satisfy" ) ) {
						QString missingPackage = line.section("no ebuilds to satisfy ", 1, 1);
						LogSingleton::Instance()->writeLog( i18n( "There is no ebuilds to satisfy %1", missingPackage ), ERROR );
						logDone++;
					}
					else
						if ( lineLower.contains( " (masked by: " ) )
							m_unmasked = line.section( "- ", 1 ).section( ")", 0 );
						else
							if ( !m_unmasked.isEmpty() && line.startsWith( "# " ) )
								m_importantMessage += line.section( "# ", 1, 1 ) + "<br/>";


		// Save to kuroo.log for debugging
		LogSingleton::Instance()->writeLog( line, TOLOG );

		// Collect blocking lines
		if ( line.contains( "is blocking" ) )
			m_blocks += line.section( "[blocks B	 ]", 1, 1 ).replace( '>', "&gt;" ).replace( '<', "&lt;" );

		// Collect output line if user want full log verbose
		if ( logDone == 0 )
			LogSingleton::Instance()->writeLog( line, EMERGE );

		countEtcUpdates( lineLower );
	}
}

/**
 * Return einfo and ewarnings collected during emerge of the package.
 * @return message
 */
const QString Emerge::packageMessage()
{
	QString message = m_packageMessage;
	m_packageMessage = QString::null;
	return message;
}


//////////////////////////////////////////////////////////////////////////////
// Post emerge stuff
//////////////////////////////////////////////////////////////////////////////

/**
 * Cleanup when emerge process is
 * Stop statusbar progress and set kuroo to ready.
 * Present eventual messages collected during emerge to the user.
 */
void Emerge::cleanup()
{
	qDebug() << "cleaning up !";
	KurooStatusBar::instance()->stopTimer();
	KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Done." ) );
	SignalistSingleton::Instance()->setKurooBusy( false );
	QueueSingleton::Instance()->addPackageList( m_emergePackageList );


	if ( !m_unmasked.isEmpty() ) {
		if ( KUser().isSuperUser() ) {
			askUnmaskPackage( m_unmasked );
		}
		else {
			KMessageBox::information( 0, i18n( "You must run Kuroo as root to unmask packages!" ), i18n( "Auto-unmasking packages" ) );
		}
	}

	/* if m_doclean then perform an eclean */
	if( m_doeclean && KurooConfig::ecleanDistfiles() ) {
		QString ecleanCOMMAND;
		if (!eClean1)
			eClean1 = new KProcess();
		eClean1->close();
		eClean1->clearProgram();

		*eClean1 << "eclean";
		ecleanCOMMAND = "eclean ";
		if( !KurooConfig::ecleanTimeLimit().isEmpty() ) {
			*eClean1 << "-t" << KurooConfig::ecleanTimeLimit();
			ecleanCOMMAND += "-t";
			ecleanCOMMAND += KurooConfig::ecleanTimeLimit();
			ecleanCOMMAND += " ";
		}
		if( KurooConfig::ecleanDestructive() )
		{
			*eClean1 << "--destructive";
			ecleanCOMMAND += "--destructive ";
		}
		*eClean1 << "--nocolor";
		ecleanCOMMAND += "--nocolor ";

		if( KurooConfig::ecleanFetchRestrict() && KurooConfig::ecleanDestructive())
		{
			*eClean1 << "--fetch-restricted";
			ecleanCOMMAND += "--fetch-restricted ";
		}
		*eClean1 << "distfiles ";
		if( KurooConfig::ecleanSizeLimit().isEmpty() )
		{
			*eClean1 << "-s" << KurooConfig::ecleanSizeLimit();
			ecleanCOMMAND += "-s" + KurooConfig::ecleanSizeLimit() + " ";
		}
		ecleanCOMMAND += "distfiles";
		qDebug() << "ECLEAN COMMAND: " << ecleanCOMMAND;

		eClean1->setOutputChannelMode(KProcess::OnlyStdoutChannel);

		connect(eClean1, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
		connect(eClean1, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotEmergeDistfilesComplete);

		eClean1->start();

		SignalistSingleton::Instance()->setKurooBusy( true );
		LogSingleton::Instance()->writeLog( i18n( "\nEclean of distfiles started" ), KUROO );
		KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Cleaning distfiles..." ) );
		KurooStatusBar::instance()->startProgress();
		m_doeclean = false;
	} else if( m_dorevdeprebuild ) {
		if (!ioRevdepRebuild)
			ioRevdepRebuild = new KProcess();
		ioRevdepRebuild->close();
		ioRevdepRebuild->clearProgram();
		*ioRevdepRebuild << "revdep-rebuild";
		*ioRevdepRebuild << "--no-color";
		*ioRevdepRebuild << "--ignore";

		connect(ioRevdepRebuild, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
		connect(ioRevdepRebuild, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotRevdepRebuildComplete);

		ioRevdepRebuild->setOutputChannelMode(KProcess::OnlyStdoutChannel);
		ioRevdepRebuild->start();

		SignalistSingleton::Instance()->setKurooBusy( true );
		LogSingleton::Instance()->writeLog( i18n( "\nRevdep-rebuild Running..." ), KUROO );
		KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Running revdep-rebuild..." ) );
		KurooStatusBar::instance()->startProgress();
		m_dorevdeprebuild = false;
	} /* end revdeprebuild section */
}

/**
 * Revdep-rebuild Complete
 */
void Emerge::slotRevdepRebuildComplete()
{
	disconnect(ioRevdepRebuild, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	disconnect(ioRevdepRebuild, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotRevdepRebuildComplete);
	LogSingleton::Instance()->writeLog( i18n( "\nRevdep-rebuildcomplete" ), KUROO );
	KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Done" ) );
	KurooStatusBar::instance()->stopTimer();
	SignalistSingleton::Instance()->setKurooBusy( false );
	delete ioRevdepRebuild; // cleanup that memory
}

/**
 * Run an eclean for packages if necessary
 */
void Emerge::slotEmergeDistfilesComplete()
{
	disconnect(eClean1, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	disconnect(eClean1, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotEmergeDistfilesComplete);
	LogSingleton::Instance()->writeLog( i18n( "\nEclean of distfiles complete" ), KUROO );
	KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Done" ) );
	KurooStatusBar::instance()->stopTimer();
	SignalistSingleton::Instance()->setKurooBusy( false );
	delete eClean1; // cleanup that memory

	if( KurooConfig::ecleanDistfiles() )
	{
		//QTextCodec *codec = QTextCodec::codecForName("utf8");
		eClean2 = new KProcess();
		//eClean2->setUseShell( true, "/bin/bash" );
		//eClean2->setComm( KProcess::Communication( KProcess::Stdout | KProcess::MergedStderr | KProcess::Stdin ) );
		eClean2->close();
		*eClean2 << "eclean";
		if( KurooConfig::ecleanTimeLimit().isEmpty() )
		{
			*eClean2 << "-t" << KurooConfig::ecleanTimeLimit();
		}

		*eClean2 << "--nocolor" << "packages";

		eClean2->start( /*KProcess::OwnGroup, true*/ );
		connect(eClean2, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
		connect(eClean2, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotEClean2Complete);
		SignalistSingleton::Instance()->setKurooBusy( true );
		LogSingleton::Instance()->writeLog( i18n( "\nEclean of packages started" ), KUROO );
		KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Cleaning distfiles..." ) );
		KurooStatusBar::instance()->startProgress();
		return;
	} else if( m_dorevdeprebuild ) {
		//QTextCodec *revdepcodec = QTextCodec::codecForName("utf8");
		ioRevdepRebuild = new KProcess();
		//ioRevdepRebuild->setUseShell( false, NULL );
		//ioRevdepRebuild->setComm( KProcess::Communication( KProcess::Stdout | KProcess::MergedStderr | KProcess::Stdin ) );
		ioRevdepRebuild->close();
		*ioRevdepRebuild << "revdep-rebuild";
		*ioRevdepRebuild << "--no-color";
		*ioRevdepRebuild << "--ignore";
		ioRevdepRebuild->start();

		connect(ioRevdepRebuild, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
		connect(ioRevdepRebuild, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotRevdepRebuildComplete);
		SignalistSingleton::Instance()->setKurooBusy( true );
		LogSingleton::Instance()->writeLog( i18n( "\nRevdep-rebuild Running..." ), KUROO );
		KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Running revdep-rebuild..." ) );
		KurooStatusBar::instance()->startProgress();
		m_dorevdeprebuild = false;
	} /* end revdeprebuild section */
}

void Emerge::slotEClean2Complete(int exitCode, QProcess::ExitStatus status)
{
	Q_UNUSED(exitCode)
	Q_UNUSED(status)

	disconnect(eClean2, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	disconnect(eClean2, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotEClean2Complete);
	LogSingleton::Instance()->writeLog( i18n( "\nEcleaning packages complete" ), KUROO );
	KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Ready" ) );
	KurooStatusBar::instance()->stopTimer();
	SignalistSingleton::Instance()->setKurooBusy( false );
	delete eClean2;
	if( m_dorevdeprebuild )
	{
		//QTextCodec *revdepcodec = QTextCodec::codecForName("utf8");
		ioRevdepRebuild = new KProcess();
		//ioRevdepRebuild->setUseShell( false, NULL );
		//ioRevdepRebuild->setComm( KProcess::Communication( KProcess::Stdout | KProcess::MergedStderr | KProcess::Stdin ) );
		ioRevdepRebuild->close();
		*ioRevdepRebuild << "revdep-rebuild";
		*ioRevdepRebuild << "--no-color";
		*ioRevdepRebuild << "--ignore";
		ioRevdepRebuild->start();
		connect(ioRevdepRebuild, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
		connect(ioRevdepRebuild, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotRevdepRebuildComplete);
		SignalistSingleton::Instance()->setKurooBusy( true );
		LogSingleton::Instance()->writeLog( i18n( "\nRevdep-rebuild Running..." ), KUROO );
		KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Running revdep-rebuild..." ) );
		KurooStatusBar::instance()->startProgress();
		m_dorevdeprebuild = false;
	} /* end revdeprebuild section */
}

/**
 * Disconnect signals and signal termination to main thread.
 */
void Emerge::slotBackupComplete(int exitCode, QProcess::ExitStatus status)
{
	Q_UNUSED(exitCode)
	Q_UNUSED(status)

	disconnect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	disconnect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotBackupComplete);
	HistorySingleton::Instance()->updateStatistics();
	m_backupComplete = 1;
	Emerge::queue(m_lastEmergeList);
}

/**
 * Disconnect signals and signal termination to main thread.
 */
void Emerge::slotCleanupQueue(int exitCode, QProcess::ExitStatus status)
{
	Q_UNUSED(exitCode)
	Q_UNUSED(status)

	cleanupQueue();
}

void Emerge::cleanupQueue()
{
	disconnect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	disconnect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupQueue);
	cleanup();
	HistorySingleton::Instance()->updateStatistics();
	m_pausable = false;
	emit signalEmergeComplete();
}

/**
 * Disconnect signals and signal termination to main thread.
 */
void Emerge::slotCleanupPretend(int exitCode, QProcess::ExitStatus status)
{
	Q_UNUSED(exitCode)
	Q_UNUSED(status)

	disconnect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	disconnect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupPretend);
	cleanup();
}

/**
 * Disconnect signals and signal termination to main thread.
 * @param proc
 */
void Emerge::slotCleanupUnmerge(int exitCode, QProcess::ExitStatus status)
{
	Q_UNUSED(exitCode)
	Q_UNUSED(status)

	disconnect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	disconnect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupUnmerge);
	cleanup();
}

/**
 * Disconnect signals and signal termination to main thread.
 */
void Emerge::slotCleanupSync(int exitCode, QProcess::ExitStatus status)
{
	Q_UNUSED(exitCode)
	Q_UNUSED(status)

	disconnect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	disconnect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupSync);
	cleanup();
}

/**
 * Disconnect signals and signal termination to main thread.
 */
void Emerge::slotCleanupCheckUpdates(int exitCode, QProcess::ExitStatus status)
{
	Q_UNUSED(exitCode)
	Q_UNUSED(status)

	disconnect(eProc, &KProcess::readyReadStandardOutput, this, &Emerge::slotEmergeOutput);
	disconnect(eProc, static_cast<void(KProcess::*)(int,QProcess::ExitStatus)>(&KProcess::finished), this, &Emerge::slotCleanupCheckUpdates);

	KurooStatusBar::instance()->stopTimer();
	//KurooStatusBar::instance()->setProgressStatus( "Emerge", i18n( "Done." ) );
	SignalistSingleton::Instance()->scanUpdatesComplete();

	if ( !m_blocks.isEmpty() ) {
		if ( !m_importantMessage.isEmpty() )
			m_importantMessage += "<br/>";
		m_importantMessage += m_blocks.join("<br/>");
	}

	/*if ( !m_importantMessage.isEmpty() )
	Message::instance()->prompt( i18n("Important"), i18n("Please check log for more information!"), m_importantMessage );*/
}

/**
 * Check if package is masked, if so ask to unmask.
 * @param packageKeyword	concatenated string with package and mask
 */
void Emerge::askUnmaskPackage( const QString& packageKeyword )
{
	QString package = packageKeyword.section( "(masked by: ", 0, 0 );
	QString keyword = ( packageKeyword.section( "(masked by: ", 1, 1) ).section( " keyword", 0, 0 );

	if ( packageKeyword.contains( "missing keyword" ) ) {
		m_importantMessage += i18n( "%1 is not available on your architecture %2!<br/><br/>", package, KurooConfig::arch() );
		m_importantMessage += i18n( "<b>missing keyword</b> means that the application has not been tested on your architecture yet.<br/>"
				"Ask the architecture porting team to test the package or test it for them and report your "
				"findings on Gentoo bugzilla website." );
		KMessageBox::information( NULL, "<qt>" + m_importantMessage + "</qt>", i18n( "Missing Keyword" ) );
	}
	else if ( keyword.contains( "-*" ) ) {
		m_importantMessage += i18n( "%1 is not available on your architecture %2!<br/><br/>", package, KurooConfig::arch() );
		m_importantMessage += i18n( "<br/><b>-* keyword</b> means that the application does not work on your architecture.<br/>"
					"If you believe the package does work file a bug at Gentoo bugzilla website." );
		KMessageBox::information( NULL, "<qt>" + m_importantMessage + "</qt>", i18n( "-* Keyword" ) );
	}
	else if ( !keyword.contains( KurooConfig::arch() ) && keyword.contains( "package.mask" ) ) {
		LogSingleton::Instance()->writeLog( i18n( "Please add package to \"package.unmask\"." ), ERROR );

		if (KMessageBox::questionYesNo( NULL,
						i18n( "<qt>Cannot emerge masked package!<br/>Do you want to unmask <b>%1</b>?</qt>", package ),
						i18n( "Information" ), KGuiItem( i18n( "Unmask" ) ), KGuiItem( i18n( "Cancel" ) ) ) == KMessageBox::Yes) {
			KurooDBSingleton::Instance()->setPackageUnMasked( KurooDBSingleton::Instance()->packageId( package ) );
			PortageFilesSingleton::Instance()->savePackageUserUnMask();
			disconnect( PortageFilesSingleton::Instance(), SIGNAL( signalPortageFilesChanged() ), this, SLOT( slotTryEmerge() ) );
			connect( PortageFilesSingleton::Instance(), SIGNAL( signalPortageFilesChanged() ), this, SLOT( slotTryEmerge() ) );
		}
	}
	else {
		LogSingleton::Instance()->writeLog( i18n( "Please add package to \"package.keywords\"." ), ERROR );

		if (KMessageBox::questionYesNo( NULL,
						i18n( "<qt>Cannot emerge testing package!<br/>Do you want to unmask <b>%1</b>?</qt>", package ),
						i18n( "Information" ), KGuiItem( i18n( "Unmask" ) ), KGuiItem( i18n( "Cancel" ) ) ) == KMessageBox::Yes) {
			KurooDBSingleton::Instance()->setPackageUnTesting( KurooDBSingleton::Instance()->packageId( package ) );
			PortageFilesSingleton::Instance()->savePackageKeywords();
			disconnect( PortageFilesSingleton::Instance(), SIGNAL( signalPortageFilesChanged() ), this, SLOT( slotTryEmerge() ) );
			connect( PortageFilesSingleton::Instance(), SIGNAL( signalPortageFilesChanged() ), this, SLOT( slotTryEmerge() ) );
		}
	}
}

/**
 * After package is auto-unmasked try remerging the list again to find next package to unmask.
 */
void Emerge::slotTryEmerge()
{
	pretend( m_lastEmergeList );
	disconnect( PortageFilesSingleton::Instance(), SIGNAL( signalPortageFilesChanged() ), this, SLOT( slotTryEmerge() ) );
}

/**
 * count etc-files to merge.
 * @param line line
 * @return success
 */
bool Emerge::countEtcUpdates( const QString& line )
{
	// count etc-files to merge
	if ( line.contains( " need updating" ) ) {
		QString tmp = line.section( "config files", 0, 0 );
		QRegExp rx("^\\d+\\)");
		int pos = rx.indexIn(tmp);
		if ( pos > -1 )
			m_etcUpdateCount += ( rx.cap(1) ).toInt();

		return true;
	}
	else
		return false;
}

