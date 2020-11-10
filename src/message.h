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

#ifndef MESSAGE_H
#define MESSAGE_H

#include <QDialog>
#include "ui_messagebase.h"

/**
 * Dialog for simple messages to user.
 */
//CLEAN: just use QDialog
class Message : public QDialog, public Ui::MessageBase
{
Q_OBJECT
    static Message* s_instance;
public:
	Message( QWidget *parent = 0 );
    ~Message();

	static Message* instance() { return s_instance; }

public slots:
	void			prompt( const QString& caption, const QString& label, const QString& text );

private slots:
	void			setLabel( const QString& label );
	void			slotUser1();

private:
	QString			m_text;
};

#endif
