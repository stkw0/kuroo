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

#ifndef COMMON_H
#define COMMON_H

#include "settings.h"
#include "message.h"
#include "core/signalist.h"
#include "core/emerge.h"
#include "core/etcupdate.h"
#include "statusbar.h"
#include "logs/log.h"
#include "queue/queue.h"
#include "portage/portage.h"
#include "core/portagedb.h"
#include "history/history.h"
#include "core/portagefiles.h"
#include "core/filewatcher.h"
#include "core/global.h"
#include "singleton.h"

#include <QRegExp>

#include <KGlobal>
#include <KLocale>
#include <KDebug>

// Define all singleton objects
typedef Singleton<Signalist> SignalistSingleton;
typedef Singleton<Emerge> EmergeSingleton;
typedef Singleton<EtcUpdate> EtcUpdateSingleton;
typedef Singleton<Queue> QueueSingleton;
typedef Singleton<Portage> PortageSingleton;
typedef Singleton<KurooDB> KurooDBSingleton;
typedef Singleton<Log> LogSingleton;
typedef Singleton<History> HistorySingleton;
typedef Singleton<PortageFiles> PortageFilesSingleton;
typedef Singleton<FileWatcher> FileWatcherSingleton;

// The package status
enum PackageStatus {
		PACKAGE_DELETED = 0,
		PACKAGE_AVAILABLE = 1,
		PACKAGE_INSTALLED = 2,
		PACKAGE_UPDATES = 4,
		PACKAGE_OLD = 8
};

static const QString PACKAGE_ALL_STRING( QString::number( PACKAGE_AVAILABLE | PACKAGE_INSTALLED | PACKAGE_UPDATES | PACKAGE_OLD ) );
static const QString PACKAGE_AVAILABLE_STRING( QString::number( PACKAGE_AVAILABLE ) );
static const QString PACKAGE_INSTALLED_STRING( QString::number( PACKAGE_INSTALLED ) );
static const QString PACKAGE_OLD_STRING( QString::number( PACKAGE_OLD ) );
static const QString PACKAGE_INSTALLED_OLD_STRING( QString::number( PACKAGE_INSTALLED | PACKAGE_OLD ) );
static const QString PACKAGE_UPDATES_STRING( QString::number( PACKAGE_UPDATES ) );
static const QString PACKAGE_INSTALLED_UPDATES_OLD_STRING( QString::number( PACKAGE_INSTALLED | PACKAGE_UPDATES | PACKAGE_OLD ) );
static const QString PACKAGE_DELETED_STRING( QString::number( PACKAGE_DELETED ) );

// Log output states
enum LogActions {
		EMERGE,
		KUROO,
		ERROR,
		TOLOG,
		EMERGELOG
};

// Icons
enum Icons {
		VIEW_PORTAGE = 1,
		VIEW_QUEUE,
		VIEW_HISTORY,
		VIEW_MERGE,
		VIEW_LOG,
		PACKAGE,
		INSTALLED,
		QUEUED,
		WORLD,
		NOTQUEUED,
		UNMERGED,
		DETAILS,
		REMOVE,
		NEW,
		EMPTY,
		WARNING,
		KUROO_READY,
		KUROO_EMERGING,
		VERSION_INSTALLED,
		QUEUED_COLUMN,
		INSTALLED_COLUMN,
		WORLD_COLUMN,
		QUICKPKG
};

// The "maskedness" of a package version.
enum Stability {
		STABLE,
		TESTING,
		HARDMASKED,
		NOTAVAILABLE,
		NOTARCH
};

static const QString STABLE_STRING( QString::number( STABLE ) );
static const QString TESTING_STRING( QString::number( TESTING ) );
static const QString HARDMASKED_STRING( QString::number( HARDMASKED ) );
static const QString NOTAVAILABLE_STRING( QString::number( NOTAVAILABLE ) );

struct Info {
	QString slot;
	QString homepage;
	QString licenses;
	QString description;
	QString keywords;
	QString useFlags;
	QString size;
};

/// Announce a line
#define LINE_INFO " ( " << __FILE__ << "Line: " << __LINE__ << " )"
#define DEBUG_LINE_INFO qDebug() << LINE_INFO

#endif

