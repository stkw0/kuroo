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

#ifndef _KUROO_H_
#define _KUROO_H_

#include <KXmlGuiWindow>

#include "kurooview.h"
#include "configdialog.h"

class QAction;
class KConfigDialog;
class IntroDlg;
class KurooInit;
class Message;
class Queue;
class Results;
class SystemTray;
class KurooStatusBar;

/**
 * @class Kuroo
 * @short Main kde window with menus, system tray icon and statusbar.
 */
class Kuroo : public KXmlGuiWindow
{
Q_OBJECT
public:
	Kuroo();
	virtual ~Kuroo();

private slots:
	void				introWizard();
	void				slotPreferences();
	void				slotBusy();
	void				slotSync();
	bool				queryClose();
	bool				queryExit();
	void				slotQuit();
	void				slotWait();
	void				slotTerminate();
	//TODO: What happened to this?
	//void				slotWhatsThis( int tabIndex );

private:
	void				setupActions();

private:
	SystemTray			*systemTray;
	Results				*kurooResults;
	Queue				*kurooQueue;
	Message				*kurooMessage;
	KurooInit			*kurooInit;
	KurooView			*m_view;
	KConfigDialog		*prefDialog;
	IntroDlg			*wizardDialog;
	KurooStatusBar		*sb;
	bool				m_shuttingDown;
	QAction *actionRefreshPortage, *actionRefreshUpdates, *actionSyncPortage;
};

#endif // _KUROO_H_

