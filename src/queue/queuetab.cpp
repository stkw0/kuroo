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
#include "statusbar.h"
#include "queuetab.h"
#include "queuelistitem.h"
#include "queuelistview.h"
#include "packageinspector.h"
#include "versionview.h"
#include "packageversion.h"
#include "ui_queuebase.h"

#include <QCheckBox>
#include <QWhatsThis>

#include <QPushButton>
#include <KMessageBox>
#include <QMenu>
#include <KUser>
//#include <kaccel.h>
#include <QAction>

/**
* @class QueueTab
* @short Page for the installation queue.
*/
QueueTab::QueueTab( QWidget* parent, PackageInspector *packageInspector )
	: QWidget( parent ), m_hasCheckedQueue( false ), m_initialQueueTime( QString::null ), m_packageInspector( packageInspector )
{
	setupUi( this );
	// Connect What's this button
	connect(pbWhatsThis, &QPushButton::clicked, this, &QueueTab::slotWhatsThis);

	// Rmb actions.
	queueView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(queueView, &QueueListView::customContextMenuRequested, this, &QueueTab::slotContextMenu);

	m_removeFromQueue = new QAction( QIcon::fromTheme(QStringLiteral("list-remove")), i18n( "&Remove"), this );
	connect(m_removeFromQueue, &QAction::triggered, this, &QueueTab::slotRemove);
	m_removeFromWorld = new QAction( QIcon::fromTheme(QStringLiteral("kuroo_world")), i18n( "Remove From &World"), this );
	connect(m_removeFromWorld, &QAction::triggered, this, &QueueTab::slotRemoveWorld);
	m_addToWorld = new QAction( QIcon::fromTheme(QStringLiteral("kuroo_world")), i18n( "Add To &World"), this );
	connect(m_addToWorld, &QAction::triggered, this, &QueueTab::slotAddWorld);
	m_clearQueue = new QAction( QIcon::fromTheme(QStringLiteral("edit-clear")), i18n( "Remove &All"), this );
	connect(m_clearQueue, &QAction::triggered, this, &QueueTab::slotClear);
	m_packageDetails = new QAction( i18n( "&Details"), this );
	connect(m_packageDetails, &QAction::triggered, this, &QueueTab::slotAdvanced);

	// Button actions.
	connect(pbCheck, &QPushButton::clicked, this, &QueueTab::slotCheck);
	connect(pbRemove, &QPushButton::clicked, this, &QueueTab::slotRemove);
	connect(pbClear, &QPushButton::clicked, this, &QueueTab::slotClear);
	connect(pbAdvanced, &QPushButton::clicked, this, &QueueTab::slotAdvanced);
	connect(queueView, &QueueListView::itemDoubleClicked, this, &QueueTab::slotAdvanced);

	connect(cbRemove, &QCheckBox::clicked, this, &QueueTab::slotRemoveInstalled);

	connect(queueView, &QueueListView::selectionChangedSignal, this, &QueueTab::slotPackage);
	connect(queueView, &QueueListView::selectionChangedSignal, this, &QueueTab::slotButtons);

	// Lock/unlock if kuroo is busy
	connect( SignalistSingleton::Instance(), SIGNAL( signalKurooBusy( bool ) ), this, SLOT( slotBusy() ) );

	// Reload view after changes in queue.
	connect( QueueSingleton::Instance(), SIGNAL( signalQueueChanged( bool ) ), this, SLOT( slotReload( bool ) ) );
	connect( PortageSingleton::Instance(), SIGNAL( signalPortageChanged() ), this, SLOT( slotRefresh() ) );

	// Forward emerge start/stop/completed to package progressbar.
	connect( QueueSingleton::Instance(), SIGNAL( signalPackageStart( const QString& ) ), queueView, SLOT( slotPackageStart( const QString& ) ) );
	connect( QueueSingleton::Instance(), SIGNAL( signalPackageComplete( const QString& ) ), queueView, SLOT( slotPackageComplete( const QString& ) ) );
	connect( QueueSingleton::Instance(), SIGNAL( signalPackageAdvance() ), queueView, SLOT( slotPackageProgress() ) );

	// Update Queue summary timer
	connect( QueueSingleton::Instance(), SIGNAL( signalPackageAdvance() ), this, SLOT( slotQueueSummary() ) );
	connect( QueueSingleton::Instance(), SIGNAL( signalPackageComplete( const QString& ) ), this, SLOT( slotQueueSummary() ) );

	// Recalculate package when user change settings in Inspector
	connect(m_packageInspector, &PackageInspector::signalPackageChanged, this, &QueueTab::slotPackage);
	connect(m_packageInspector, &PackageInspector::signalNextPackage, this, &QueueTab::slotNextPackage);
	connect(m_packageInspector, &PackageInspector::finished, this, &QueueTab::slotButtons);

	slotInit();
}

/**
* Save listview geometry.
*/
QueueTab::~QueueTab()
{
	if ( cbForceConf->isChecked() )
		KurooConfig::setForceConf( true );
	else
		KurooConfig::setForceConf( false );

	if ( cbDownload->isChecked() )
		KurooConfig::setDownload( true );
	else
		KurooConfig::setDownload( false );

	if ( cbNoWorld->isChecked() )
		KurooConfig::setNoWorld( true );
	else
		KurooConfig::setNoWorld( false );

	if ( cbRemove->isChecked() )
		KurooConfig::setRemove( true );
	else
		KurooConfig::setRemove( false );

	if ( cbBackupPkg->isChecked() )
		KurooConfig::setBackupPkg( true );
	else
		KurooConfig::setBackupPkg( false );

	if ( KurooConfig::enableEclean() || KurooConfig::revdepEnabled() )
		cbSkipHousekeeping->setDisabled( false );
	else
		cbSkipHousekeeping->setDisabled( true );
}

/**
* Initialize Queue view.
*/
void QueueTab::slotInit()
{
	//queueFrame->setPaletteBackgroundColor( colorGroup().base() );

	if ( KurooConfig::forceConf() )
		cbForceConf->setChecked( true );
	else
		cbForceConf->setChecked( false );

	if ( KurooConfig::download() )
		cbDownload->setChecked( true );
	else
		cbDownload->setChecked( false );

	if ( KurooConfig::noWorld() )
		cbNoWorld->setChecked( true );
	else
		cbNoWorld->setChecked( false );

	if ( KurooConfig::remove() )
		cbRemove->setChecked( true );
	else
		cbRemove->setChecked( false );

	if ( KurooConfig::backupPkg() )
		cbBackupPkg->setChecked( true );
	else
		cbBackupPkg->setChecked( false );

	if ( KurooConfig::enableEclean() || KurooConfig::revdepEnabled() )
		cbSkipHousekeeping->setDisabled( false );
	else
		cbSkipHousekeeping->setDisabled( true );


	cbDownload->setToolTip( i18n(  "<qt><table width=300><tr><td>Instead of doing any package building, "
									"just perform fetches for all packages (the main package as well as all dependencies), "
									"grabbing all potential files.</td></tr></table></qt>" ) );

	cbNoWorld->setToolTip( i18n(   "<qt><table width=300><tr><td>Emerge as normal, "
									"but do not add the packages to the @world profile for later updating.</td></tr></table></qt>" ) );

	cbForceConf->setToolTip( i18n( "<qt><table width=300><tr><td>Causes portage to disregard merge records indicating that a config file"
									"inside of a CONFIG_PROTECT directory has been merged already. "
									"Portage will normally merge those files only once to prevent the user"
									"from dealing with the same config multiple times. "
									"This flag will cause the file to always be merged.</td></tr></table></qt>" ) );

	cbBackupPkg->setToolTip( i18n(   "<qt><table width=300><tr><td>Emerge as normal, "
									"but use quickpkg to make a backup of the installed ebuilds before merging.</td></tr></table></qt>" ) );

	//TODO: port to kde4
	// Keyboard shortcuts
	/*KAccel* pAccel = new KAccel( this );
	pAccel->insert( "View package details...", i18n("View package details..."), i18n("View package details..."),
					Qt::Key_Return, this, SLOT( slotAdvanced() ) );*/

	pbRemove->setIcon( QIcon::fromTheme(QStringLiteral("list-remove")) );
	pbClear->setIcon( QIcon::fromTheme(QStringLiteral("edit-clear")) );
	//pbAdvanced->setIcon( QIcon::fromTheme(QStringLiteral("options")) );
	pbCheck->setIcon( QIcon::fromTheme(QStringLiteral("run-build")) );
	pbGo->setIcon( QIcon::fromTheme(QStringLiteral("run-build-install")) );
	pbWhatsThis->setIcon( QIcon::fromTheme(QStringLiteral("help-about")) );
}

/**
* What's this info explaning this tabs functionality.
*/
void QueueTab::slotWhatsThis()
{
	QWhatsThis::showText( QCursor::pos(), i18n( "<qt>"
			"The emerge queue quickly shows which packages listed for installation.<br/>"
			"Since many applications depend on each other, any attempt to install a certain software package might result in the installation "
			"of several dependencies as well. Don't worry, Portage handles dependencies well.<br/><br/>"
			"If you want to find out what Portage would install when you ask it to install a certain package, press 'Check Installation'.<br/>"
			"When all dependencies are press 'Start Installation'.</qt>" ), this );
}

/**
* Forward signal from next-buttons.
* @param isNext
*/
void QueueTab::slotNextPackage( bool isNext )
{
	if ( !m_packageInspector->isParentView( VIEW_QUEUE ) )
		return;

	queueView->nextPackage( isNext );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Queue view slots
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
* Reload queue when package view is changed, fex when package is removed.
*/
void QueueTab::slotRefresh()
{
	DEBUG_LINE_INFO;
	if ( !QueueSingleton::Instance()->isQueueBusy() )
	{
		slotRemoveInstalled();
		queueView->insertPackageList( m_hasCheckedQueue );
	}

	slotBusy();
}

/**
* Load Queue packages.
*/
void QueueTab::slotReload( bool hasCheckedQueue )
{
	DEBUG_LINE_INFO;
	// Reenable the inspector after queue changes
	//m_packageInspector->setDisabled( true );
	pbAdvanced->setDisabled( true );

	// If user is not su emerge pretend will not set packages as checked
	m_hasCheckedQueue = hasCheckedQueue;
	if ( m_hasCheckedQueue && !KUser().isSuperUser() )
		m_hasCheckedQueue = false;

	// Load all packages
	slotRefresh();
	if (hasCheckedQueue) {
		queueView->expandToDepth(2);
	}

	// Enable the gui
	slotBusy();

	m_initialQueueTime = formatTime( queueView->totalDuration() );
	KurooStatusBar::instance()->clearElapsedTime();
	slotQueueSummary();
}

/**
* View current queue summary.
*/
void QueueTab::slotQueueSummary()
{
	queueBrowser->clear();
	QString queueBrowserLines(   i18n( "<table width=100% border=0 cellpadding=0><tr><td colspan=2><b>Summary</b></td></tr>" ) );
			queueBrowserLines += i18n( "<tr><td width=10%>Number&nbsp;of&nbsp;packages:</td><td> %1</td></tr>", queueView->count() );
			queueBrowserLines += i18n( "<tr><td width=10%>Initial&nbsp;estimated&nbsp;time:</td><td> %1</td></tr>", m_initialQueueTime );
			queueBrowserLines += i18n( "<tr><td width=10%>Elapsed&nbsp;time:</td><td> %1</td></tr>", formatTime( KurooStatusBar::instance()->elapsedTime() ) );
			queueBrowserLines += i18n( "<tr><td width=10%>Estimated&nbsp;time&nbsp;remaining:</td><td> %1</td></tr></table>", formatTime( queueView->totalDuration() ) );
	queueBrowser->setText( queueBrowserLines );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Toggle button slots
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
* Disable/enable buttons when kuroo busy signal is received.
*/
void QueueTab::slotBusy()
{
	// No db or queue is empty - no fun!
	if ( !SignalistSingleton::Instance()->isKurooReady() || KurooDBSingleton::Instance()->isQueueEmpty() ) {
		pbRemove->setDisabled( true );
		pbAdvanced->setDisabled( true );
		pbClear->setDisabled( true );
		cbDownload->setDisabled( true );
		cbForceConf->setDisabled( true );
		cbNoWorld->setDisabled( true );
		pbCheck->setDisabled( true );
		pbGo->setDisabled( true );
		cbSkipHousekeeping->setDisabled( true );
	}
	else
		slotButtons();
}

/**
* Disable buttons if no package is selected or kuroo is busy emerging.
*/
void QueueTab::slotButtons()
{
	if ( m_packageInspector->isVisible() )
		return;

	// Kuroo is busy emerging toggle to "abort"
	if ( EmergeSingleton::Instance()->isRunning() ) {
		pbGo->setText( i18n( "Abort Installation" ) );
		disconnect(pbGo, &QPushButton::clicked, this, &QueueTab::slotGo);
		disconnect(pbGo, &QPushButton::clicked, this, &QueueTab::slotStop);
		connect(pbGo, &QPushButton::clicked, this, &QueueTab::slotStop);
	}
	else {
		pbGo->setText( i18n( "Step &2: Start Installation" ) );
		disconnect(pbGo, &QPushButton::clicked, this, &QueueTab::slotGo);
		disconnect(pbGo, &QPushButton::clicked, this, &QueueTab::slotStop);
		connect(pbGo, &QPushButton::clicked, this, &QueueTab::slotGo);
		if ( KurooConfig::enableEclean() || KurooConfig::revdepEnabled() )
			cbSkipHousekeeping->setDisabled( false );
		else
			cbSkipHousekeeping->setDisabled( true );
	}

	// No package selected, disable all buttons
	if ( queueView->selectedPackagesByIds().isEmpty() ) {
		pbRemove->setDisabled( true );
		pbAdvanced->setDisabled( true );
	}

	if (KurooDBSingleton::Instance()->isQueueEmpty())
		return;

	// Queue is not empty - enable button "Remove all" and "Check Installation"
	cbDownload->setDisabled( false );

	// When emerging packages do not allow user to change the queue
	if ( EmergeSingleton::Instance()->isRunning() || SignalistSingleton::Instance()->isKurooBusy() ) {
		pbRemove->setDisabled( true );
		pbClear->setDisabled( true );
		pbCheck->setDisabled( true );
		cbSkipHousekeeping->setDisabled( true );
	}
	else {
		pbRemove->setDisabled( false );
		pbClear->setDisabled( false );
		pbCheck->setDisabled( false );
		if ( KurooConfig::enableEclean() || KurooConfig::revdepEnabled() )
			cbSkipHousekeeping->setDisabled( false );
		else
			cbSkipHousekeeping->setDisabled( true );
	}


	// User is su and packages in queue are "checked" - enable checkboxes
	if ( m_hasCheckedQueue && KUser().isSuperUser() ) {
		pbGo->setDisabled( false );
		cbForceConf->setDisabled( false );
		cbNoWorld->setDisabled( false );
		cbRemove->setDisabled( false );
	}
	else {
		pbGo->setDisabled( true );
		cbForceConf->setDisabled( true );
		cbNoWorld->setDisabled( true );
		cbRemove->setDisabled( true );
	}

	m_packageInspector->setDisabled( false );
	pbAdvanced->setDisabled( false );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Package slots
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
* Emerge all packages in the installation queue.
*/
void QueueTab::slotCheck()
{
	// Only user-end packages not the dependencies
	QStringList packageList = queueView->allPackagesNoChildren();
	qDebug() << packageList;
	if( cbUpdate->isChecked() )
		packageList.prepend( "-uN" );
	EmergeSingleton::Instance()->pretend( packageList );
}

/**
* Emerge all packages in the installation queue.
*/
void QueueTab::slotGo()
{
	// If emerge is running I'm the abort function
	if ( EmergeSingleton::Instance()->isRunning() )
		slotStop();

	// Only user-end packages not the dependencies
	QStringList packageList = queueView->allPackagesNoChildren();

	if ( cbSkipHousekeeping->isChecked() )
		EmergeSingleton::Instance()->setSkipHousekeeping( true );
	else
		EmergeSingleton::Instance()->setSkipHousekeeping( false );

	// Only download? prepend --fetch-all-uri
	// Else, let's install the user-end packages
	if ( cbDownload->isChecked() ) {
		switch( KMessageBox::questionYesNoList( this,
			i18n("Do you want to Download following packages?"), packageList, i18n("Installation queue"),
			KStandardGuiItem::yes(), KStandardGuiItem::no(), "dontAskAgainDownload", KMessageBox::Dangerous ) ) {

				case KMessageBox::Yes:
					packageList.prepend( "--fetch-all-uri" );
					QueueSingleton::Instance()->installQueue( packageList );
					KurooStatusBar::instance()->setTotalSteps( queueView->totalDuration() );
				default:
					break;
			}
	}
	else {
		switch( KMessageBox::questionYesNoList( this,
			i18n("Do you want to install following packages?"), packageList, i18n("Installation queue"),
			KStandardGuiItem::yes(), KStandardGuiItem::no(), "dontAskAgainInstall", KMessageBox::Dangerous ) ) {

				case KMessageBox::Yes: {
					if( cbUpdate->isChecked() )
						packageList.prepend( "-uN" );

					// Force portage to reinstall files protected in CONFIG_PROTECT
					if ( cbForceConf->isChecked() )
						packageList.prepend( "--noconfmem" );

					// Emerge as normal, but do not add the packages to the @world profile for later updating.
					if ( cbNoWorld->isChecked() )
						packageList.prepend( "--oneshot" );

					QueueSingleton::Instance()->installQueue( packageList );
					KurooStatusBar::instance()->setTotalSteps( queueView->totalDuration() );
				}
				default:
					break;
			}
	}
}

/**
* Kill the running emerge process.
*/
void QueueTab::slotStop()
{
	switch ( KMessageBox::warningYesNo( this,
		i18n( "Do you want to abort the running installation?" ) ) ) {

			case KMessageBox::Yes :
				EmergeSingleton::Instance()->stop();
				KurooStatusBar::instance()->setProgressStatus( QString::null, i18n("Done.") );
				// added an explicit busy switch to allow someone to continue once
				// an abort has happened. 20070223 - aga
				SignalistSingleton::Instance()->setKurooBusy(false);
				// added a log entry for the abort
				LogSingleton::Instance()->writeLog( i18n("Emerge aborted by user."), KUROO );
				SignalistSingleton::Instance()->emergeAborted();
			default:
				break;
		}
}

/**
* Launch emerge pretend of packages in queue.
*/
void QueueTab::slotPretend()
{
	PortageSingleton::Instance()->pretendPackageList( queueView->allId() );
}

/**
* Remove package from Queue.
*/
void QueueTab::slotRemove()
{
	if ( isVisible() )
		m_packageInspector->hide();

	QueueSingleton::Instance()->removePackageIdList(queueView->selectedPackagesByIds());
}

/**
* Remove all packages from Queue.
*/
void QueueTab::slotClear()
{
	if ( isVisible() )
		m_packageInspector->hide();

	QueueSingleton::Instance()->reset();
}

/**
* Remove package from Queue.
*/
void QueueTab::slotRemoveInstalled()
{
	QueueSingleton::Instance()->setRemoveInstalled( cbRemove->isChecked() );
}

/**
* Open advanced dialog with: ebuild, versions, use flags...
*/
void QueueTab::slotAdvanced()
{
	DEBUG_LINE_INFO;
	pbRemove->setDisabled( true );
	pbClear->setDisabled( true );
	pbCheck->setDisabled( true );
	pbAdvanced->setDisabled( true );
	pbGo->setDisabled( true );

	if ( queueView->currentPackage() )
		processPackage( true );
}

void QueueTab::slotPackage()
{
	if (m_packageInspector->isVisible())
		processPackage( true );
	else
		processPackage( false );
}

void QueueTab::slotAddWorld()
{
	if ( isVisible() )
		m_packageInspector->hide();

	const QStringList selectedIdsList = queueView->selectedPackagesByIds();

	QStringList packageList;
	foreach ( QString id, selectedIdsList )
		packageList += queueView->packageItemById( id )->category() + "/" + queueView->packageItemById( id )->name();
	PortageSingleton::Instance()->appendWorld( packageList );
}

void QueueTab::slotRemoveWorld()
{
	if ( isVisible() )
		m_packageInspector->hide();

	const QStringList selectedIdsList = queueView->selectedPackagesByIds();

	QStringList packageList;
	foreach ( QString id, selectedIdsList )
		packageList += queueView->packageItemById( id )->category() + "/" + queueView->packageItemById( id )->name();
	PortageSingleton::Instance()->removeFromWorld( packageList );
}

/**
* Process package and view in Inspector.
*/
void QueueTab::processPackage( bool viewInspector )
{
	// Queue view is hidden don't update
	if ( m_packageInspector->isVisible() && !m_packageInspector->isParentView( VIEW_QUEUE ) )
		return;

	//TODO: This test wasn't needed before, it would be nice to get rid of it again
	if (queueView->currentPackage())
	{
		// Initialize the portage package object with package and it's versions data
		queueView->currentPackage()->parsePackageVersions();

		// Refresh inspector if visible
		if ( viewInspector )
			m_packageInspector->edit( queueView->currentPackage(), VIEW_QUEUE );
	}
}

/**
* Popup menu for current package.
* @param point
*/
void QueueTab::slotContextMenu(const QPoint &point)
{
	QModelIndex item = queueView->indexAt(point);

	if ( !item.isValid() )
		return;

	QMenu menu( this );

	menu.addAction( m_removeFromQueue );
	menu.addAction( m_packageDetails );

	// Allow editing of World when superuser
	PackageListItem* internalItem = static_cast<PackageListItem*>( item.internalPointer() );
	if ( !internalItem->isInWorld() ) {
		menu.addAction( m_addToWorld );
		m_addToWorld->setEnabled( KUser().isSuperUser() );
	} else {
		menu.addAction( m_removeFromWorld );
		m_removeFromWorld->setEnabled( KUser().isSuperUser() );
	}

	// No change to Queue when busy
	if ( EmergeSingleton::Instance()->isRunning() || SignalistSingleton::Instance()->isKurooBusy() )
		m_removeFromQueue->setDisabled(true);

	if ( m_packageInspector->isVisible() ) {
		m_removeFromQueue->setDisabled(true);
		m_packageDetails->setDisabled(true);
		m_addToWorld->setDisabled(true);
		m_removeFromWorld->setDisabled(true);
	}

	menu.exec(queueView->viewport()->mapToGlobal(point));
}

