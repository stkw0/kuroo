/***************************************************************************
*	Copyright (C) 2004 by karye												*
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

#include "common.h"
#include "packageemergetime.h"
#include "historylistview.h"

#include <QTreeWidget>
#include <QMouseEvent>
#include <QFrame>

/**
* @class HistoryItem
* @short ListViewItem for package emerge/unmerges date
*/
HistoryListView::HistoryItem::HistoryItem( QTreeWidget* parent, const QString& date )
: QTreeWidgetItem( parent, QStringList(date) )
{}

HistoryListView::HistoryItem::HistoryItem( HistoryItem* parent, const QString& package )
: QTreeWidgetItem( parent, QStringList(package) ), m_einfo( QString::null )
{}

void HistoryListView::HistoryItem::setEinfo( const QString& einfo )
{
	m_einfo = einfo;
	setText( 2, m_einfo.section( "<br/>", 0, 0 ) );
}

QString HistoryListView::HistoryItem::einfo()
{
	return m_einfo;
}

/**
* @class HistoryListView
* @short Specialized listview for emerge history.
*/
HistoryListView::HistoryListView( QWidget *parent/*, const char *name */)
	: QTreeWidget( parent /*, name*/ ), m_loc( KLocale::global() )
{
	setColumnCount( 3 );
	QTreeWidgetItem * header = new QTreeWidgetItem((QTreeWidget *)0); // the header must be NOT child of QTreeWidget
	header->setText( 0, i18n("Date") );
	header->setText( 1, i18n("Duration") );
	header->setText( 2, i18n("Emerge log file") );
	setHeaderItem( header );

	//setSelectionMode( QAbstractItemView::ExtendedSelection );
	//setProperty( "selectionMode", "Extended" );
	setFrameShape( QFrame::NoFrame );
	setRootIsDecorated( true );
	//setFullWidth( true );

	/*setColumnWidthMode( 0, QTreeWidget::Manual );
	setColumnWidthMode( 1, QTreeWidget::Manual );
	setColumnWidthMode( 2, QTreeWidget::Manual );*/

	setColumnWidth( 0, 300 );
	setColumnWidth( 1, 80 );
	//setResizeMode( QTreeWidget::LastColumn );

	setSortingEnabled( false );
	//setSorting( -1 );

	setAlternatingRowColors( true );
}

HistoryListView::~HistoryListView()
{}

/**
* @return current entry.
*/
QString HistoryListView::current()
{
	QTreeWidgetItem *item = currentItem();

	if ( item && item->parent() )
		return item->text(0);
	else
		return QString::null;
}

/**
* @return list of selected packages.
*/
QStringList HistoryListView::selected()
{
	QStringList packageList;
	foreach( QTreeWidgetItem* item, selectedItems() ) {
		//if ( item.parent() && item.isSelected() ) {
			packageList += item->text(0);
		//}
	}
	return packageList;
}

void HistoryListView::mouseDoubleClickEvent(QMouseEvent *event)
{
	QModelIndex index = indexAt(event->pos());
	if (!index.isValid())
		return;

	HistoryItem *item = static_cast<HistoryItem*>(index.internalPointer());
	if (!item)
		return;

	emit itemDoubleClicked(item);
}

/**
* Populate listview with log entries
*/
void HistoryListView::loadFromDB( int days )
{
	clear();
	m_itemMap.clear();

	QDateTime dtLimit = QDateTime::currentDateTime();
	dtLimit = dtLimit.addDays( -days );

	const QStringList historyList = KurooDBSingleton::Instance()->allHistory();
	for( QStringList::ConstIterator it = historyList.begin(), end = historyList.end(); it != end; ++it ) {
		QString timeStamp = *it++;
		QString package = *it++;
		QString duration = *it++;
		QString einfo = *it;
		einfo.replace( "&gt;", ">" ).replace( "&lt;", "<" );

		QStringList parts = parsePackage( package );
		QString packageString = parts[1] + "-" + parts[2] + " (" + parts[0].section( "-", 0, 0 ) + "/" +  parts[0].section( "-", 1, 1 ) + ")";

		// Convert emerge date to local date format
		QDateTime dt;
		dt.setTime_t( timeStamp.toUInt() );
		QString emergeDate = m_loc->formatDate( dt.date() );

		if ( dt >= dtLimit ) {

			// Convert emerge duration (in seconds) to local time format
			QTime t( 0, 0, 0 );
			t = t.addSecs( duration.toUInt() );
			QString emergeDuration = m_loc->formatTime( t, true, true );

			if ( !duration.isEmpty() || ( KurooConfig::viewUnmerges() && !package.isEmpty() ) ) {
				if ( !m_itemMap.contains( emergeDate ) ) {
					HistoryItem *item = new HistoryItem( this, emergeDate );
					item->setExpanded( true );//->setOpen( true );
					m_itemMap[ emergeDate ] = item;
				}

				HistoryItem *item = new HistoryItem( m_itemMap[ emergeDate ], packageString );
				if ( duration.isEmpty() )
					item->setIcon( 0, QIcon::fromTheme(QStringLiteral("kuroo_unmerged")) );
				else {
					item->setIcon( 0, QIcon::fromTheme(QStringLiteral("kuroo_new")) );
					item->setText( 1, emergeDuration );
					item->setEinfo( einfo );
				}
			}
		}
	}

	// Count emerge/unmerge events
	for( int i = 0; i < topLevelItemCount(); i++ ) {
		QTreeWidgetItem *item = topLevelItem(i);
		QString events = item->text(0) + " (" + QString::number( item->childCount() ) + ")";
		item->setText( 0, events );
	}

	emit signalHistoryLoaded();
}

