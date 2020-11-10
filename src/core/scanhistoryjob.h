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

#ifndef SCANHISTORYJOB_H
#define SCANHISTORYJOB_H

#include <QObject>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/JobPointer>
#include <ThreadWeaver/QObjectDecorator>
#include <ThreadWeaver/Thread>

class DbConnection;
class QStringList;
class PackageEmergeTime;

typedef QMap<QString, PackageEmergeTime> EmergeTimeMap;

/**
 * @class ScanHistoryJob
 * @short Thread for parsing emerge/unmerge entries found in emerge.log.
 */
class ScanHistoryJob : public ThreadWeaver::QObjectDecorator
{
public:
	ScanHistoryJob( const QStringList& logLines = QStringList() );
};
class ScanHistoryJobImpl : public ThreadWeaver::Job
{
public:
	ScanHistoryJobImpl( const QStringList& logLines = QStringList() );
	~ScanHistoryJobImpl();

private:
    void 						run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* );
	//void 						completeJob();

	QString 					escapeString( const QString& ) const;

private:
	DbConnection* const			m_db;

	// Log lines to parse
	QStringList 				m_logLines;
};

#endif
