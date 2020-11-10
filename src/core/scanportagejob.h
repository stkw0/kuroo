/***************************************************************************
 *	Copyright (C) 2005 by karye												*
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

#ifndef SCANPORTAGEJOB_H
#define SCANPORTAGEJOB_H

#include <QObject>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/JobPointer>
#include <ThreadWeaver/QObjectDecorator>
#include <ThreadWeaver/Thread>

class DbConnection;

typedef QMap<QString, QString> InstalledMap;

/**
 * @class ScanPortageJob
 * @short Thread for scanning local portage tree.
 */
class ScanPortageJob : public ThreadWeaver::QObjectDecorator
{
public:
	ScanPortageJob();
};
class ScanPortageJobImpl : public ThreadWeaver::Job
{
public:
	ScanPortageJobImpl();
	~ScanPortageJobImpl();

protected:

	void 						run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* );
private:
	void						scanInstalledPackages();
	//void 						completeJob();
	Info						scanInfo( const QString& path, const QString& category, const QString& name, const QString& version );
	QString						formatSize( const QString& size );

	void						loadCache();
	QString						cacheFind( const QString& package );

private:
	QRegExp						rxAtom;

	DbConnection* const 				m_db;

	QMap<QString, QString> 				m_mapCache;

	struct Data {
		QString					description;
		QString					homepage;
		QString					status;
		QString					licenses;
		QString					useFlags;
		QString					slot;
		QString					size;
		QString					keywords;
	};
	typedef QMap<QString, Data>		PortageVersions;
	struct Versions {
		QString					status;
		QString					description;
		QString					path;
		PortageVersions				versions;
	};
	typedef QMap<QString, Versions>		PortagePackages;
	struct Categories {
		QString					idCategory;
		QString					idSubCategory;
		PortagePackages				packages;
	};
	typedef QMap<QString, Categories>	PortageCategories;
	PortageCategories				m_categories;
};

#endif
