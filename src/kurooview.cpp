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

#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>		//usleep()

#include <QPainter>
#include <QPixmap>
#include <QBitmap>
#include <QThread>

#include <KMessageBox>
#include <KUser>

#include "common.h"
#include "kurooview.h"
#include "portagetab.h"
#include "queuetab.h"
#include "queuelistview.h"
#include "logstab.h"
#include "historytab.h"
#include "mergetab.h"
#include "packageinspector.h"

/**
 * @class KurooView
 * @short Gui content with icon-menu and pages.
 *
 * Insert all 5 pages in a widgetStack, connects icon-menu buttons to corresponding pages (tabs).
 * Highlights icon-texts when changes are mades in the page.
 */
KurooView::KurooView( QWidget *parent ) :
	//DCOPObject( "kurooIface" ),
	KPageWidget( parent ),
	viewPortage( 0 ), viewHistory( 0 ), viewQueue( 0 ), viewLogs( 0 ), viewMerge( 0 ), packageInspector( 0 ),
	m_isHistoryRestored( false )
{
	setFaceType( List );

	// Create the package inspector //Matt: defined here to be able to hide it on page change
	packageInspector = new PackageInspector( this );

	// Add all pages
	viewPortage = new PortageTab( this, packageInspector );
	//qDebug() << "KurooView.constructor categoreisView.minWidth=" << viewPortage->categoriesView->minimumWidth()
	//		<< "actual width=" << viewPortage->categoriesView->width();
	KPageWidgetItem* pagePortage = addPage( viewPortage, i18n("Packages") );
	//qDebug() << "KurooView.constructor categoreisView.minWidth=" << viewPortage->categoriesView->minimumWidth()
	//		<< "actual width=" << viewPortage->categoriesView->width();
	pagePortage->setHeader( "" );
	pagePortage->setIcon( QIcon::fromTheme(QStringLiteral("kuroo")) );

	viewQueue = new QueueTab( this, packageInspector );
	KPageWidgetItem* pageQueue = addPage( viewQueue, i18n("Queue") );
	pageQueue->setHeader( "" );
	pageQueue->setIcon( QIcon::fromTheme(QStringLiteral("kuroo_view_queue")) );

	viewHistory = new HistoryTab( /*this */);
	KPageWidgetItem* pageHistory = addPage( viewHistory, i18n("History") );
	pageHistory->setHeader( "" );
	pageHistory->setIcon( QIcon::fromTheme(QStringLiteral("kuroo_view_history")) );

	viewMerge = new MergeTab( this );
	KPageWidgetItem* pageMerge = addPage( viewMerge, i18n("Configuration") );
	pageMerge->setHeader( "" );
	pageMerge->setIcon( QIcon::fromTheme(QStringLiteral("kuroo_view_configuration")) );

	viewLogs = new LogsTab( this );
	KPageWidgetItem* pageLogs = addPage( viewLogs, i18n("Log") );
	pageLogs->setHeader( "" );
	pageLogs->setIcon( QIcon::fromTheme(QStringLiteral("kuroo_view_log")) );

	// Connect menu-icons to the pages
	connect(this, &KurooView::currentPageChanged, this, &KurooView::slotShowView);
	setCurrentPage( pagePortage );
	//qDebug() << "categoreisView.minWidth=" << viewPortage->categoriesView->minimumWidth()
	//		<< "actual width=" << viewPortage->categoriesView->width();

	// Give log access to logBrowser and checkboxes
	// Check emerge.log for new entries. (In case of cli activities outside kuroo)
	LogSingleton::Instance()->setGui( viewLogs->logBrowser, viewLogs->verboseLog, viewLogs->saveLog );

	// Confirm changes in views with bleue text menu
	//connect( this, SIGNAL( currentChanged( KPageWidgetItem* ) ), this, SLOT( slotResetMenu( KPageWidgetItem* ) ) );
}
KurooView::~KurooView() {}

/**
 * Activate corresponding page when clicking on icon in menu.
 */
void KurooView::slotShowView()
{
	if ( packageInspector->isVisible() )
		packageInspector->hide();

	/*int tabIndex = viewMenu->currentItem()-> + 1;
	viewStack->raiseWidget( tabIndex );*/
}

/**
 * Check if database needs to refreshed.
 */
void KurooView::slotInit()
{
	DEBUG_LINE_INFO;
	//qDebug() << "categoriesView.minWidth=" << viewPortage->categoriesView->minimumWidth() <<
	//			"actual width=" << viewPortage->categoriesView->width();
	connect( HistorySingleton::Instance(), SIGNAL( signalScanHistoryCompleted() ), this, SLOT( slotCheckPortage() ) );

	::sleep( 10 );	//allow db to load
	// Check if history is empty, then maybe this is also a fresh install with empty db
	if ( KurooDBSingleton::Instance()->isHistoryEmpty() ) {
		m_isHistoryRestored = true;

		KMessageBox::information( this, i18n( "<qt>Kuroo database is empty!<br/>"
											  "Kuroo will now first scan your emerge log to create the emerge history.<br/>"
											  "Next, package information in Portage will be collected.</qt>"),
										i18n( "Initialiazing Kuroo") );
		HistorySingleton::Instance()->slotRefresh();
	}
	else {

		// Load packages if db is not empty
		if ( !KurooDBSingleton::Instance()->isPortageEmpty() ) {
			viewPortage->slotReload();
			viewQueue->slotReload( false );
		}

		// Check if kuroo db needs updating
		if ( !HistorySingleton::Instance()->slotRefresh() ) {
			disconnect( HistorySingleton::Instance(), SIGNAL( signalScanHistoryCompleted() ), this, SLOT( slotCheckPortage() ) );

			assert(QThread::currentThread() == qApp->thread());
			switch( KMessageBox::warningYesNo( this,
				i18n("<qt>Kuroo database needs refreshing!<br/>Emerge log shows that your system has changed.</qt>"),
				i18n("Initialiazing Kuroo"), KGuiItem( i18n("Refresh") ), KGuiItem( i18n("Skip") ) ) ) {
			case KMessageBox::Yes:
				PortageSingleton::Instance()->slotRefresh();
				break;
			default:
				KurooDBSingleton::Instance()->setKurooDbMeta( "scanTimeStamp", QString::number( QDateTime::currentDateTime().toTime_t() ) );
				slotCheckPortage();
			}
		}
	}
}

/**
 * When starting kuroo, check if portage need to be rescanned.
 * Updates must be scanned afterwards.
 */
void KurooView::slotCheckPortage()
{
	DEBUG_LINE_INFO;
	disconnect( HistorySingleton::Instance(), SIGNAL( signalScanHistoryCompleted() ), this, SLOT( slotCheckPortage() ) );

	// Restore backup after db is recreated because of new version
	if ( m_isHistoryRestored ) {
		HistorySingleton::Instance()->updateStatistics();
		m_isHistoryRestored = false;
	}

	DEBUG_LINE_INFO;

	if ( KurooDBSingleton::Instance()->isPortageEmpty() )
		PortageSingleton::Instance()->slotRefresh();
	else {

		// Ready to roll
		SignalistSingleton::Instance()->setKurooReady( true );
	}
	DEBUG_LINE_INFO;

	// Warn user that emerge need root permissions - many rmb actions are disabled
	if ( !KUser().isSuperUser() )
		KMessageBox::information( this, i18n("You must run Kuroo as root to emerge packages!"), i18n("Information"), "dontAskAgainNotRoot" );
}

/**
 * Dcop interface to emerge pretend process
 * @param packageList	list of packages to emerge
 */
void KurooView::slotEmergePretend( QString package )
{
	EmergeSingleton::Instance()->pretend( QStringList( package ) );
}

/**
 * Clear the highlighting menu text back to normal when visits the view.
 */
/*void KurooView::slotResetMenu( KPageWidgetItem* menuItem )
{
	dynamic_cast<IconListItem*>( menuItem )->setChanged( false );
	//viewMenu->triggerUpdate( true );
}*/


///////////////////////////////////////////////////////////////////////////////////////////////////
// Create menu icons and highlight menutext when changes.
///////////////////////////////////////////////////////////////////////////////////////////////////
/*Using standard KPageDialog now
void KurooView::IconListItem::paint( QPainter *painter )
{
	if ( isSelected() ) {
		//painter->setPen( listWidget()->colorGroup().highlightedText() );
		m_modified = false;
	}
	else {
		if ( m_modified ) {
			//painter->setPen( listWidget()->colorGroup().link() );
		} else {
			//painter->setPen( listWidget()->colorGroup().text() );
		}
	}

	QFontMetrics fm = painter->fontMetrics();
	int ht = fm.boundingRect( 0, 0, 0, 0, Qt::AlignCenter, text() ).height();
	int wp = mPixmap.width();
	int hp = mPixmap.height();

	//painter->drawPixmap( ( listWidget()->maxItemWidth()-wp ) / 2, 5, mPixmap );

	if ( !text().isEmpty() ) {
		//painter->drawText( 0, hp + 7, listWidget()->maxItemWidth(), ht, Qt::AlignCenter, text() );
	}
}

int KurooView::IconListItem::height( const QListWidget *lb ) const
{
	if ( text().isEmpty() )
		return mPixmap.height();
	else {
		int ht = lb->fontMetrics().boundingRect( 0, 0, 0, 0, Qt::AlignCenter, text() ).height();
		return ( mPixmap.height() + ht + 10 );
	}
}

int KurooView::IconListItem::width( const QListWidget *lb ) const
{
	int wt = lb->fontMetrics().boundingRect( 0, 0, 0, 0, Qt::AlignCenter, text() ).width() + 10;
	int wp = mPixmap.width() + 10;
	int w  = qMax( wt, wp );
	return qMax( w, mMinimumWidth );
}

const QPixmap &KurooView::IconListItem::defaultPixmap()
{
	static QPixmap *pix = 0;
	if ( !pix ) {
		pix = new QPixmap( 32, 32 );
		QPainter p( pix );
		p.eraseRect( 0, 0, pix->width(), pix->height() );
		p.setPen( Qt::red );
		p.drawRect ( 0, 0, pix->width(), pix->height() );
		p.end();

		QBitmap mask( pix->width(), pix->height() );
		mask.fill( Qt::black );
		p.begin( &mask );
		p.setPen( Qt::white );
		p.drawRect ( 0, 0, pix->width(), pix->height() );
		p.end();

		pix->setMask( mask );
	}

	return *pix;
}

void KurooView::IconListItem::setChanged( bool modified )
{
	m_modified = modified;
}

bool KurooView::IconListItem::isChanged()
{
	return m_modified;
}
*/

