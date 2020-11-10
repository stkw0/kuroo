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
#include "logstab.h"

#include <QCheckBox>
#include <QPushButton>

#include <KInputDialog>

/**
 * @class LogsTab
 * @short Tabpage for emerge log browser, emerge history and portage directories sizes.
 * 
 * @todo: view log-file content instead, let user select how many lines to view.
 */
LogsTab::LogsTab( QWidget* parent ) : QWidget( parent )
{
	setupUi( this );
	connect(pbEnter, &QPushButton::clicked, this, &LogsTab::slotUserInput);

	// Enable/disable this view and buttons when kuroo is busy
	connect( SignalistSingleton::Instance(), SIGNAL( signalKurooBusy( bool ) ), this, SLOT( slotBusy() ) );

	connect( SignalistSingleton::Instance(), SIGNAL( signalFontChanged() ), this, SLOT( slotSetFont() ) );
	
// 	logBrowser->setTextFormat( Qt::LogText ); // Text doesn't wrap in log mode!
	init();
}

/**
 * Save checkboxes state.
 */
LogsTab::~LogsTab()
{
	if ( saveLog->isChecked() )
		KurooConfig::setSaveLog( true );
	else
		KurooConfig::setSaveLog( false );

	if ( verboseLog->isChecked() )
		KurooConfig::setVerboseLog( true );
	else
		KurooConfig::setVerboseLog( false );

	KurooConfig::self()->save();
}

/**
 * Restore checkboxes state
 */
void LogsTab::init()
{
	if ( KurooConfig::saveLog() )
		saveLog->setChecked( true );
	else
		saveLog->setChecked( false );

	if ( KurooConfig::verboseLog() )
		verboseLog->setChecked( true );
	else
		verboseLog->setChecked( false );

	slotSetFont();
}

/**
 * Open dialog for manually sending text to running emerge process.
 */
void LogsTab::slotUserInput()
{
	QString input = KInputDialog::getText( i18n("User Input"), i18n("Enter text:"), QString::null, 0, this, 0, 0, QString::null );
	EmergeSingleton::Instance()->inputText( input );
}

/**
 * Disable/enable buttons when kuroo is busy.
 */
void LogsTab::slotBusy()
{
	if ( SignalistSingleton::Instance()->isKurooBusy() )
		pbEnter->setDisabled( false );
	else
		pbEnter->setDisabled( true );
}

void LogsTab::slotSetFont()
{
	logBrowser->setFont( KurooConfig::logFont() );
}

