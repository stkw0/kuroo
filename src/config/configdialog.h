/**************************************************************************
*   Copyright (C) 2004 by                                                 *
*   karye@users.sourceforge.net                                           *
*   Stefan Bogner <bochi@online.ms>                                       *
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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <KConfigDialog>
#include <KConfigSkeleton>

#include "ui_options1.h"
#include "ui_options2.h"
#include "ui_options3.h"
#include "ui_options4.h"

/**
 * @class ConfigDialog
 * @short Kuroo preferences.
 */
class ConfigDialog : public KConfigDialog
{
Q_OBJECT

public:
    ConfigDialog( QWidget *parent, const QString& name, KConfigSkeleton *config );
	~ConfigDialog();
	
private:
	const QStringList 	readMakeConf();
	void 				parseMakeConf();
	bool 				saveMakeConf();
	
private slots:
	void 				slotSaveAll();
	void				slotDefaults();
	
private:
	bool				m_isDefault;
	Ui::Options1 form1;
	Ui::Options2 form2;
	Ui::Options3 form3;
	Ui::Options4 form4;
};

#endif
