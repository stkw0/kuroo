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

#ifndef MERGELISTVIEW_H
#define MERGELISTVIEW_H

#include <QTreeWidget>

class QTreeWidgetItem;

/**
 * @class MergeListView
 * @short Specialized listview for merge history.
 */
class MergeListView : public QTreeWidget
{
Q_OBJECT
public:
    MergeListView(QWidget *parent = 0/*, const QString& name = 0*/);
	~MergeListView();
	
	class			MergeItem;
	
	void			loadConfFiles( const QStringList& confFilesList );
	void			loadBackupFiles( const QStringList& confFilesList );
	
signals:
	void    		signalHistoryLoaded();
	
private:
	KLocale 		*m_loc;
	
	typedef QMap< QString, MergeItem* > ItemMap;
	ItemMap			m_itemMap;
};

/**
 * @class MergeItem
 * @short ListViewItem with merge files.
 */
class MergeListView::MergeItem : public QTreeWidgetItem
{
public:
    MergeItem( QTreeWidget* parent, const QString& date );
    MergeItem( QTreeWidget* parent, const QString& source, const QString& destination );
    MergeItem( MergeItem* parent, const QString& source, const QString& destination );
	
	QString			source();
	QString			destination();
	
private:
	QString 		m_source, m_destination;
};

#endif
