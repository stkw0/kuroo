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
#include "versionview.h"

#include <QTreeWidget>

VersionView::VersionItem::VersionItem( QTreeWidget* parent, const QString& version, const bool& isInstalled, const int& stability )
    : QTreeWidgetItem( parent ), m_isInstalled( isInstalled ), m_stability( stability )
{
	setText( 0, version );

	QFont font;
	QPalette palette;

	if ( m_isInstalled )
		font.setBold( true );
	
	switch ( m_stability )
	{
	case ( TESTING ) :
		font.setItalic( true );
		break;

	case ( HARDMASKED ) :
		font.setItalic( true );
		palette.setColor( QPalette::Text, Qt::darkRed );
	}

	setFont( 1, font );
	setFont( 2, font );
	setFont( 3, font );
	//setForeground( 0, palette.foreground() );
	//setForeground( 1, palette.foreground() );
	//setForeground( 2, palette.foreground() );
	//setForeground( 3, palette.foreground() );
}

VersionView::VersionItem::~VersionItem()
{}

/**
 * @class VersionView
 * @short Version listview.
 */
VersionView::VersionView( QWidget *parent/*, const QString& name */)
    : QTreeWidget( parent ), m_emergeVersion( QString::null )
{
	setHeaderLabels( QStringList() << " " << i18n( "Version" ) << i18n( "Stability" ) <<  i18n( "Size" ) );
	setColumnWidth( 1, 100 );
	setColumnWidth( 2, 100 );
	setColumnWidth( 3, 70 );
	setRootIsDecorated(false);
	//setColumnAlignment( 3, Qt::AlignRight );
	//setResizeMode( QTreeWidget::LastColumn );
	//setSorting( -1 );
	setSelectionMode( QTreeWidget::NoSelection );
}

VersionView::~VersionView()
{
}

void VersionView::insertItem( const QString& version, const QString& stability, const QString& size, const bool& isInstalled )
{
	VersionItem* item;
	if ( stability == i18n("Testing") )
		item = new VersionItem( this, " ", isInstalled, TESTING );
	else
		if ( stability == i18n("Hardmasked") )
			item = new VersionItem( this, " ", isInstalled, HARDMASKED );
		else
			item = new VersionItem( this, " ", isInstalled, 0 );

	item->setText( 1, version );
	item->setText( 2, stability );
	item->setText( 3, size );

	QHeaderView *hv = header();
	hv->setResizeMode(0, QHeaderView::ResizeToContents);
	hv->setResizeMode(1, QHeaderView::ResizeToContents);
	hv->setResizeMode(2, QHeaderView::ResizeToContents);
}

/**
 * Mark the installation version with icon.
 * @param version
 */
void VersionView::usedForInstallation( const QString& version )
{
    QTreeWidgetItemIterator it( this );
    while( *it ) {
        if( dynamic_cast<VersionItem*>( *it )->isInstalled() )
            m_installedIndex = indexFromItem( *it );
		
        if ( (*it)->text(1) == version ) {
            (*it)->setIcon( 0, QIcon::fromTheme(QStringLiteral("kuroo_version_installed")) );
            m_emergeIndex = indexFromItem( *it );
		}
        it++;
	}
	
	m_emergeVersion = version;
}


