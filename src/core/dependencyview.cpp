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
#include "dependencyview.h"

#include <qpainter.h>

enum Format {
		DEPENDENCY_PACKAGE,
		DEPENDENCY_USE,
		DEPENDENCY_HEADER,
		DEPENDENCY_OPERATOR
};

// capture positions inside the regexp. (like m_rxAtom.cap(POS_CALLSIGN))
enum Positions {
		POS_CALLSIGN = 1,
		POS_PREFIX,
		POS_CATEGORY,
		POS_SUBCATEGORY,
		POS_PACKAGE,
		POS_VERSION
};


/**
* @class DependencyItem
* @short Subclass for formating text.
*/
class DependencyView::DependencyItem : public QTreeWidgetItem
{
public:
		DependencyItem( QTreeWidget* parent, const QString& name, int index, int format );
		DependencyItem( QTreeWidgetItem* parent, const QString& name, int index, int format );
	~DependencyItem();

protected:
		void 			paintCell( QPainter *p, const QPalette &cg, int column/*, int width*/, int alignment );
		virtual int     compare( QTreeWidgetItem* i/*, int col*/, bool ascending ) const;
		int index() { return m_index; }

private:
	int				m_index, m_format;
};

DependencyView::DependencyItem::DependencyItem( QTreeWidget* parent, const QString& name, int index, int format )
		: QTreeWidgetItem( parent ), m_index( index ), m_format( format )
{
QTreeWidgetItem::setText( 0, name );
}

DependencyView::DependencyItem::DependencyItem( QTreeWidgetItem* parent, const QString& name, int index, int format )
		: QTreeWidgetItem( parent ), m_index( index ), m_format( format )
{
	QTreeWidgetItem::setText( 0, name );
}

DependencyView::DependencyItem::~DependencyItem()
{}

/**
* Order items first inserted as top-item.
*/
int DependencyView::DependencyItem::compare( QTreeWidgetItem* item/*, int col*/, bool ascending ) const
{
	int a = m_index;
	int b = dynamic_cast<DependencyItem*>( item )->index();

	if ( a == b )
		return 0;

	if ( ascending )
		return a > b ? 1 : -1;
	else
		return a < b ? -1 : 1;
}

/**
* Format dependency-items nicely.
*/
void DependencyView::DependencyItem::paintCell( QPainter *p, const QPalette &palette, int column/*, int width*/, int alignment )
{
		QPalette m_palette( palette );
	QFont font( p->font() );

	switch ( m_format ) {
		case ( DEPENDENCY_HEADER ) :
			font.setBold( true );
			break;

		case ( DEPENDENCY_OPERATOR ) :
			font.setItalic( true );
						m_palette.setColor( QPalette::Active, QPalette::Text, QPalette::Dark );
			break;

		case ( DEPENDENCY_USE ) :
			font.setItalic( true );
						m_palette.setColor( QPalette::Active, QPalette::Text, QPalette::Dark );
			break;

// 		case ( DEPENDENCY_PACKAGE ) :
// 			font.setUnderline( true );
// 			m_cg.setColor( QColorGroup::Text, m_cg.link() );
// 			break;
	}

		setFont( column, font );
		setTextAlignment( column, alignment );
		//setForeground ?
		//QTreeWidgetItem::paintCell( p, m_palette, column, width, alignment );
}


///////////////////////////////////////////////////////////////////////////////////////////
// Listview stuff
///////////////////////////////////////////////////////////////////////////////////////////

/**
* @class DependencyView
* @short Listview to build dependency-tree view.
*/
DependencyView::DependencyView( QWidget *parent/*, const char *name */)
		: QTreeWidget( parent )
{
	//QTreeWidget::setText( 0, name );
	setHeaderLabel( i18n( "Dependency" ) );
	//setResizeMode( QTreeWidget::LastColumn );
	//setSorting( -1 );
	// 	connect( this, SIGNAL( executed( QListViewItem* ) ), this, SLOT( slotPackageClicked( QListViewItem* ) ) );
}

DependencyView::~DependencyView()
{
}

/**
* Forward signal when user click on package.
*/
void DependencyView::slotPackageClicked( QTreeWidgetItem* item )
{
	PortageAtom atom( item->text(0) );
	if( !atom.isValid() ) return;
	SignalistSingleton::Instance()->packageClicked( atom.category() + " " + atom.package() );
}

/**
* Populate the tree with all dependencies.
* @param	list of depend atoms
*/
void DependencyView::insertDependAtoms( const QStringList& dependAtomsList )
{
	int index( 0 );
	DependencyItem *parent, *lastDepend;
	QString lastWord;

		foreach( QString word, dependAtomsList ) {
		index++;

		// Insert Depend-headers
		if ( word == "DEPEND=" ) {
			parent = new DependencyItem( this, i18n("Compile-time dependencies"), index, DEPENDENCY_HEADER );
						parent->setExpanded( true );
			continue;
		}

		if ( word == "RDEPEND=" ) {
			parent = new DependencyItem( this, i18n("Runtime dependencies"), index, DEPENDENCY_HEADER );
						parent->setExpanded( true );
			continue;
		}

		if ( word == "PDEPEND=" ) {
			parent = new DependencyItem( this, i18n("Post-merge dependencies"), index, DEPENDENCY_HEADER );
						parent->setExpanded( true );
			continue;
		}

		// Safety check
		if ( !parent )
			continue;

		// Indent one step
		if ( word == "(" ) {
			if ( word != lastWord )
				parent = lastDepend;
			else
				parent = new DependencyItem( parent, parent->text(0), index, DEPENDENCY_OPERATOR );

						parent->setExpanded( true );
			lastWord = word;
			continue;
		}
		lastWord = word;

		// Remove one indent step
		if ( word == ")" ) {
			if ( parent->parent() )
				parent = dynamic_cast<DependencyItem*>( parent->parent() );
			continue;
		}

		// OR-header
		if ( word == "||" ) {
			lastDepend = new DependencyItem( parent, i18n("Depend on either:"), index, DEPENDENCY_OPERATOR );
						lastDepend->setExpanded( true );
			continue;
		}

		// Insert package
		if ( word.contains( "/" ) ) {
			lastDepend = new DependencyItem( parent, word, index, DEPENDENCY_PACKAGE );
			continue;
		}

		// Insert use
		word.remove( '?' );
		if ( word.startsWith("!") ) {
			word.remove( '!' );
			lastDepend = new DependencyItem( parent, i18n( "Without USE-flag %1:", word ), index, DEPENDENCY_USE );
		}
		else {
			lastDepend = new DependencyItem( parent, i18n( "With USE-flag %1:", word ), index, DEPENDENCY_USE );
		}
	}

	sortItems( 0, Qt::DescendingOrder );
}

