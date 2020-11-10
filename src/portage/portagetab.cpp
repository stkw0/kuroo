/****************************************************************************
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
 ****************************************************************************/

#include <unistd.h>

#include "common.h"
#include "search.h"
#include "categorieslistview.h"
#include "portagetab.h"
#include "packageinspector.h"
#include "packageversion.h"
#include "versionview.h"
#include "uninstallinspector.h"
#include "ui_portagebase.h"
#include "packagelistmodel.h"
#include "packagelistitem.h"

#include <QAction>
#include <QButtonGroup>
#include <QGroupBox>
#include <QMenu>
#include <QTimer>
#include <QWhatsThis>

/*#include <QPushButton>
#include <QTextBrowser>
#include <QLineEdit>
#include <kiconloader.h>*/
//#include <kaccel.h>
#include <KButtonGroup>
#include <KMessageBox>
#include <KUser>

enum Focus {
		CATEGORYLIST,
		SUBCATEGORYLIST,
		PACKAGELIST
};

/**
 * @class PortageTab
 * @short Package view with filters.
 */
PortageTab::PortageTab( QWidget* parent, PackageInspector *packageInspector )
	: QWidget( parent ), m_focusWidget( PACKAGELIST ), m_delayFilters( 0 ),
	m_packageInspector( packageInspector ), m_uninstallInspector( 0 )
{
	setupUi( this );

	filterGroup->setSelected(0);
	//kdDebug() << "PortageTab.constructor categoryView minimumWidth=" << categoriesView->minimumWidth()
	//		<< "actual width=" << categoriesView->width();
	// Connect What's this button
	//connect( pbWhatsThis, SIGNAL( clicked() ), parent->parent(), SLOT( whatsThis() ) ); //this one appears to be stale
	connect(pbWhatsThis, &QPushButton::clicked, this, &PortageTab::slotWhatsThis);

	// Connect the filters
	connect(filterGroup, &KButtonGroup::released, this, &PortageTab::slotFilters);
	connect(searchFilter, &QLineEdit::textChanged, this, &PortageTab::slotFilters);

	// Rmb actions.
	packagesView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(packagesView, &PortageListView::customContextMenuRequested, this, &PortageTab::slotContextMenu);

	m_addToQueue = new QAction( QIcon::fromTheme(QStringLiteral("kuroo_queue")), i18n( "&Add to Queue"), this );
	connect(m_addToQueue, &QAction::triggered, this, &PortageTab::slotEnqueue);
	m_removeFromQueue = new QAction( QIcon::fromTheme(QStringLiteral("kuroo_queue")), i18n( "&Remove from Queue"), this );
	connect(m_removeFromQueue, &QAction::triggered, this, &PortageTab::slotDequeue);
	m_addToWorld = new QAction( QIcon::fromTheme(QStringLiteral("kuroo_world")), i18n( "Add To &World"), this );
	connect(m_addToWorld, &QAction::triggered, this, &PortageTab::slotAddWorld);
	m_removeFromWorld = new QAction( QIcon::fromTheme(QStringLiteral("kuroo_world")), i18n( "Remove From &World"), this );
	connect(m_removeFromWorld, &QAction::triggered, this, &PortageTab::slotRemoveWorld);
	m_packageDetails = new QAction( i18n( "&Details"), this );
	connect(m_packageDetails, &QAction::triggered, this, &PortageTab::slotAdvanced);
	m_uninstallPackage = new QAction( QIcon::fromTheme(QStringLiteral("list-remove")), i18n( "&Uninstall" ), this );
	connect(m_uninstallPackage, &QAction::triggered, this, &PortageTab::slotUninstall);
	m_quickPackage = new QAction( QIcon::fromTheme(QStringLiteral("kuroo_quickpkg")), i18n( "Backup Package"), this );
	connect(m_quickPackage, &QAction::triggered, this, &PortageTab::slotBackup);

	// Button actions.
	//connect( pbQueue, SIGNAL( clicked() ), this, SLOT( slotEnqueue() ) );
	connect(pbUninstall, &QPushButton::clicked, this, &PortageTab::slotUninstall);
	connect(packagesView, &PortageListView::doubleClickedSignal, this, &PortageTab::slotAdvanced);
	connect(pbAdvanced, &QPushButton::clicked, this, &PortageTab::slotAdvanced);
	connect(pbClearFilter, &QPushButton::clicked, this, &PortageTab::slotClearFilter);

	// Toggle Queue button between "add/remove" when after queue has been edited
	//connect( QueueSingleton::Instance(), SIGNAL( signalQueueChanged( bool ) ), this, SLOT( slotButtons() ) );
	connect( SignalistSingleton::Instance(), SIGNAL( signalPackageQueueChanged() ), this, SLOT( slotButtons() ) );

	// Reload view after changes.
	connect( PortageSingleton::Instance(), SIGNAL( signalPortageChanged() ), this, SLOT( slotReload() ) );

	// Enable/disable this view and buttons when kuroo is busy
	connect( SignalistSingleton::Instance(), SIGNAL( signalKurooBusy( bool ) ), this, SLOT( slotBusy() ) );

	// Load Inspector with current package info
	connect(packagesView, &PortageListView::selectionChangedSignal, this, &PortageTab::slotPackage);

	// Enable/disable buttons
	connect(packagesView, &PortageListView::selectionChangedSignal, this, &PortageTab::slotButtons);

	// Connect changes made in Inspector to this view so it gets updated
	connect(m_packageInspector, &PackageInspector::signalPackageChanged, this, &PortageTab::slotPackage);
	connect(m_packageInspector, &PackageInspector::signalNextPackage, this, &PortageTab::slotNextPackage);
	connect(m_packageInspector, &PackageInspector::finished, this, &PortageTab::slotButtons);

	// Shortcut to enter filter with package name
	connect( SignalistSingleton::Instance(), SIGNAL( signalPackageClicked( const QString& ) ), this, SLOT( slotFillFilter( const QString& ) ) );

	slotInit();
}

PortageTab::~PortageTab()
{}

/**
 * Initialize Portage view.
 */
void PortageTab::slotInit()
{
	// Get color theme
	QPalette p;
	p.setColor(portageFrame->backgroundRole(), palette().base().color());
	portageFrame->setPalette(p);

	// Change select-color in summaryBrowser to get contrast
	//QColorGroup colorGroup = QColorGroup(palette());
	QPalette summaryPalette;
	//QColorGroup summaryColorGroup( colorGroup );
	summaryPalette.setColor( QPalette::HighlightedText, palette().dark().color() );

	/*summaryPalette.setActive( summaryColorGroup );
	summaryPalette.setInactive( summaryColorGroup );
	summaryPalette.setDisabled( summaryColorGroup );*/
	summaryPalette.setColor(QPalette::Active, QPalette::NoRole, palette().color(QPalette::Active, QPalette::NoRole));
	summaryPalette.setColor(QPalette::Inactive, QPalette::NoRole, palette().color(QPalette::Inactive, QPalette::NoRole));
	summaryPalette.setColor(QPalette::Disabled, QPalette::NoRole, palette().color(QPalette::Disabled, QPalette::NoRole));

	summaryBrowser->setPalette( summaryPalette );

	// Keyboard shortcuts
	/*KAccel* pAccel = new KAccel( this );
	pAccel->insert( "View package details...", i18n("View package details..."), i18n("View package details..."),
					Qt::Key_Return, this, SLOT( slotAdvanced() ) );*/

	// Initialize the uninstall dialog
	m_uninstallInspector = new UninstallInspector( this );

	pbClearFilter->setIcon( QIcon::fromTheme(QStringLiteral("edit-clear-locationbar-ltr")) );
	pbQueue->setIcon( QIcon::fromTheme(QStringLiteral("kuroo_queue")) );
	pbUninstall->setIcon( QIcon::fromTheme(QStringLiteral("list-remove")) );
	//TODO: There is no icon in the pics folder for this, and I can't find any stock icon named options, so
	//I'm turning it off for now.
	//pbAdvanced->setIcon( QIcon::fromTheme(QStringLiteral("options")) );
	pbWhatsThis->setIcon( QIcon::fromTheme(QStringLiteral("help-about")) );

	slotBusy();
}

/**
 * What's this info explaning this tabs functionality.
 */
void PortageTab::slotWhatsThis()
{
	QWhatsThis::showText( QCursor::pos(), i18n( "<qt>"
			"This tab gives an overview of all packages available: in Portage, installed packages as well as package updates.<br/>"
			"To keep your system in perfect shape (and not to mention install the latest security updates) you need to update your system regularly. "
			"Since Portage only checks the ebuilds in your Portage tree you first have to sync your Portage tree: "
			"Select 'Sync Portage' in the Portage menu.<br/>"
			"After syncing Kuroo will search for newer version of the applications you have installed. "
			"However, it will only verify the versions for the applications you have explicitly installed - not the dependencies.<br/>"
			"If you want to update every single package on your system, check the Deep checkbox in Kuroo Preferences.<br/><br/>"
			"When you want to remove a software package from your system, select a package and press 'Uninstall'. "
			"This will tell Portage to remove all files installed by that package from your system except the configuration files "
			"of that application if you have altered those after the installation. "
			"However, a big warning applies: Portage will not check if the package you want to remove is required by another package. "
			"It will however warn you when you want to remove an important package that breaks your system if you unmerge it.<br/><br/>"
			"Use the package Inspector to manage package specific version and use-flag settings: press 'Details' to open the Inspector.</qt>" ), this );
}

/**
 * Forward signal from next-buttons only if this tab is visible for user.
 * @param isNext
 */
void PortageTab::slotNextPackage( bool isNext )
{
	if ( !m_packageInspector->isParentView( VIEW_PORTAGE ) )
		return;

	packagesView->nextPackage( isNext );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Toggle button slots
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Reset queue button text when queue is refreshed.
 */
void PortageTab::slotInitButtons()
{
	pbQueue->setText( i18n("Add to Queue") );
}

/**
 * Disable/enable buttons when kuroo is busy.
 */
void PortageTab::slotBusy()
{
	// If no db no fun!
	if ( !SignalistSingleton::Instance()->isKurooReady() ) {
		pbUninstall->setDisabled( true );
		pbAdvanced->setDisabled( true );
		pbQueue->setDisabled( true );
		filterGroup->setDisabled( true );
		searchFilter->setDisabled( true );
		pbClearFilter->setDisabled( true );
	}
	else {
		filterGroup->setDisabled( false );
		searchFilter->setDisabled( false );
		pbClearFilter->setDisabled( false );
		slotButtons();
	}
}

/**
 * Toggle buttons states.
 */
void PortageTab::slotButtons()
{
	if ( m_packageInspector->isVisible() )
		return;
	else {
		filterGroup->setDisabled( false );
		searchFilter->setDisabled( false );
		pbClearFilter->setDisabled( false );
	}

	// No current package, disable all buttons
	if ( packagesView->selectedPackages().isEmpty()) {
		pbQueue->setDisabled( true );
		pbAdvanced->setDisabled( true );
		pbUninstall->setDisabled( true );
		return;
	}

	m_packageInspector->setDisabled( false );
	pbAdvanced->setDisabled( false );

	// When kuroo is busy disable queue and uninstall button
	if ( SignalistSingleton::Instance()->isKurooBusy() ) {
		pbQueue->setDisabled( true );
		pbUninstall->setDisabled( true );
		return;
	}
	else
		pbQueue->setDisabled( false );

	// Toggle queue button between add/remove
	if ( packagesView->currentPackage()) {
		pbQueue->setDisabled( false );

		if ( QueueSingleton::Instance()->isQueued(packagesView->currentPackage()->id()))
		{
			disconnect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotEnqueue);
			disconnect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotDequeue);
			connect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotDequeue);
			pbQueue->setText( i18n("Remove from Queue") );
		}
		else
		{
			disconnect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotEnqueue);
			disconnect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotDequeue);
			connect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotEnqueue);
			pbQueue->setText( i18n("Add to Queue") );
		}
		if (packagesView->currentPackage()->isInstalled() && ::getuid() == 0)
			pbUninstall->setDisabled( false );
		else
			pbUninstall->setDisabled( true );
	}
	else if (packagesView->selectedPackages().count() > 1) {
		pbQueue->setDisabled( false );
		int queued = 0, installed = 0;
		foreach(PackageListItem *item, packagesView->selectedPackages())
		{
			if (QueueSingleton::Instance()->isQueued(item->id()))
				queued++;
			if (item->isInstalled() && ::getuid() == 0)
				installed++;
		}

		// If all in queue, button is dequeue
		if (packagesView->selectedPackages().count() == queued)
		{
			disconnect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotEnqueue);
			disconnect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotDequeue);
			connect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotDequeue);
			pbQueue->setText( i18n("Remove from Queue") );
		}
		else // If none or some in queue, button is queue.
		{
			disconnect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotEnqueue);
			disconnect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotDequeue);
			connect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotEnqueue);
			pbQueue->setText( i18n("Add to Queue") );
		}

		// If all or some installed, button uninstall is active
		if (installed > 0)
			pbUninstall->setDisabled( false );
		else
			pbUninstall->setDisabled( true );
	}
	else {
		disconnect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotEnqueue);
		disconnect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotDequeue);
		connect(pbQueue, &QPushButton::clicked, this, &PortageTab::slotEnqueue);
		pbQueue->setText( i18n("Add to Queue") );
		pbQueue->setDisabled( true );
		pbUninstall->setDisabled( true );
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Package view slots
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Initialize category and subcategory views with the available categories and subcategories.
 */
void PortageTab::slotReload()
{
 	m_packageInspector->setDisabled( true );
	pbAdvanced->setDisabled( true );

	disconnect(categoriesView, &CategoriesListView::currentItemChanged, this, &PortageTab::slotListSubCategories);
	disconnect(subcategoriesView, &SubCategoriesListView::currentItemChanged, this, &PortageTab::slotListPackages);

	//kdDebug() << "PortageTab.slotReload categoriesView.minWidth=" << categoriesView->minimumWidth()
	//		<< "actual width" << categoriesView->width();
	categoriesView->init();
	subcategoriesView->init();

	connect(categoriesView, &CategoriesListView::currentItemChanged, this, &PortageTab::slotListSubCategories);
	connect(subcategoriesView, &SubCategoriesListView::currentItemChanged, this, &PortageTab::slotListPackages);

	categoriesView->loadCategories( KurooDBSingleton::Instance()->portageCategories( 1 << filterGroup->selected(), searchFilter->text() )/*, false */);
}

void PortageTab::slotFillFilter( const QString& text )
{
	searchFilter->setText( text );
}

/**
 * Execute query based on filter and text. Add a delay of 250ms.
 */
void PortageTab::slotFilters()
{
	m_delayFilters++;
	QTimer::singleShot( 250, this, SLOT( slotActivateFilters() ) );
}

/**
 * Execute query based on filter and text.
 */
void PortageTab::slotActivateFilters()
{
	--m_delayFilters;
	if ( m_delayFilters == 0 )
		categoriesView->loadCategories( KurooDBSingleton::Instance()->portageCategories( 1 << filterGroup->selected(), searchFilter->text() )/*, true */);
}

/**
 * List packages when clicking on category in installed.
 */
void PortageTab::slotListSubCategories()
{
	subcategoriesView->loadCategories( KurooDBSingleton::Instance()->portageSubCategories( categoriesView->currentCategoryId(),
		1 << filterGroup->selected(), searchFilter->text() ) );
}

/**
 * List packages when clicking on subcategory.
 */
void PortageTab::slotListPackages()
{
	int numberOfTerms = 0; //For singular or plural message
	// Disable all buttons if query result is empty
	QStringList dbres = KurooDBSingleton::Instance()->portagePackagesBySubCategory(categoriesView->currentCategoryId(),
										       subcategoriesView->currentCategoryId(),
										       1 << filterGroup->selected(),
										       searchFilter->text());

	packagesView->setPackages(dbres);
	if ( dbres.count() == 0) {
		m_packageInspector->hide();
		slotButtons();
		summaryBrowser->clear();
		if (radioUpdates->isChecked())
			numberOfTerms = -1;
		else {
			/*** We check number of terms to pass to showNoHitsWarning **/
			QChar space((char)32);
			bool iscounted=false;
			for (int i = 0; i < searchFilter->text().length(); i++) {
				if (searchFilter->text()[i] != space) {
					if (!iscounted) numberOfTerms++;
					iscounted = true;
				}
				else {
					iscounted = false;
				}
			}
		}
		qDebug() << numberOfTerms;
		//FIXME:implement this
		//packagesView->showNoHitsWarning( true , numberOfTerms );

		//TODO: This forces white instead of using the current color-scheme default
		// Highlight text filter background in red if query failed
		if ( !searchFilter->text().isEmpty() )
		{
			//searchFilter->setPaletteBackgroundColor( QColor( KurooConfig::noMatchColor() ) );
			QPalette p;
			p.setColor(searchFilter->backgroundRole(), QColor(KurooConfig::noMatchColor()));
			searchFilter->setPalette(p);

		}
		else
		{
			//searchFilter->setPaletteBackgroundColor( Qt::white );
			QPalette p;
			p.setColor(searchFilter->backgroundRole(), Qt::white);
			searchFilter->setPalette(p);
		}
	}
	else {
		//FIXME:implement this
		//packagesView->showNoHitsWarning( false , numberOfTerms );

		// Highlight text filter background in green if query successful
		if ( !searchFilter->text().isEmpty() )
		{
			//searchFilter->setPaletteBackgroundColor( QColor( KurooConfig::matchColor() ) );
			QPalette p;
			p.setColor(searchFilter->backgroundRole(), QColor(KurooConfig::matchColor()));
			searchFilter->setPalette(p);
		}
		else
		{
			//searchFilter->setPaletteBackgroundColor( Qt::white );
			QPalette p;
			p.setColor(searchFilter->backgroundRole(), Qt::white);
			searchFilter->setPalette(p);
		}
	}
}

/**
 * Reset text filter when clicking on clear button.
 */
void PortageTab::slotClearFilter()
{
	searchFilter->clear();
}

/**
 * Refresh packages list.
 */
void PortageTab::slotRefresh()
{
	switch( KMessageBox::questionYesNo( this,
		i18n( "<qt>Do you want to refresh the Packages view?<br>"
			  "This will take a couple of minutes...</qt>"), i18n( "Refreshing Packages" ),
										KStandardGuiItem::yes(), KStandardGuiItem::no(), "dontAskAgainRefreshPortage" ) ) {
		case KMessageBox::Yes:
			PortageSingleton::Instance()->slotRefresh();
		default:
			break;
	}
}

/**
 * Append or remove package to the queue.
 */
void PortageTab::slotEnqueue()
{
	if ( !EmergeSingleton::Instance()->isRunning() || !SignalistSingleton::Instance()->isKurooBusy() ) {
		const QStringList selectedIdsList = packagesView->selectedPackagesByIds();
		QStringList packageIdList;
		foreach( QString id, selectedIdsList )
		{
			if ( packagesView->packageItemById( id )->isInPortage() && !packagesView->packageItemById( id )->isQueued() )
			{
				packageIdList += id;
			}
		}
		QueueSingleton::Instance()->addPackageIdList( packageIdList );
	}

	slotButtons();
}

void PortageTab::slotDequeue()
{
	if ( !EmergeSingleton::Instance()->isRunning() || !SignalistSingleton::Instance()->isKurooBusy() ) {
		const QStringList selectedIdsList = packagesView->selectedPackagesByIds();
		QueueSingleton::Instance()->removePackageIdList( selectedIdsList );
	}

	slotButtons();
}

/**
 * Uninstall selected package.
 */
void PortageTab::slotUninstall()
{
	if ( !EmergeSingleton::Instance()->isRunning() || !SignalistSingleton::Instance()->isKurooBusy() || ::getuid() != 0 ) {
		const QStringList selectedIdsList = packagesView->selectedPackagesByIds();

		// Pick only installed packages
		QStringList packageList;
		foreach ( QString id, selectedIdsList ) {
			if ( packagesView->packageItemById( id )->isInstalled() ) {
				packageList += id;
				packageList += packagesView->packageItemById( id )->category() + "/" + packagesView->packageItemById( id )->name();
			}
		}

		m_uninstallInspector->view( packageList );
	}
}

void PortageTab::slotAddWorld()
{
	const QStringList selectedIdsList = packagesView->selectedPackagesByIds();
	QStringList packageList;
	foreach ( QString id, selectedIdsList )
		packageList += packagesView->packageItemById( id )->category() + "/" + packagesView->packageItemById( id )->name();
	PortageSingleton::Instance()->appendWorld( packageList );
}

void PortageTab::slotRemoveWorld()
{
	const QStringList selectedIdsList = packagesView->selectedPackagesByIds();
	QStringList packageList;
	foreach ( QString id, selectedIdsList )
		packageList += packagesView->packageItemById( id )->category() + "/" + packagesView->packageItemById( id )->name();
	PortageSingleton::Instance()->removeFromWorld( packageList );
}

void PortageTab::slotBackup()
{
	const QStringList selectedIdsList = packagesView->selectedPackagesByIds();
	QStringList packageList;
	foreach( QString id, selectedIdsList ) {
		if( packagesView->packageItemById( id )->isInstalled() && packagesView->packageItemById( id )->isInPortage() )
			packageList += packagesView->packageItemById( id )->category() + "/" + packagesView->packageItemById( id )->name();
	}
	EmergeSingleton::Instance()->quickpkg( packageList );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Package slots
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Open advanced dialog with: ebuild, versions, use flags...
 */
void PortageTab::slotAdvanced()
{
	DEBUG_LINE_INFO;
	pbUninstall->setDisabled( true );
	pbAdvanced->setDisabled( true );
	pbQueue->setDisabled( true );
	filterGroup->setDisabled( true );
	searchFilter->setDisabled( true );
	pbClearFilter->setDisabled( true );

	if ( packagesView->currentPackage() )
		processPackage( true );
}

void PortageTab::slotPackage()
{
	if ( m_packageInspector->isVisible() )
		processPackage( true );
	else
		processPackage( false );
}

/**
 * Process package and all it's versions.
 * Update summary and Inspector.
 */
void PortageTab::processPackage( bool viewInspector )
{
	if ( m_packageInspector->isVisible() && !m_packageInspector->isParentView( VIEW_PORTAGE ) )
		return;

	summaryBrowser->clear();

	// Multiple packages selected
	const QStringList selectedIdsList = packagesView->selectedPackagesByIds();
	int count = selectedIdsList.size();
	if ( count > 1 ) {

		// Build summary html-view
		QString lines = "<table width=100% border=0 cellpadding=0>";
		lines += "<tr><td colspan=2><font";
		//bgcolor=#" + GlobalSingleton::Instance()->bgHexColor() + "
		//lines += "color=#"GlobalSingleton::Instance()->fgHexColor()
		lines += " size=+1><b>";
		lines += QString::number( count ) + i18n( " packages selected" ) + "</b></font></td></tr>";
		lines += "<tr><td>";
		foreach ( QString id, selectedIdsList ) {
			lines += packagesView->packageItemById( id )->name();
			lines += " (" + packagesView->packageItemById( id )->category().section( "-", 0, 0 ) + "/";
			lines += packagesView->packageItemById( id )->category().section( "-", 1, 1 ) + "), ";
		}
		lines = lines.left( lines.length() - 2 );
		lines += "</td></tr>";
		summaryBrowser->setText( lines + "</table>");

		pbAdvanced->setDisabled( true );
		return;
	}

	// Initialize the portage package object with the current package and it's versions data
	PackageListItem *package = packagesView->currentPackage();
	if (!package) {
		return;
	}

	package->parsePackageVersions();
	QString linesInstalled = package->linesInstalled();
	QString linesAvailable = package->linesAvailable();
	QString linesEmerge = package->linesEmerge();

	// Build summary html-view
	QString lines = "<table width=100% border=0 cellpadding=0>";
			lines += "<tr><td";
			//"bgcolor=#" + GlobalSingleton::Instance()->bgHexColor();
			lines += " colspan=2><b>";
			//lines += "<font color=#" + GlobalSingleton::Instance()->fgHexColor() + ">"
			lines += "<font size=+1>" + package->name() + "</font> ";
			lines += "(" + package->category().section( "-", 0, 0 ) + "/";
			lines += package->category().section( "-", 1, 1 ) + ")</font></b></td></tr>";

	if ( package->isInPortage() ) {
		lines += "<tr><td colspan=2>" + package->description() + "</td></tr>";
		lines += "<tr><td colspan=2>" + i18n( "<b>Homepage: </b>" ) + "<a href=\"" + package->homepage();
		lines += "\">" + package->homepage() + "</a></td></tr>";
	}
	else {
		lines += "<tr><td colspan=2><font color=darkRed><b>"
				+ i18n( "Package not available in Portage tree anymore!" )
				+ "</b></font></td></tr>";
	}

	// Construct installed verions line
	if ( !linesInstalled.isEmpty() ) {
		linesInstalled = "<tr><td width=10%><b>"
						+ i18n( "Installed&nbsp;version:" )
						+ "</b></font></td><td width=90%>"
						+ linesInstalled
						+ "</td></tr>";
	}
	else {
		linesInstalled = "<tr><td width=10%><b>"
						+ i18n( "Installed&nbsp;version:" )
						+ "</b></font></td><td width=90%>"
						+ "Not installed"
						+ "</td></tr>";
	}

	if ( package->isInPortage() ) {

		// Construct emerge version line
		if ( !linesEmerge.isEmpty() ) {
			linesEmerge = "<tr><td width=10%><b>"
						+ i18n( "Emerge&nbsp;version:" )
						+ "</b></td><td width=90%>"
						+ linesEmerge
						+ "</td></tr>";
		}
		else {
			if ( package->isInArch() && linesAvailable.isEmpty() ) {
				linesEmerge = "<tr><td width=10%><b>"
							+ i18n( "Emerge&nbsp;version:" )
							+ "</font></td><td width=90%><font color=darkRed>"
							+ i18n( "No version available on" )
							+ KurooConfig::arch()
							+ "</b></td></tr>";
			}
			else {
				linesEmerge = "<tr><td width=10%><b>"
							+ i18n( "Emerge&nbsp;version:" )
							+ "</font></td><td width=90%><font color=darkRed>"
							+ i18n( "No version available - please check package details" )
							+ "</font></b></td></tr>";
			}
		}

		// Construct available versions line
		if ( !linesAvailable.isEmpty() ) {
			linesAvailable = "<tr><td width=10%><b>"
							+ i18n( "Available&nbsp;versions:" )
							+ "</b></td><td width=90%>"
							+ linesAvailable
							+ "</b></td></tr>";
		}
		else {
			linesAvailable = "<tr><td width=10%><b>"
						+ i18n( "Available&nbsp;versions:" )
						+ "</td><td width=90%><font color=darkRed>"
						+ i18n( "No versions available on %1", KurooConfig::arch() )
						+ "</font></b></td></tr>";
		}

		summaryBrowser->setText( lines + linesInstalled + linesEmerge + linesAvailable + "</table>" );
	}
	else {
		summaryBrowser->setText( lines + linesInstalled + "</table>" );
	}

	// Refresh inspector if visible
	if ( viewInspector ) {
		m_packageInspector->edit( package, VIEW_PORTAGE );
	}
}

/**
 * Popup menu for actions like emerge.
 * @param item
 * @param point
 */
void PortageTab::slotContextMenu(const QPoint &point)
{
	QModelIndex item = packagesView->indexAt(point);

	if ( !item.isValid() )
		return;

	const QStringList selectedIdsList = packagesView->selectedPackagesByIds();

	bool hasQueuedItems = false;
	bool hasUnqueuedItemsInPortage = false;
	bool hasItemsInWorld = false;
	bool hasItemsOutOfWorld = false;
	bool hasInstalledPackages = false;
	bool hasInstalledAndInPortagePackages = false;

	foreach( QString id, selectedIdsList ) {
		PackageListItem *currentItem = packagesView->packageItemById( id );

		if( currentItem->isQueued() ) {
			hasQueuedItems = true;
		} else {
			hasUnqueuedItemsInPortage |= currentItem->isInPortage();
		}

		hasItemsInWorld |= currentItem->isInWorld();
		hasItemsOutOfWorld |= !currentItem->isInWorld();

		if( currentItem->isInstalled() ) {
			hasInstalledPackages = true;
			hasInstalledAndInPortagePackages |= currentItem->isInPortage();
		}

		if( hasQueuedItems && hasUnqueuedItemsInPortage && hasItemsInWorld && hasItemsOutOfWorld
			&& hasInstalledPackages && hasInstalledAndInPortagePackages )
			break;	//stop the loop if we already have everything
	}

	QMenu menu( this );

	menu.addAction( m_addToQueue );
	m_addToQueue->setEnabled(hasUnqueuedItemsInPortage);
	menu.addAction( m_removeFromQueue );
	m_removeFromQueue->setEnabled(hasQueuedItems);
	menu.addAction(m_packageDetails);
	m_packageDetails->setEnabled( selectedIdsList.count() == 1 );
	menu.addAction(m_addToWorld);
	m_addToWorld->setEnabled(false);	//default to false, enable later if super user
	menu.addAction(m_removeFromWorld);
	m_removeFromWorld->setEnabled(false);	//default to false, enable later if super user
	menu.addAction(m_uninstallPackage);
	m_uninstallPackage->setEnabled(hasInstalledPackages);
	menu.addAction(m_quickPackage);
	m_quickPackage->setEnabled(hasInstalledAndInPortagePackages);
	/*
	int enqueueMenuItem;
	enqueueMenuItem = menu.addAction( ImagesSingleton::Instance()->icon( QUEUED ), i18n("&Add to queue"), ADDQUEUE );
	menu.setEnabled( enqueueMenuItem, hasUnqueuedItemsInPortage );

	int dequeueMenuItem;
	dequeueMenuItem = menu.insertItem( ImagesSingleton::Instance()->icon( QUEUED ), i18n("&Remove from queue"), DELQUEUE );
	menu.setItemEnabled( dequeueMenuItem, hasQueuedItems );

	int detailsMenuItem;
	detailsMenuItem = menu.insertItem( ImagesSingleton::Instance()->icon( DETAILS ), i18n( "Details..." ), DETAILS );
	menu.setItemEnabled( detailsMenuItem, selectedIdsList.count() == 1 );

	int addWorldMenuItem;
	addWorldMenuItem = menu.insertItem( ImagesSingleton::Instance()->icon( WORLD ), i18n( "Add to @world" ), ADDWORLD );
	menu.setItemEnabled( addWorldMenuItem, false );

	int delWorldMenuItem;
	delWorldMenuItem = menu.insertItem( ImagesSingleton::Instance()->icon( WORLD ), i18n( "Remove from @world" ), DELWORLD );
	menu.setItemEnabled( delWorldMenuItem, false );

	int uninstallMenuItem;
	uninstallMenuItem = menu.insertItem( ImagesSingleton::Instance()->icon( REMOVE ), i18n("&Uninstall"), UNINSTALL );
	menu.setItemEnabled( uninstallMenuItem, hasInstalledPackages );

	int backupMenuItem;
	backupMenuItem = menu.insertItem( ImagesSingleton::Instance()->icon( QUICKPKG ), i18n("Backup Package"), QUICKPACKAGE );
	menu.setItemEnabled( backupMenuItem, hasInstalledAndInPortagePackages );

	// No change to Queue when busy @todo: something nuts here. Click once then open rmb to make it work!
	qDebug() << "EmergeSingleton::Instance()->isRunning()=" << EmergeSingleton::Instance()->isRunning();
	qDebug() << "SignalistSingleton::Instance()->isKurooBusy()=" << SignalistSingleton::Instance()->isKurooBusy();
	qDebug() << "hasQueuedItems=" << hasQueuedItems;
	qDebug() << "hasUnqueuedItemsInPortage=" << hasUnqueuedItemsInPortage;
	qDebug() << "hasItemsInWorld=" << hasItemsInWorld;
	qDebug() << "hasItemsOutOfWorld=" << hasItemsOutOfWorld;
	qDebug() << "hasInstalledPackages=" << hasInstalledPackages;
	qDebug() << "hasInstalledAndInPortagePackages=" << hasInstalledAndInPortagePackages;
*/
	bool busy = EmergeSingleton::Instance()->isRunning() || SignalistSingleton::Instance()->isKurooBusy();
	if ( busy ) {
		//menu.setItemEnabled( enqueueMenuItem, false );
		//menu.setItemEnabled( dequeueMenuItem, false );
		//menu.setItemEnabled( backupMenuItem, false );
		m_addToQueue->setDisabled(true);
		m_removeFromQueue->setDisabled(true);
		m_quickPackage->setDisabled(true);
	}

	// No uninstall when emerging or no privileges
	if ( busy || !KUser().isSuperUser() ) {
		//menu.setItemEnabled( uninstallMenuItem, false );
		m_uninstallPackage->setDisabled(true);
	}

	// Allow editing of World when superuser
	if ( KUser().isSuperUser() ) {
		//menu.setItemEnabled( addWorldMenuItem, hasItemsOutOfWorld );
		//menu.setItemEnabled( delWorldMenuItem, hasItemsInWorld );
		m_addToWorld->setEnabled(hasItemsOutOfWorld);
		m_removeFromWorld->setEnabled(hasItemsInWorld);
	}

	if ( m_packageInspector->isVisible() ) {
// 		menu.setItemEnabled( enqueueMenuItem, false );
// 		menu.setItemEnabled( dequeueMenuItem, false );
// 		menu.setItemEnabled( detailsMenuItem, false );
// 		menu.setItemEnabled( addWorldMenuItem, false );
// 		menu.setItemEnabled( delWorldMenuItem, false );
// 		menu.setItemEnabled( uninstallMenuItem, false );
// 		menu.setItemEnabled( backupMenuItem, false );
		m_addToQueue->setDisabled(true);
		m_removeFromQueue->setDisabled(true);
		m_packageDetails->setDisabled(true);
		m_addToWorld->setDisabled(true);
		m_removeFromWorld->setDisabled(true);
		m_uninstallPackage->setDisabled(true);
		m_quickPackage->setDisabled(true);
	}

	menu.exec(packagesView->viewport()->mapToGlobal(point));
/*
	switch( menu.exec( point ) ) {

		case ADDQUEUE:
			slotEnqueue();
			break;

		case DELQUEUE:
			slotDequeue();
			break;

		case UNINSTALL:
			slotUninstall();
			break;

		case DETAILS:
			slotAdvanced();
			break;

		case ADDWORLD: {
			break;
		}

		case DELWORLD: {
			break;
		}

		case QUICKPACKAGE: {
			break;
		}
	}*/
}

