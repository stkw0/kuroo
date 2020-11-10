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

#ifndef LOGSTAB_H
#define LOGSTAB_H

#include "ui_logsbase.h"

/**
 * @class LogsTab
 * @short Tabpage for emerge log browser.
 */
class LogsTab : public QWidget, public Ui::LogsBase
{
Q_OBJECT
public:
    LogsTab( QWidget *parent = 0 );
    ~LogsTab();
	
private:
	void 		init();
	
private slots:
	void		slotUserInput();
	void		slotBusy();
	void		slotSetFont();
	
};

#endif
