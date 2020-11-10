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
#include "packageitem.h"
#include "packageversion.h"
#include "dependatom.h"

#include <QPainter>
#include <QList>
#include <QTreeWidget>

/**
* @class PackageItem
* @short Base class for package.
*/
PackageItem::PackageItem( QTreeWidget* parent, const QString& name, const QString& id, const QString& category, const QString& description, const int status )
	: QTreeWidgetItem( parent ),
	m_parent( parent ), m_isMouseOver( false ), m_index( 0 ),
	m_id( id ), m_name( name ), m_status( status ), m_description( description ),
	m_category( category ), m_isQueued( false ), m_inWorld( false ),
	m_isInitialized( false )
{
	setText( 0, name );
	//TODO: Tihs is probably not the most efficient place to put this
	if ( m_status & PACKAGE_AVAILABLE )
		setIcon( 0, QIcon::fromTheme(QStringLiteral("kuroo_package")) );
	else {
		if ( KurooConfig::installedColumn() ) {
			setIcon( 0, QIcon::fromTheme(QStringLiteral("kuroo_package")) );
			setIcon( 1, QIcon::fromTheme(QStringLiteral("kuroo_version_installed")) );
		}
		else
			setIcon( 0, QIcon::fromTheme(QStringLiteral("kuroo_stable")) );

	}
	if ( !this->isHidden() && PortageSingleton::Instance()->isInWorld( m_category + "/" + m_name ) )
		m_inWorld = true;
}

PackageItem::PackageItem( QTreeWidgetItem* parent, const QString& name, const QString& id, const QString& category, const QString& description, const int status )
	: QTreeWidgetItem( parent ),
	m_parent( parent->treeWidget() ), m_isMouseOver( false ), m_index( 0 ),
	m_id( id ), m_name( name ), m_status( status ), m_description( description ), m_category( category ), m_isQueued( false ), m_inWorld( false ),
	m_isInitialized( false )
{
	setText( 0, name );
	if ( !this->isHidden() && PortageSingleton::Instance()->isInWorld( m_category + "/" + m_name ) )
		m_inWorld = true;
}

PackageItem::~PackageItem()
{}

/**
* Set icons when package is visible.
*/
void PackageItem::paintCell( QPainter* painter, const QPalette& palette, int column, int width, int alignment )
{
	if ( !this->isHidden() ) {
		QPalette m_palette( palette );
		QFont font( painter->font() );

		if ( m_isMouseOver ) {
			font.setBold( true );
			painter->setFont( font );
// 			m_colorgroup.setColor( QColorGroup::Base, m_colorgroup.dark() );
// 			QTreeWidgetItem::paintCell( painter, m_colorgroup, column, width, alignment );
		}

		// Optimizing - check only relevant columns
		switch ( column ) {

			case 0 : {
				if ( m_status & PACKAGE_AVAILABLE )
					setIcon( 0, QIcon::fromTheme(QStringLiteral("kuroo_package")) );
				else {
					if ( KurooConfig::installedColumn() ) {
						setIcon( 0, QIcon::fromTheme(QStringLiteral("kuroo_package")) );
						setIcon( 1, QIcon::fromTheme(QStringLiteral("kuroo_version_installed")) );
					}
					else
						setIcon( 0, QIcon::fromTheme(QStringLiteral("installed")) );

					if ( m_status & PACKAGE_OLD ) {
						font.setItalic( true );
						painter->setFont( font );
						m_palette.setColor( QPalette::Text, palette.color(QPalette::Dark) );
					}
				}
				break;
			}

			case 2 : {
				if ( PortageSingleton::Instance()->isInWorld( m_category + "/" + m_name ) ) {
					m_inWorld = true;
					setIcon( 2, QIcon::fromTheme(QStringLiteral("kuroo_world")) );
				}
				else {
					m_inWorld = false;
					setIcon( 2, QIcon::fromTheme(QStringLiteral("kuroo_empty")) );
				}
			}
		}

		setForeground( column, m_palette.foreground() );
		setTextAlignment( column, alignment );
	}
}


void PackageItem::resetDetailedInfo()
{
	m_isInitialized = false;
}

/**
* Initialize the package with all its versions and info. Executed when PortageItem get focus first time.
*/
void PackageItem::initVersions()
{
	if ( !m_isInitialized ) {
		m_versions.clear();
		m_versionMap.clear();

		// Get list of accepted keywords, eg if package is "untesting"
		QString acceptedKeywords = KurooDBSingleton::Instance()->packageKeywordsAtom( id() );
		const QStringList versionsList = KurooDBSingleton::Instance()->packageVersionsInfo( id() );
		for(QStringList::const_iterator it = versionsList.constBegin(); it != versionsList.constEnd(); ++it ) {
			QString versionString = *it++;
			QString description = *it++;
			QString homepage = *it++;
			QString status = *it++;
			QString licenses = *it++;
			QString useFlags = *it++;
			QString slot = *it++;
			QString keywords = *it++;
			QString size = *it;

			PackageVersion* version = new PackageVersion( versionString );
			version->setDescription( description );
			version->setHomepage( homepage );
			version->setLicenses( licenses.split(" ") );
			version->setUseflags( useFlags.split(" ") );
			version->setSlot( slot );
			version->setKeywords( keywords.split(" ") );
			version->setAcceptedKeywords( acceptedKeywords.split(" ") );
			version->setSize( size );

			if ( status == PACKAGE_INSTALLED_STRING )
				version->setInstalled( true );

			m_versions.append( version );
			m_versionMap.insert( versionString, version );
		}

		// Now that we have all available versions, sort out masked ones and leaving unmasked.

		// Check if any of this package versions are hardmasked
		atom = new PortageAtom( this );
		const QStringList atomHardMaskedList = KurooDBSingleton::Instance()->packageHardMaskAtom( id() );
	// 	qDebug() << "atomHardMaskedList=" << atomHardMaskedList;
		foreach( QString mask, atomHardMaskedList ) {

			// Test the atom string on validness, and fill the internal variables with the extracted atom parts,
			// and get the matching versions
			if ( atom->parse( mask ) ) {
				QList<PackageVersion*> versions = atom->matchingVersions();
				QList<PackageVersion*>::iterator versionIterator;
				for( versionIterator = versions.begin(); versionIterator != versions.end(); versionIterator++ )
					( *versionIterator )->setHardMasked( true );
			}
		}
		delete atom;

		// Check if any of this package versions are user-masked
		atom = new PortageAtom( this );
		const QStringList atomUserMaskedList = KurooDBSingleton::Instance()->packageUserMaskAtom( id() );
	// 	qDebug() << "atomUserMaskedList=" << atomUserMaskedList;
		foreach( QString mask, atomUserMaskedList ) {

			// Test the atom string on validness, and fill the internal variables with the extracted atom parts,
			// and get the matching versions
			if ( atom->parse( mask ) ) {
				QList<PackageVersion*> versions = atom->matchingVersions();
				QList<PackageVersion*>::iterator versionIterator;
				for( versionIterator = versions.begin(); versionIterator != versions.end(); versionIterator++ )
					( *versionIterator )->setUserMasked( true );
			}
		}
		delete atom;

		// Check if any of this package versions are unmasked
		atom = new PortageAtom( this );
		const QStringList atomUnmaskedList = KurooDBSingleton::Instance()->packageUnMaskAtom( id() );
	// 	qDebug() << "atomUnmaskedList=" << atomUnmaskedList;
		foreach( QString mask, atomUnmaskedList ) {

			// Test the atom string on validness, and fill the internal variables with the extracted atom parts,
			// and get the matching versions
			if ( atom->parse( mask ) ) {
				QList<PackageVersion*> versions = atom->matchingVersions();
				QList<PackageVersion*>::iterator versionIterator;
				for( versionIterator = versions.begin(); versionIterator != versions.end(); versionIterator++ )
					( *versionIterator )->setUnMasked( true );
			}
		}
		delete atom;

		// This package has collected all it's data
		m_isInitialized = true;
	}
}


/**
* Return a list of PackageVersion objects sorted by their version numbers,
* with the oldest version at the beginning and the latest version at the end
* of the list.
* @return sortedVersions
*/
QList<PackageVersion*> PackageItem::sortedVersionList()
{
	QList<PackageVersion*> sortedVersions;
	QList<PackageVersion*>::iterator sortedVersionIterator;

	for( QList<PackageVersion*>::iterator versionIterator = m_versions.begin(); versionIterator != m_versions.end(); versionIterator++ ) {
		if ( versionIterator == m_versions.begin() ) {
			sortedVersions.append( *versionIterator );
			continue; // if there is only one version, it can't be compared
		}

		// reverse iteration through the sorted version list
		sortedVersionIterator = sortedVersions.end();
		while ( true ) {
			if ( sortedVersionIterator == sortedVersions.begin() ) {
				sortedVersions.prepend( *versionIterator );
				break;
			}

			sortedVersionIterator--;
			if ( (*versionIterator)->isNewerThan( (*sortedVersionIterator)->version() ) ) {
				sortedVersionIterator++; // insert after the compared one, not before
				sortedVersions.insert( sortedVersionIterator, *versionIterator );
				break;
			}
		}
	}
	return sortedVersions;
}

/**
* Parse sorted list of versions for stability, installed, emerge versions ...
*/
void PackageItem::parsePackageVersions()
{
	if ( !m_isInitialized )
		initVersions();

	m_versionsDataList.clear();
	m_linesAvailable = QString::null;
	m_linesEmerge = QString::null;
	m_linesInstalled = QString::null;

	// Iterate sorted versions list
	QString version;
	QList<PackageVersion*> sortedVersions = sortedVersionList();
	QList<PackageVersion*>::iterator sortedVersionIterator;
	for ( sortedVersionIterator = sortedVersions.begin(); sortedVersionIterator != sortedVersions.end(); sortedVersionIterator++ ) {

		version = (*sortedVersionIterator)->version();

		// Mark official version stability for version listview
		QString stability;
		if ( (*sortedVersionIterator)->isNotArch() ) {
			stability = i18n( "Not on %1", KurooConfig::arch() );
		}
		else {
			if ( (*sortedVersionIterator)->isOriginalHardMasked() ) {
				stability = i18n( "Hardmasked" );
				version = "<font color=darkRed><i>" + version + "</i></font>";
			}
			else {
				if ( (*sortedVersionIterator)->isOriginalTesting() ) {
					stability = i18n( "Testing" );
					version = "<i>" + version + "</i>";
				}
				else {
					if ( (*sortedVersionIterator)->isAvailable() ) {
						stability = i18n( "Stable" );
					}
					else {
						stability = i18n( "Not available" );
					}
				}
			}
		}

// 		qDebug() << "version="<< (*sortedVersionIterator)->version() << " isInstalled=" << (*sortedVersionIterator)->isInstalled() <<
// 			" stability=" << stability;

		// Versions data for use by Inspector in vewrsion view
		m_versionsDataList << (*sortedVersionIterator)->version() << stability << (*sortedVersionIterator)->size();

		// Create nice summary showing installed packages
		if ( (*sortedVersionIterator)->isInstalled() ) {
			m_versionsDataList << "1";
			version = "<b>" + version + "</b>";
			m_linesInstalled.prepend( version + " (" + stability + "), " );
		}
		else
			m_versionsDataList << "0";

		// Collect all available packages except those not in users arch
		if ( (*sortedVersionIterator)->isAvailable() ) {
			m_emergeVersion = (*sortedVersionIterator)->version();
			m_linesEmerge = version + " (" + stability + ")";
			m_linesAvailable.prepend( version + ", " );
		}
		else {
			if ( (*sortedVersionIterator)->isNotArch() )
				m_isInArch = false;
			else
				m_linesAvailable.prepend( version + ", " );
		}

		// Get description and homepage from most recent version = assuming most correct
		m_description = (*sortedVersionIterator)->description();
		m_homepage = (*sortedVersionIterator)->homepage();
	}

	// Remove trailing commas
	m_linesInstalled.truncate( m_linesInstalled.length() - 2 );
	m_linesAvailable.truncate( m_linesAvailable.length() - 2 );
}


///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

void PackageItem::setRollOver( bool isMouseOver )
{
	m_isMouseOver = isMouseOver;
	//repaint();
}

/**
* Register package index in parent listView.
* @param index
*/
void PackageItem::setPackageIndex( const int& index )
{
	m_index = index;
}

void PackageItem::setDescription( const QString& description )
{
	m_description = description;
}

void PackageItem::setInstalled()
{
	m_status = PACKAGE_INSTALLED;
}

/**
* Mark package as queued. Emit signal only if status is changed.
* @param isQueued
*/
void PackageItem::setQueued( const bool& isQueued )
{
	if ( m_isQueued != isQueued ) {
		m_isQueued = isQueued;
		SignalistSingleton::Instance()->packageQueueChanged();
	}
}

/**
* Is this the first package in the listview. Since they are inserted in reverse order it means has the highest index.
* @return true if first
*/
bool PackageItem::isFirstPackage() const
{
	return ( m_index == m_parent->topLevelItemCount() );
}

/**
* Is this package installed.
* @return true if yes
*/
bool PackageItem::isInstalled() const
{
	return ( m_status & ( PACKAGE_INSTALLED | PACKAGE_UPDATES | PACKAGE_OLD ) );
}

/**
* Is this package available in Portage tree?
* @return true if yes
*/
bool PackageItem::isInPortage() const
{
	return ( m_status & ( PACKAGE_AVAILABLE | PACKAGE_INSTALLED | PACKAGE_UPDATES ) );
}

