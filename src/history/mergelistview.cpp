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

#include "common.h"
#include "mergelistview.h"

#include <QTreeWidget>

/**
 * @class MergeItem
 * @short ListViewItem with merge files.
 */
MergeListView::MergeItem::MergeItem( QTreeWidget* parent, const QString& date )
    : QTreeWidgetItem( parent )
{
    setText( 0, date );
}

MergeListView::MergeItem::MergeItem( QTreeWidget* parent, const QString& source, const QString& destination )
    : QTreeWidgetItem( parent ), m_source( source ), m_destination( destination )
{
	setText( 0 , m_source );
}

MergeListView::MergeItem::MergeItem( MergeItem* parent, const QString& source, const QString& destination )
    : QTreeWidgetItem( parent ), m_source( source ), m_destination( destination )
{
	setText( 0 , m_source.section( QRegExp( "\\d{8}_\\d{4}/" ), 1, 1 ).replace( ":" , "/" ) );
}

QString MergeListView::MergeItem::source()
{
	return m_source;
}

QString MergeListView::MergeItem::destination()
{
	return m_destination;
}

/**
 * @class MergeListView
 * @short Specialized listview for emerge history.
 */
MergeListView::MergeListView( QWidget *parent/*, const QString& name */)
    : QTreeWidget( parent ), m_loc( KLocale::global() )
{
    setHeaderLabel( i18n("Configuration file") );
	
	setProperty( "selectionMode", "Extended" );
    //setFrameShape( QFrame::NoFrame );
	setRootIsDecorated( true );
    //setFullWidth( true );

	setColumnWidth( 0, 300 );
	setColumnWidth( 1, 300 );
    //setColumnWidthMode( 0, QTreeWidget::Manual );
    //setResizeMode( QTreeWidget::LastColumn );
	
    //setSorting( -1 );
}

MergeListView::~MergeListView()
{}

/**
 * Append the new unmerged configuration files.
 */
void MergeListView::loadConfFiles( const QStringList& filesList )
{
	clear();
	m_itemMap.clear();
	
	foreach ( QString file, filesList ) {
		QString source = file;
		QString destination = source;
		destination.remove( QRegExp("\\._cfg\\d\\d\\d\\d_") );
		new MergeItem( this, source, destination );
	}
}

/**
 * Insert merged configurations files from backup.
 */
void MergeListView::loadBackupFiles( const QStringList& filesList )
{
	clear();
	m_itemMap.clear();
	
	// Find dates
    foreach ( QString file, filesList ) {
        QString date = file.section( "/", -2, -2 ).mid(0,8);
		if ( !m_itemMap.contains( date ) )
			m_itemMap[ date ] = NULL;
	}
	
	// Insert sorted dates
	for ( ItemMap::Iterator it = m_itemMap.begin(); it != m_itemMap.end(); ++it ) {
		QString date = it.key();
		QString localDate = m_loc->formatDate( QDate( date.mid(0,4).toInt(), date.mid(4,2).toInt(), date.mid(6,2).toInt() ) );
		
		MergeItem *item = new MergeItem( this, localDate );
        item->setExpanded( true );
		m_itemMap[ date ] = item;
	}
	
	// Insert files
    foreach ( QString source, filesList ) {
		QString date = source.section( "/", -2, -2 ).mid(0,8);
		new MergeItem( m_itemMap[ date ], source, source + ".orig" );
	}
}

