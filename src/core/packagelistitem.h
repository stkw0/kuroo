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

#ifndef PACKAGE_LIST_ITEM
#define PACKAGE_LIST_ITEM

#include <QObject>
#include <QString>
#include <QMap>

#include "common.h"
#include "dependatom.h"
#include "packagebase.h"
#include "packageversion.h"
#include "portagelistview.h"

class PackageListItem : public QObject, public PackageBase
{
	Q_OBJECT
public:
	PackageListItem(QObject *parent = 0);
	PackageListItem(const QString& name, const QString& id, const QString& category, const QString& description, const int status, const QString& update, QObject *parent = 0);

	~PackageListItem();

	QString						id() const {return m_id;}
	QString						description() const {return m_description;}
	QString						update() const {return m_update;}
	QString						homepage() const {return m_homepage;}
	int							status() const {return m_status;}
	bool						isInWorld() const {return m_isInWorld;}
	bool						isQueued() const {return QueueSingleton::Instance()->isQueued(m_id);}
	inline const QString&		linesInstalled() const { return m_linesInstalled; }
	inline const QString&		linesAvailable() const { return m_linesAvailable; }
	inline const QString&		linesEmerge() const { return m_linesEmerge; }
	inline bool					isInArch() const { return m_isInArch; }
	inline const QStringList&	versionDataList() const { return m_versionsDataList; }
	inline const QString&		emergeVersion() const { return m_emergeVersion; }
	bool						isLastPackage() const  { return (m_index == 1); }
	bool						isFirstPackage() const;
	inline QMap<QString, PackageVersion*> versionMap() const { return m_versionMap; }

	void						initVersions();
	QList<PackageVersion*>		sortedVersionList();
	void						parsePackageVersions();
	void						setQueued(const bool);
	bool						isInPortage() const;
	bool						isInstalled() const;

	void						resetDetailedInfo();

	void						setPackageIndex(const int idx);
	int							packageIndex() const {return m_index;}

private:
	QString						m_id;
	QString						m_description;
	QString						m_update;
	bool						m_isInWorld;
	int							m_status;
	int							m_index;

	// True if package and its versions has been initialized with all data
	bool						m_isInitialized;

	// Alternatively map with all versions and their data
	QMap<QString, PackageVersion*> m_versionMap;

	// Atom object needed for versions stability
	PortageAtom*				atom;

	// Formatted string
	QString						m_linesInstalled;
	QString						m_linesAvailable;
	QString						m_linesEmerge;

	// Version used by emerge
	QString						m_emergeVersion;

	// Latest versions homepage supposed to be most current
	QString						m_homepage;

	// Versions list together with stability info etc...
	QStringList					m_versionsDataList;

	// Is this package available in this arch?
	bool						m_isInArch;
};

#endif
