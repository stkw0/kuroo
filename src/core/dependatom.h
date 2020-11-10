/***************************************************************************
 *   Copyright (C) 2005 by Jakob Petsovits                                 *
 *   jpetso@gmx.at                                                         *
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

#ifndef DEPENDATOM_H
#define DEPENDATOM_H

#include "packagebase.h"
#include "packageversion.h"

#include <QList>
#include <QRegExp>

/**
 * This class is the provides capabilities to parse DEPEND atoms and get
 * the appropriate matching Package objects out of a given portage tree.
 * DEPEND atoms are the strings in files like package.keywords, and are
 * also (all should we say mainly) used in ebuilds to describe dependencies.
 *
 * @class PortageAtom
 * @short  An atom parser and matching package retriever.
 */
class PortageAtom { //Atom already defined in kapp.h
public:
	PortageAtom( PackageBase* portagePackage=0 );
	PortageAtom( const QString& atom );
	~PortageAtom();

	bool parse( const QString& atom );

	QList<PackageVersion*> matchingVersions();

	/**
	 * Return true if the atom begins with a call sign ("!") which means that
	 * this package is blocking another one. This is only used inside ebuilds,
	 * where it looks, for example, like DEPEND="!app-cdr/dvdrtools".
	 * If there is no call sign, the function returns false.
	 */
	inline bool isBlocking() const { return m_callsign; }
	inline bool isValid() const { return m_matches; }
	inline const QString category() const { return m_category; }
	inline const QString package() const { return m_package; }
	
private:
	// A pointer to the portage tree from which the packages are retrieved.
	PackageBase* m_portagePackage;

	// The regular expression for the whole atom.
	QRegExp rxAtom;

	// The regular expression for just the package name and version.
	QRegExp rxVersion;

	// This is set to the result of parse().
	bool m_matches;

	// These are the extracted parts of the atom.

	// true if the callsign prefix ("blocked by this package" in ebuild dependencies) is there.
	bool m_callsign;

	// A compare sign (greater than / less than / equal) or the "all revisions" prefix ("~").
	QString m_prefix;

	// The main category of the package.
	QString m_category;

	// The package name.
	QString m_package;

	// The complete version string.
	QString m_version;
};

#endif
