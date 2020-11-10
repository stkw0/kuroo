/***************************************************************************
 *	Copyright (C) 2010 by cazou88											*
 *	cazou88@users.sourceforge.net											*
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
#include "packagebase.h"
#include "packagelistitem.h"
#include "queuelistview.h"
#include "packageversion.h"

PackageListItem::PackageListItem(QObject *parent)
 : QObject(parent), PackageBase(),
   m_id(""),
   m_description(""),
   m_update(""),
   m_status(0),
   m_isInitialized(false)
{}

PackageListItem::PackageListItem(const QString& name, const QString& id, const QString& category, const QString& description, const int status, const QString& update, QObject *parent)
 : QObject(parent), PackageBase(name, category),
   m_id(id),
   m_description(description),
   m_update(update),
   m_status(status),
   m_isInitialized(false)
{
	if (PortageSingleton::Instance()->isInWorld(m_category + "/" + m_name))
		m_isInWorld = true;
	else
		m_isInWorld = false;
}

PackageListItem::~PackageListItem()
{
	//qDebug() << "Destroying PackageListItem " << m_name;
}

/**
 * Initialize the package with all its versions and info. Executed when PortageItem get focus first time.
 */
void PackageListItem::initVersions()
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
QList<PackageVersion*> PackageListItem::sortedVersionList()
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
void PackageListItem::parsePackageVersions()
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
		else {
			m_versionsDataList << "0";
		}

		// Collect all available packages except those not in users arch
		if ( (*sortedVersionIterator)->isAvailable() ) {
			m_emergeVersion = (*sortedVersionIterator)->version();
			m_linesEmerge = version + " (" + stability + ")";
			m_linesAvailable.prepend( version + ", " );
		}
		else {
			if ( (*sortedVersionIterator)->isNotArch() ) {
				m_isInArch = false;
			}
			else {
				m_linesAvailable.prepend( version + ", " );
			}
		}

		// Get description and homepage from most recent version = assuming most correct
		m_description = (*sortedVersionIterator)->description();
		m_homepage = (*sortedVersionIterator)->homepage();
	}

	// Remove trailing commas
	m_linesInstalled.truncate( m_linesInstalled.length() - 2 );
	m_linesAvailable.truncate( m_linesAvailable.length() - 2 );

}

/**
 * Is this package installed.
 * @return true if yes
 */
bool PackageListItem::isInstalled() const
{
	return ( m_status & ( PACKAGE_INSTALLED | PACKAGE_UPDATES | PACKAGE_OLD ) );
}

/**
 * Is this package available in Portage tree?
 * @return true if yes
 */
bool PackageListItem::isInPortage() const
{
	return ( m_status & ( PACKAGE_AVAILABLE | PACKAGE_INSTALLED | PACKAGE_UPDATES ) );
}

void PackageListItem::resetDetailedInfo()
{
	m_isInitialized = false;
}

void PackageListItem::setPackageIndex(const int idx)
{
	m_index = idx;
}

bool PackageListItem::isFirstPackage() const
{
	PortageListView* plv = dynamic_cast<PortageListView*>(parent());
	QueueListView* qlv = dynamic_cast<QueueListView*>(parent());

	if (plv)
		return (m_index == plv->packages().count());

	if (qlv)
		return (m_index == qlv->allPackages().count());

	return true;
}
