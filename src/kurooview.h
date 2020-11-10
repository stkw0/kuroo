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

#ifndef _KUROOVIEW_H_
#define _KUROOVIEW_H_

#include <QWidget>
//#include <kurooiface.h>

#include <KPageWidget>

class PortageTab;
class QueueTab;
class HistoryTab;
class LogsTab;
class MergeTab;
class PackageInspector;

/**
 * @class KurooView
 * @short Create the gui content with icon-menu and pages.
 */
class KurooView : public KPageWidget
{
Q_OBJECT
public:
	KurooView( QWidget *parent );
	~KurooView();

	PortageTab*			viewPortage;
	HistoryTab*			viewHistory;
	QueueTab*			viewQueue;
	LogsTab* 			viewLogs;
	MergeTab*			viewMerge;
	PackageInspector*	packageInspector;

	void 				slotEmergePretend( QString package );

public slots:
	void 				slotInit();

private slots:
	void 				slotCheckPortage();
	//void				slotResetMenu( KPageWidgetItem* menuItem );
	void 				slotShowView();

private:
	// True if history needs to be recreated from scratch
	bool				m_isHistoryRestored;
};

#endif // _KUROOVIEW_H_
