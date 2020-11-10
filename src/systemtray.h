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

#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

//#include <KSystemTrayIcon>
#include <KStatusNotifierItem>
#include <QAction>


/**
 * @class SystemTray
 * @short Singleton object that creates the kuroo systemtray icon and actions.
 */
class SystemTray : public KStatusNotifierItem
{
Q_OBJECT
	static SystemTray* s_instance;

public:
	SystemTray( QWidget *parent = 0 );
	~SystemTray();

	static SystemTray* 	instance() { return s_instance; }
	//void				inactivate();

private slots:
	void 				slotPreferences();
	void				slotBusy( bool busy );
	void				slotPause();
	void				slotUnpause();

private:
	QAction*			m_menuPause;
	QAction*			m_menuUnpause;

signals:
	void				signalPreferences();
};

#endif
