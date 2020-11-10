/***************************************************************************
*   Copyright (C) 2004 by karye                                           *
*   karye@users.sourceforge.net                                           *
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

#ifndef UNINSTALLINSPECTOR_H
#define UNINSTALLINSPECTOR_H

#include <QDialog>
#include "ui_uninstallbase.h"
#include <QDialogButtonBox>

/**
 * @class UninstallInspector
 * @short Dialog for selected package and version to uninstall.
 */
class UninstallInspector : public QDialog
{
Q_OBJECT
public:
	UninstallInspector( QWidget *parent = 0 );
    ~UninstallInspector();

	void			view( const QStringList& packageList );

private slots:
	void			slotOk();
	void			slotItemActivated(QTreeWidgetItem*/*, int col */);
	
private:
    Ui::UninstallBase m_uninstallbase;
	QDialogButtonBox* buttonBox;
};

#endif
