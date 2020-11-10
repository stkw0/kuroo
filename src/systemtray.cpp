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

#include <assert.h>

#include <QThread>
#include <QMenu>

#include "common.h"
#include "systemtray.h"

SystemTray* SystemTray::s_instance = 0;

/**
 * @class SystemTray
 * @short Singleton object that creates the kuroo systemtray icon and actions.
 *
 * Displays kuroo icon in systemtray and switches to "busy" icon when kuroo is busy.
 * The user can disable the systemtray icon in settings.
 */
SystemTray::SystemTray( QWidget *parent )
	: KStatusNotifierItem( parent )
{
	s_instance = this;

	//Rearranged some things trying to get rid of 'QSystemTrayIcon::setVisible: No Icon set' message (didn't work)
	setIconByName( "kuroo" );
	setCategory( KStatusNotifierItem::ApplicationStatus );
	setStatus( KStatusNotifierItem::Active );
	setToolTipTitle( i18n("Kuroo - Portage frontend") );
	//setting associated widget as recommended in http://techbase.kde.org/Development/Tutorials/PortToKStatusNotifierItem
	setAssociatedWidget( parent );

	contextMenu()->addAction( i18n("&Configure Kuroo..."), this, SLOT( slotPreferences() ) );
	m_menuPause = contextMenu()->addAction( i18n("Pause Emerge"), this, SLOT( slotPause() ) );
	m_menuUnpause = contextMenu()->addAction( i18n("Unpause Emerge"), this, SLOT( slotUnpause() ) );

	m_menuPause->setEnabled( false );
	m_menuUnpause->setEnabled( false );

	connect( SignalistSingleton::Instance(), SIGNAL( signalKurooBusy(bool) ), this, SLOT( slotBusy(bool) ) );
}

SystemTray::~SystemTray()
{}

/*void SystemTray::activate()
{
	slotBusy( false );
	setStatus( KStatusNotificationItem::Active );
	//show();
}

void SystemTray::inactivate()
{
	setStatus( KStatusNotifierItem::Passive );
	//hide();
}*/

void SystemTray::slotPreferences()
{
	emit signalPreferences();
}

void SystemTray::slotPause()
{
	if( EmergeSingleton::Instance()->canPause() ) {
		m_menuPause->setEnabled( false );
		m_menuUnpause->setEnabled( true );
		EmergeSingleton::Instance()->slotPause();
	}
}

void SystemTray::slotUnpause()
{
	if( EmergeSingleton::Instance()->canPause() ) {
		m_menuPause->setEnabled( true );
		m_menuUnpause->setEnabled( false );
		EmergeSingleton::Instance()->slotUnpause();
	}
}

/**
 * Show busy kuroo icon.
 * @param	kuroo state = busy or ready
 */
void SystemTray::slotBusy( bool busy )
{
	assert(QThread::currentThread() == qApp->thread());
	if ( busy ) {
		setIconByName( "kuroo_emerging" );
		if( EmergeSingleton::Instance()->isPaused() && EmergeSingleton::Instance()->canPause() ) {
			m_menuUnpause->setEnabled( true );
		} else if( !EmergeSingleton::Instance()->isPaused() && EmergeSingleton::Instance()->canPause() ) {
			m_menuPause->setEnabled( true );
		}
	}
	else {
		setIconByName( "kuroo" );
		if( EmergeSingleton::Instance()->canPause() && EmergeSingleton::Instance()->isPaused() ) {
			m_menuUnpause->setEnabled( true );
		} else {
			m_menuPause->setEnabled( false );
			m_menuUnpause->setEnabled( false );
		}
	}
}

