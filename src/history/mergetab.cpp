/***************************************************************************
 *   Copyright (C) 2005 by Karye   *
 *   karye@users.sourceforge.net   *
 *                                                                         *
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

#include "common.h"
#include "historylistview.h"
#include "mergelistview.h"
#include "mergetab.h"

#include <QCheckBox>
#include <QPushButton>
#include <QWhatsThis>

#include <KTreeWidgetSearchLine>
#include <QPushButton>

/**
 * @class MergeTab
 * @short Tabpage for emerge log browser.
 */
MergeTab::MergeTab( QWidget* parent )
    : QWidget( parent )
{
	setupUi( this );

	// Connect What's this button
	connect(pbWhatsThis, &QPushButton::clicked, this, &MergeTab::slotWhatsThis);

	pbClearFilter->setIcon( QIcon::fromTheme(QStringLiteral("edit-clear-locationbar-ltr")) );

	mergeFilter->setTreeWidget( mergeView );

	connect( EtcUpdateSingleton::Instance(), SIGNAL( signalScanCompleted() ), this, SLOT( slotLoadConfFiles() ) );
	connect( EtcUpdateSingleton::Instance(), SIGNAL( signalEtcFileMerged() ), this, SLOT( slotReload() ) );

	// When all packages are emerged...
	connect( EmergeSingleton::Instance(), SIGNAL( signalEmergeComplete() ), this, SLOT( slotReload() ) );

	connect(unmergeView, &MergeListView::itemSelectionChanged, this, &MergeTab::slotButtonMerge);
	connect(mergeView, &MergeListView::itemSelectionChanged, this, &MergeTab::slotButtonView);

	connect(pbMerge, &QPushButton::clicked, this, &MergeTab::slotMergeFile);
	connect(pbView, &QPushButton::clicked, this, &MergeTab::slotViewFile);

	slotInit();
}

/**
 * Save splitters and listview geometry.
 */
MergeTab::~MergeTab()
{}

/**
 * Initialize geometry and content.
 */
void MergeTab::slotInit()
{
	pbWhatsThis->setIcon( QIcon::fromTheme(QStringLiteral("help-about")) );
	unmergeView->setHeaderLabel( i18n("New Configuration file") );
	mergeView->setHeaderLabel( i18n("Merged Configuration file") );
	mergeView->setSelectionMode(QAbstractItemView::SingleSelection);
	unmergeView->setSelectionMode(QAbstractItemView::SingleSelection);
	pbMerge->setDisabled( true );
	pbView->setDisabled( true );
	slotReload();
}

/**
 * What's this info explaning this tabs functionality.
 */
void MergeTab::slotWhatsThis()
{
	QWhatsThis::showText( QCursor::pos(), i18n( "<qt>"
			"This tab keeps track of all configuration files that need to be merged.<br/>"
			"Your system is scanned automatically for configuration files after completed installation.<br/>"
			"Select a file to merge and press 'Merge changes'. Kompare will then open with old and new files. "
			"After saving changes in Kompare the configuration file will be removed. "
			"Old merged changes can then be reviewed in the right list.</qt>" ), this );
}

/**
 * Reload history view.
 */
void MergeTab::slotReload()
{
	DEBUG_LINE_INFO;
	EtcUpdateSingleton::Instance()->slotEtcUpdate();
}

/**
 * List new configuration files in mergeView.
 */
void MergeTab::slotLoadConfFiles()
{
	DEBUG_LINE_INFO;
	QStringList confFilesList = EtcUpdateSingleton::Instance()->confFilesList();
	unmergeView->loadConfFiles( confFilesList );

	QStringList backupFilesList = EtcUpdateSingleton::Instance()->backupFilesList();
	mergeView->loadBackupFiles( backupFilesList );

	emit signalMergeChanged();
}

void MergeTab::slotClearFilter()
{
	mergeFilter->clear();
}

/**
 * Activate buttons only when file is selected.
 */
void MergeTab::slotButtonView()
{
	QTreeWidgetItem *item = mergeView->currentItem();
	if ( item && item->parent() ) {
		unmergeView->clearSelection();
		pbView->setDisabled( false );
		pbMerge->setDisabled( true );
	}
	else {
		pbMerge->setDisabled( true );
		pbView->setDisabled( true );
	}
}

/**
 * Activate buttons only when file is selected.
 */
void MergeTab::slotButtonMerge()
{
	QTreeWidgetItem *item = unmergeView->currentItem();
	if ( item ) {
		mergeView->clearSelection();
		pbMerge->setDisabled( false );
		pbView->setDisabled( true );
	}
	else {
		pbMerge->setDisabled( true );
		pbView->setDisabled( true );
	}
}

/**
 * View merged changes in diff tool.
 */
void MergeTab::slotViewFile()
{
    QTreeWidgetItem *item = mergeView->currentItem();
	if ( !item || !item->parent() )
		return;

	QString source = dynamic_cast<MergeListView::MergeItem*>( item )->source();
	QString destination = dynamic_cast<MergeListView::MergeItem*>( item )->destination();

	EtcUpdateSingleton::Instance()->runDiff( source, destination/*, false */);
}

/**
 * Launch diff tool to merge changes.
 */
void MergeTab::slotMergeFile()
{
	QTreeWidgetItem *item = unmergeView->currentItem();
	if ( !item && item->parent() )
		return;

	QString source = dynamic_cast<MergeListView::MergeItem*>( item )->source();
	QString destination = dynamic_cast<MergeListView::MergeItem*>( item )->destination();

	EtcUpdateSingleton::Instance()->runDiff( source, destination/*, true */);
}

