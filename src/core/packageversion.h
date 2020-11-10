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

#ifndef PACKAGEVERSION_H
#define PACKAGEVERSION_H

#include <qobject.h>
#include <qstringlist.h>
#include <qregexp.h>

/**
* A PackageVersion specializing in Portage.
* It features accurate version comparison just like Portage itself does,
* and can store additional values from Portage that the original
* PackageVersion does not provide.
*/
class PackageVersion
{
public:
	friend class PackageListItem;

	bool isAvailable() const;
	bool isNotArch() const;
	
	bool isNewerThan( const QString& otherVersion ) const;
	bool isOlderThan( const QString& otherVersion ) const;
	
	int stability( const QString& arch ) const;
	
	bool isInstalled() const;
	bool isOverlay() const;
	const QString& version() const;
	const QString& date() const;
	const QString& description() const;
	const QString& homepage() const;
	const QString& slot() const;
	const QStringList& licenses() const;
	const QStringList& keywords() const;
	const QStringList& useflags() const;
	QStringList& acceptedKeywords();
	QString size() const;
	bool hasDetailedInfo() const;
	
	bool isHardMasked() const;
	bool isUserMasked() const;
	bool isUnMasked() const;
	bool isOriginalHardMasked() const;
	bool isOriginalTesting() const;
	
	void setVersion( const QString& version );
	void setInstalled( bool isInstalled );
	void setOverlay( bool isOverlay );
	void setDate( const QString& date );
	void setDescription( const QString& description );
	void setHomepage( const QString& homepage );
	void setSlot( const QString& slot );
	void setLicenses( const QStringList& licenses );
	void setKeywords( const QStringList& keywords );
	void setUseflags( const QStringList& useflags );
	void setAcceptedKeywords( const QStringList& acceptedKeywords );
	void setSize( const QString& size );
	
	void setHardMasked( bool isHardMasked );
	void setUserMasked( bool isUserMasked );
	void setUnMasked( bool isUnMasked );
	
protected:
	PackageVersion( const QString& version );
	virtual ~PackageVersion();
	
private:
	int revisionNumber( const QString& versionString, int* foundPos = NULL ) const;
	long suffixNumber( const QString& versionString, int* foundPos = NULL ) const;
	int trailingCharNumber( const QString& versionString, int* foundPos = NULL ) const;
	
	/** The package containing this version. */
	//PackageListItem* m_package;
	
	// Info retrievable by retrieving QFileInfos for ebuilds
	// (without parsing their contents):
	
	QString m_version;
	/** Date of the ebuild file's last modification. */
	QString m_date;
	/** true if the package is installed, false otherwise. */
	bool m_installed;
	/** true if the package is from the overlay tree, false otherwise. */
	bool m_overlay;
	
	
	// Info retrievable by scanning and parsing the contents of an ebuild file:
	
	/** A short line describing the package. */
	QString m_description;
	/** URL of the package's home page. */
	QString m_homepage;
	/** The slot for this version. Mostly a number, but only has to be interpreted as string. */
	QString m_slot;
	/** List of licenses that are used in the package. */
	QStringList m_licenses;
	/** List of keywords, like x86 or ~alpha. */
	QStringList m_keywords;
	/** List of use flags that influence compilation of the package. */
	QStringList m_useflags;
	/** A list of additionally accepted keywords for this specific package. */
	QStringList m_acceptedKeywords;
	
	// Info that's not in the ebuild:
	
	/** Downloaded file size in bytes (retrievable by scanning the digest). */
	QString m_size;
	/** true if this version is hardmasked, false otherwise.
	* Retrievable by scanning package.[un]mask and Co. */
	bool m_isHardMasked;
	
	QRegExp rxNumber, rxRevision, rxSuffix, rxTrailingChar;
	
	/** Official Gentoo hardmasked state */
	bool m_isOriginalHardMasked;
	
	bool m_isUserMasked;
	
	bool m_isUnMasked;
};

#endif

