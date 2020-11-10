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
#include "historylistview.h"
#include "historytab.h"
#include "ui_historybase.h"

#include <QCheckBox>
#include <QPushButton>
#include <QWhatsThis>
#include <QTextStream>

#include <KMessageBox>
#include <KTreeWidgetSearchLine>
#include <QPushButton>

/**
* @class HistoryTab
* @short Tabpage for emerge log browser.
*/
HistoryTab::HistoryTab( /*QWidget* parent */)
{
	setupUi( this );
	// Connect What's this button
	connect(pbWhatsThis, &QPushButton::clicked, this, &HistoryTab::slotWhatsThis);

	pbClearFilter->setIcon( QIcon::fromTheme(QStringLiteral("edit-clear-locationbar-ltr")) );

	historyFilter->setTreeWidget( historyView );

	// Connect button and checkbox
	connect(viewUnmerges, &QCheckBox::toggled, this, &HistoryTab::slotViewUnmerges);
	connect(pbClearFilter, &QPushButton::clicked, this, &HistoryTab::slotClearFilter);
	connect(historyView, &HistoryListView::itemDoubleClicked, this, &HistoryTab::slotViewInfo);

	// Reload view after changes.
	connect( HistorySingleton::Instance(), SIGNAL( signalHistoryChanged() ), this, SLOT( slotReload() ) );
	connect(cbDays, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, static_cast<void(HistoryTab::*)(int)>(&HistoryTab::slotReload));

	// Load history view after scan completed
	connect( HistorySingleton::Instance(), SIGNAL( signalScanHistoryCompleted() ), this, SLOT( slotReload() ) );

	connect(historyView, &HistoryListView::itemSelectionChanged, this, &HistoryTab::slotButtonView);
	connect(pbView, &QPushButton::clicked, this, &HistoryTab::slotViewInfo);

	slotInit();
}

/**
* Save checkboxes state
*/
HistoryTab::~HistoryTab()
{
	if ( viewUnmerges->isChecked() ) {
		KurooConfig::setViewUnmerges( true );
	}
	else {
		KurooConfig::setViewUnmerges( false );
	}
}

/**
* Initialize geometry and content.
* Restore checkboxes state
*/
void HistoryTab::slotInit()
{
	pbWhatsThis->setIcon( QIcon::fromTheme(QStringLiteral("help-about")) );

	pbView->setDisabled( true );

	if ( KurooConfig::viewUnmerges() ) {
		viewUnmerges->setChecked( true );
	}
	else {
		viewUnmerges->setChecked( false );
	}
}

/**
* What's this info explaning this tabs functionality.
*/
void HistoryTab::slotWhatsThis()
{
	QWhatsThis::showText( QCursor::pos(), i18n( "<qt>"
			"The emerge history keeps track of emerged/unemerged packages.<br/>"
			"Activate the log function in your /etc/make.conf if you want to trace those messages. "
			"Select an entry and press 'View emerge log'.</qt>" ), this );
}

/**
* Reload history view.
*/
void HistoryTab::slotReload()
{
	slotReload( cbDays->currentIndex() );
}

void HistoryTab::slotReload( int limit )
{
	int days;

	switch ( limit ) {
		case 0 :
			days = 7;
			break;

		case 1:
			days = 30;
			break;

		case 2:
			days = 180;
			break;

		case 3:
			days = 365;
			break;

		case 4:
			days = 10000;
			break;
	}

	historyView->loadFromDB( days );
}

void HistoryTab::slotClearFilter()
{
	historyFilter->clear();
}

void HistoryTab::slotViewUnmerges( bool on )
{
	KurooConfig::setViewUnmerges( on );
	slotReload();
}

/**
* Activate button only when any file is selected.
*/
void HistoryTab::slotButtonView()
{
	QTreeWidgetItem *item = historyView->currentItem();
	if ( item && item->parent() ) {
		pbView->setDisabled( false );
	}
	else {
		pbView->setDisabled( true );
	}
}

/**
* Open in external browser.
*/
void HistoryTab::slotViewInfo()
{
	QTreeWidgetItem *item = historyView->currentItem();
	if ( !item ) {
		return;
	}

	if ( !( item->text(2) ).isEmpty() ) {

		QString logText;
		QString eLogFile( KurooConfig::dirELog() + "/" + dynamic_cast<HistoryListView::HistoryItem*>( item )->einfo() );
		QFile logFile( eLogFile );
		if ( logFile.open( QIODevice::ReadOnly ) ) {
			QTextStream stream( &logFile );
			while ( !stream.atEnd() ) {
				logText += stream.readLine() + "<br/>";
			}
			logFile.close();

			Message::instance()->prompt( i18n( "Emerge log" ), i18n( "Installation message for %1:", item->text(0) ), logText );
		}
		else {
			qCritical() << "Reading: " << eLogFile;
			KMessageBox::error( this, i18n( "Can not find elog for this emerge. Please check elog settings in /etc/make.conf.\nError reading:\n" ) + eLogFile
								, i18n( "emerge elog" ) );
		}
	}
}

