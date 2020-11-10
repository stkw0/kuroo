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

#include <unistd.h>

//#include <kdeversion.h>
#include <KActionCollection>
#include <KIO/Job>
#include <KMessageBox>
#include <KStandardAction>
#include <KStandardShortcut>
#include <KUser>

#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Queue>
#include <QDebug>
#include <QAction>
#include <QTimer>
#include <QCheckBox>


#include "common.h"
#include "systemtray.h"
#include "kurooinit.h"
#include "kuroo.h"
#include "statusbar.h"
#include "introdlg.h"
#include "portagetab.h"
#include "logstab.h"

/**
* @class Kuroo
* @short Main kde window with menus, system tray icon and statusbar.
*/
Kuroo::Kuroo() : KXmlGuiWindow( 0 ),
	kurooMessage( new Message( this ) ),
	kurooInit( new KurooInit( this ) ), m_view( new KurooView( this ) ),
	prefDialog( 0 ), wizardDialog( 0 ), m_shuttingDown( false )
{
	qDebug() << "Initializing Kuroo GUI";

	setCentralWidget( m_view );
	setupActions();
	setStatusBar( new KurooStatusBar( this ) );

	// Add system tray icon
	if ( KurooConfig::isSystrayEnabled() ) {
		systemTray = new SystemTray( this );
		connect(systemTray, &SystemTray::signalPreferences, this, &Kuroo::slotPreferences);
	}

	// Lock/unlock if kuroo is busy.
	connect( SignalistSingleton::Instance(), SIGNAL( signalKurooBusy( bool ) ), this, SLOT( slotBusy() ) );

	// when the last window is closed, the application should quit
    //TODO: check whether to connect this signal in main.cpp
	//connect( kapp, SIGNAL( lastWindowClosed() ), kapp, SLOT( quit() ) );

	// Kuroo must initialize with db first
	SignalistSingleton::Instance()->setKurooReady( false );

	// Initialize with settings in make.conf
	prefDialog = KConfigDialog::exists( i18n( "settings" ) );
	if ( !prefDialog )
		prefDialog = new ConfigDialog( m_view, i18n( "settings" ), KurooConfig::self() );

	// Zack Rusin's delayed initialization technique
	QTimer::singleShot( 0, m_view, SLOT( slotInit() ) );
}

/**
* If necessary wait for job to finish before terminating.
*/
Kuroo::~Kuroo()
{
	int maxLoops( 99 );
	while ( true ) {
		//if ( ThreadWeaver::Queue::instance()->isJobPending( "DBJob" ) || ThreadWeaver::Queue::instance()->isJobPending( "CachePortageJob" ) )
		if (!ThreadWeaver::Queue::instance()->isIdle())
			::usleep( 100000 ); // Sleep 100 msec
		else
			break;

		if ( maxLoops-- == 0 ) {
			KMessageBox::error( 0, i18n("Kuroo is not responding. Attempting to terminate kuroo!"), i18n("Terminating") );
			break;
		}
	}
}

/**
* Build mainwindow menus and toolbar.
*/
void Kuroo::setupActions()
{
	KStandardAction::quit( this, SLOT( slotQuit() ), actionCollection() );
	KStandardAction::preferences( this, SLOT( slotPreferences() ), actionCollection() );

	QAction * actionReleaseInfo = new QAction( i18n("&Release information"), this );
	//actionReleaseInfo->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_W ) );
	actionCollection()->setDefaultShortcut( actionReleaseInfo,  QKeySequence( Qt::CTRL + Qt::Key_W ) );
	actionCollection()->addAction( "information", actionReleaseInfo );
	connect(actionReleaseInfo, &QAction::triggered, this, &Kuroo::introWizard);
	/*
	actionReleaseInfo = new QAction(, 0,
									this, SLOT( introWizard() ), actionCollection(), "information" );
	actionReleaseInfo->setText( i18n("&Release information") );
	actionReleaseInfo->setShortcut(  );
	*/
	actionRefreshPortage = new QAction( i18n("&Refresh Packages"), this );
	//actionRefreshPortage->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_P ) );
	//PortageSingleton::Instance() , SLOT( slotRefresh() ), actionCollection(), "refresh_portage" );
	actionCollection()->setDefaultShortcut( actionRefreshPortage, QKeySequence( Qt::CTRL + Qt::Key_P ) );
	actionCollection()->addAction( "refresh_portage", actionRefreshPortage );
	connect( actionRefreshPortage, SIGNAL(triggered(bool)), PortageSingleton::Instance(), SLOT( slotRefresh() ) );

	actionRefreshUpdates = new QAction( i18n("&Refresh Updates"), this );
	//actionRefreshUpdates->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_U ));
	actionCollection()->setDefaultShortcut( actionRefreshUpdates, QKeySequence( Qt::CTRL + Qt::Key_U ) );
	actionCollection()->addAction( "refresh_updates", actionRefreshUpdates );
	connect( actionRefreshUpdates, SIGNAL(triggered(bool)), PortageSingleton::Instance(), SLOT( slotRefreshUpdates()) );

	actionSyncPortage = new QAction( i18n("&Sync Portage"), this );
	//actionSyncPortage->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_S ) );
	actionCollection()->setDefaultShortcut( actionSyncPortage, QKeySequence( Qt::CTRL + Qt::Key_S ) );
	actionCollection()->addAction( "sync_portage", actionSyncPortage );
	connect(actionSyncPortage, &QAction::triggered, this, &Kuroo::slotSync);
	/*
	actionRefreshUpdates = new QAction( i18n("&Refresh Updates"), 0, QKeySequence( Qt::CTRL + Qt::Key_U ),
										PortageSingleton::Instance() , SLOT( slotRefreshUpdates() ), actionCollection(), "refresh_updates" );

	actionSyncPortage = new QAction( i18n("&Sync Portage"), 0, ,
										, ,  );
										*/
	setupGUI( Default, "kurooui.rc" );
}

/**
* Disable buttons when kuroo is busy.
*/
void Kuroo::slotBusy()
{
	bool isBusy = (SignalistSingleton::Instance()->isKurooBusy() || EmergeSingleton::Instance()->isRunning());
	if ( !isBusy ) {
		// Make sure progressbar is stopped!
		KurooStatusBar::instance()->stopTimer();
	}
	actionRefreshPortage->setEnabled( !isBusy );
	actionRefreshUpdates->setEnabled( !isBusy );

	if ( EmergeSingleton::Instance()->isRunning() || SignalistSingleton::Instance()->isKurooBusy() ||
		!KUser().isSuperUser() || KurooDBSingleton::Instance()->isPortageEmpty() ) {
		actionSyncPortage->setEnabled( false );
	}
	else {
		actionSyncPortage->setEnabled( true );
	}

	// No db no fun!
	if ( !SignalistSingleton::Instance()->isKurooReady() ) {
		actionRefreshPortage->setEnabled( false );
		actionRefreshUpdates->setEnabled( false );
		actionSyncPortage->setEnabled( false );
	}
}

/**
* Launch emerge portage sync.
*/
void Kuroo::slotSync()
{
	KLocale *loc = KLocale::global();
	QDateTime t;
	QString timeStamp( KurooDBSingleton::Instance()->getKurooDbMeta( "syncTimeStamp" ) );
	QString lastSyncDate( QString::null );

	if ( !timeStamp.isEmpty() ) {
		t.setTime_t( timeStamp.toUInt() );
		lastSyncDate = loc->formatDateTime(t);
	} else {
		lastSyncDate = "never";
	}

	switch( KMessageBox::questionYesNo( this,
		i18n( "<qt>Do you want to synchronize portage?<br/>"
			"This will take a couple of minutes...</qt>" ), i18n( "Last sync: %1", lastSyncDate ) ) ) {

		case KMessageBox::Yes:
			PortageSingleton::Instance()->slotSync();
		default:
			break;
	}
}

/**
* Open kuroo preferences window.
*/
void Kuroo::slotPreferences()
{
	prefDialog = KConfigDialog::exists( i18n( "settings" ) );
	if ( !prefDialog )
		prefDialog = new ConfigDialog( m_view, i18n( "settings" ), KurooConfig::self() );
	prefDialog->show();
	prefDialog->raise();
	//prefDialog->setActiveWindow();
}

/**
* Show the wizard.
*/
void Kuroo::introWizard()
{
	if ( !wizardDialog )
		wizardDialog = new IntroDlg( /*this */);

	wizardDialog->show();
}

/**
* Hide or minimize kuroo window when clicking in close button.
*/
bool Kuroo::queryClose()
{
	if ( !m_shuttingDown ) {
		if ( !KurooConfig::isSystrayEnabled() )
			slotQuit();
		else {
			hide();
			return false;
		}
	}

	return true;
}

/**
* Bye, bye!
*/
bool Kuroo::queryExit()
{
	DEBUG_LINE_INFO;
	//Quit from the system tray icon calls this instead of the quit action on the main window
	if ( !m_shuttingDown ) {
		slotQuit();
	}
	return true;
}

/**
* Backup emerge and merge history entries to text file.
* Wait for the backup of the log is completed before terminating.
*/
void Kuroo::slotQuit()
{
	qDebug() << "Cleaning up after ourselves";
	KurooDBSingleton::Instance()->backupDb();
	KIO::Job *backupLogJob = LogSingleton::Instance()->backupLog();
	if ( backupLogJob != NULL )
		connect(backupLogJob, &KIO::Job::result, this, &Kuroo::slotWait);
	else
		slotWait();
}

/**
* Abort any running threads.
*/
void Kuroo::slotWait()
{
	if ( SignalistSingleton::Instance()->isKurooBusy() ) {
		switch( KMessageBox::questionYesNo( this,
			i18n("<qt>Kuroo is busy<br/><br/>"
				"Do you want to quit?<br/>"
				"All jobs will be aborted.</qt>"), i18n("Quit") ) ) {

			case KMessageBox::Yes: {
				ThreadWeaver::Queue::instance()->requestAbort();//AllJobsNamed( "DBJob" );
				//ThreadWeaver::Queue::instance()->abortAllJobsNamed( "CachePortageJob" );
				QTimer::singleShot( 500, this, SLOT( slotTerminate() ) );
			}
			default:
				break;
		}
	}
	else
		slotTerminate();
}

/**
* Terminate kuroo.
*/
void Kuroo::slotTerminate()
{
	m_shuttingDown = true;
	close();
}

