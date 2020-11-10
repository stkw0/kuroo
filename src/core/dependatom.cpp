/***************************************************************************
 *   Copyright (C) 2005 by Jakob Petsovits                                 *
 *   jpetso@gmx.at                                                         *
 *   Copyright (C) 2005 by Karye                                           *
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
#include "dependatom.h"
#include "packageversion.h"
#include "packagebase.h"
#include <QList>

//CLEAN: rename files to atom

// For more info on DEPEND atoms, see the DEPEND Atoms section of man 5 ebuild

// The complete atom regexp in non-escaped form (for testing, or similar):
// ^(!)?(~|(?:<|>|=|<=|>=))?((?:[a-z]|[0-9])+)-((?:[a-z]|[0-9])+)/((?:[a-z]|[A-Z]|[0-9]|-|\+|_)+)((?:\*$|-\d+(?:\.\d+)*[a-z]?(?:\*$)?)(?:_(?:alpha|beta|pre|rc|p)\d+)?(?:-r\d+)?)?$

/**
 * @class PortageAtom
 * @short Class that parse and compare depend atoms.
 * Initialize this object.
 * @param packages  The package that will be filtered out.
 */
PortageAtom::PortageAtom( PackageBase* portagePackage )
	: m_portagePackage( portagePackage ),
	rxAtom(
			"^"    										// Start of the string
			"(!)?" 										// "Block these packages" flag, only occurring in ebuilds
			"(~|(?:<|>|=|<=|>=))?" 						// greater-than/less-than/equal, or "all revisions" prefix
			"((?:[a-z]|[0-9])+)-((?:[a-z]|[0-9])+|virtual)/"   	// category and subcategory FIXME:What about 'virtual' category ?
			"((?:[a-z]|[A-Z]|[0-9]|-|\\+|_)+)" 			// package name
			//FIXME:this will take -9999 version in the package name. See /usr/lib/portage/pym/portage/versions.py that has a better regexp.
			"("           								// start of the version part
			"(?:-\\d*(?:\\.\\d+)*[a-z]?)" 				// base version number, including wildcard version matching (*)
			"(?:_(?:alpha|beta|pre|rc|p)\\d*)?" 		// version suffix
			"(?:-r\\d*)?"  								// revision
			"\\*?)?"          							// end of the (optional) version part and the atom string
			"(?::.*)?"									// slot
			"(?:\\w*#.*)?$"									// line comment
		),
	rxVersion(
		"^"
		"((?:[a-z]|[A-Z]|[0-9]|-|\\+|_)+)" 			// package name
		"("           								// start of the version part
		"(?:-\\d*(?:\\.\\d+)*[a-z]?)" 				// base version number, including wildcard version matching (*)
		"(?:_(?:alpha|beta|pre|rc|p)\\d*)?" 		// version suffix
		"(?:-r\\d*)?"  								// revision
		"\\*?)$"          							// end of the (optional) version part and the atom string
	), m_matches( false ), m_callsign( false ), m_category( QString::null )
{
	rxAtom.setMinimal(true);	//Versions without a . in them were greedily matched into the package name
}

PortageAtom::PortageAtom( const QString& atom )
	: rxAtom(
			"^"											// Start of the string
			"(!)?"										// "Block these packages" flag, only occurring in ebuilds
			"(~|(?:<|>|=|<=|>=))?"						// greater-than/less-than/equal, or "all revisions" prefix
			"((?:[a-z]|[0-9])+)-((?:[a-z]|[0-9])+|virtual)/"	// category and subcategory
			"((?:[a-z]|[A-Z]|[0-9]|-|\\+|_)+)"			// package name
			"("											// start of the version part
			"(?:-\\d*(?:\\.\\d+)*[a-z]?)"				// base version number, including wildcard version matching (*)
			"(?:_(?:alpha|beta|pre|rc|p)\\d*)?"			// version suffix
			"(?:-r\\d*)?"								// revision
			"\\*?)?"									// end of the (optional) version part and the atom string
			"(?::.*)?"									// slot
			"(?:\\w*#.*)?$"									// line comment
		),
	rxVersion(
		"^"
		"((?:[a-z]|[A-Z]|[0-9]|-|\\+|_)+)" 			// package name
		"("           								// start of the version part
		"(?:-\\d*(?:\\.\\d+)*[a-z]?)" 				// base version number, including wildcard version matching (*)
		"(?:_(?:alpha|beta|pre|rc|p)\\d*)?" 		// version suffix
		"(?:-r\\d*)?"  								// revision
		"\\*?)$"          							// end of the (optional) version part and the atom string
	), m_matches( false ), m_callsign( false ), m_category( QString::null )
{
	PortageAtom();
	rxAtom.setMinimal(true);	//Versions without a . in them were greedily matched into the package name
	parse( atom );
}

PortageAtom::~PortageAtom()
{
}

/**
 * Test the atom string on validness, and fill the internal variables with
 * the extracted atom parts like category, package name and version. If the
 * atom is valid, you can afterwards call matchingPackages() and isBlocking().
 * If it's invalid, the result of the these functions is undefined.
 * So, make sure to check the return value.
 *
 * @param atom  The atom string that should be parsed. This string is expected
 *              not to have leading or trailing whitespaces, otherwise it
 *              will not be considered valid in any case.
 * @return  true if the atom is valid, false otherwise.
 */
bool PortageAtom::parse( const QString& atom )
{
// 	qDebug() << "atom=" << atom;

	// Do the regexp match, which also prepares for text capture
	if ( !rxAtom.exactMatch( atom ) ) {
		qDebug() << atom << " didn't match the regex exactly.";
		m_matches = false;
		return false;
	}

	// Get the captured strings
	m_callsign	= rxAtom.cap( 1 ).isEmpty() ? false : true;
	//qDebug() << m_callsign;
	m_prefix	= rxAtom.cap( 2 );
	//qDebug() << m_prefix;
	m_category	= rxAtom.cap( 3 ) + "-" + rxAtom.cap( 4 );
	//qDebug() << m_category;
	m_package	= rxAtom.cap( 5 );
	//qDebug() << m_package;
	m_version	= rxAtom.cap( 6 );
	//qDebug() << m_version;

	// Additional check: If there is a version, there also must be a prefix
	if ( m_version.isEmpty() != m_prefix.isEmpty() ) {
		//Try to fix it with the second regex
		if ( rxVersion.exactMatch( m_package ) ) {
			m_package = rxVersion.cap( 1 );
			m_version = rxVersion.cap( 2 );
		}
	}

	// Additional check: If there is a version, there also must be a prefix
	if ( m_version.isEmpty() != m_prefix.isEmpty() ) {
		qDebug() << atom << " has prefix, category, pagkage, and version " << m_prefix << m_category << m_package << m_version;
		m_matches = false;
		return false;
	}

	// Strip the hyphen at the start of the version, except when it's a "*"
	if ( !m_version.isEmpty() && m_version[0] == '-' )
		m_version = m_version.mid(1);

	// Not yet returned false, so it's a valid atom
	m_matches = true;
	return true;
}

/**
 * Retrieve the set of package versions that is matching the atom.
 * The searched packages are the ones from the package list.
 * If no matching package versions are found, an empty list is returned.
 */
QList<PackageVersion*> PortageAtom::matchingVersions()
{
	QList<PackageVersion*> matchingVersions;

	if ( m_package.isEmpty() || !m_matches )
		return matchingVersions; // return an empty list

	if ( m_portagePackage->category() != m_category || m_portagePackage->name() != m_package )
		return matchingVersions; // return an empty list

	bool matchAllVersions( false );
	if ( m_version.isEmpty() || m_version == "*" )
		matchAllVersions = true;

	bool matchBaseVersion( false );
	if ( m_version.endsWith("*") ) {

		// remove the trailing star
		m_version = m_version.left( m_version.length() - 1 );
		matchBaseVersion = true;
	}

	// When this is set true, it will match all versions
	// with exactly the same version string as the parsed one.
	bool matchEqual = m_prefix.endsWith("=");

	// When this is set true, it will match all versions greater than
	// (but not equalling) the parsed version from the atom.
	// When false, it will match all versions less than
	// (but not equalling, too) the parsed version.
	bool matchGreaterThan = m_prefix.startsWith(">");

	// Match '~' prefix for this exact version and all its revisions
	bool matchAllRevisions( false );
	if ( m_prefix.startsWith("~") ) {
		matchEqual = true;
		matchAllRevisions = true;
	}

	QList<PackageVersion*> versions = m_portagePackage->versionList();

// 	qDebug() << "PortageAtom::matchingVersions matchBaseVersion=" << matchBaseVersion << " matchEqual=" << matchEqual << " matchGreaterThan=" << matchGreaterThan;

	// So, let's iterate through the versions to check if they match or not
	for ( QList<PackageVersion*>::iterator versionIterator = versions.begin(); versionIterator != versions.end(); versionIterator++ ) {

// 		qDebug() << "PortageAtom::matchingVersions m_version=" << m_version << " version=" << (*versionIterator)->version() <<
// 			"       (*versionIterator)->isNewerThan( m_version )=" << (*versionIterator)->isNewerThan( m_version ) << endl;

		if ( ( matchAllVersions ) ||
			( matchBaseVersion   && (*versionIterator)->version().startsWith( m_version ) ) ||
			( matchEqual         && (*versionIterator)->version() == m_version ) ||
			( matchAllRevisions  && (*versionIterator)->version().startsWith( m_version + "-r" ) ) ||
			( matchGreaterThan   && (*versionIterator)->isNewerThan( m_version ) ) ||
			( !matchEqual        && !matchGreaterThan && (*versionIterator)->isOlderThan( m_version ) ) )
		{
			matchingVersions.append( (PackageVersion*) *versionIterator );
			continue;
		}
	}

	return matchingVersions;

} // end of matchingVersions()
