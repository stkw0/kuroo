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

#ifndef HISTORY_H
#define HISTORY_H

#include <QObject>
#include <QStringList>
#include <QFile>
#include <QVector>
#include <QTextStream>

class PackageEmergeTime;

struct eLog {
	QString package;
	uint	timestamp;
};

typedef QMap<QString, PackageEmergeTime> EmergeTimeMap;
typedef QVector<eLog> eLogVector;

/**
 * @class History
 * @short Object for the emerge history and statistics.
 */
class History : public QObject
{
Q_OBJECT
public:
	History( QObject *m_parent = 0 );
    ~History();

	void					init( QObject *parent = 0 );
	void					loadTimeStatistics();
	const EmergeTimeMap 	getStatisticsMap();
	void 					setStatisticsMap( const EmergeTimeMap& statisticsMap );
	const QString 			packageTime( const QString& packageNoversion );
	void					appendEmergeInfo();
	void					updateStatistics();
	const QStringList		allMergeHistory();
	void					scanELog();
	eLogVector				getELogs();

public slots:
	void					slotInit();
	void					slotScanHistoryCompleted();
	bool					slotRefresh();

private slots:
	void					slotScanHistory( const QStringList& lines );
	void					slotParse();
	void					slotWeaverDone(ThreadWeaver::JobPointer);

private:
	QObject*				m_parent;

	// Watches the emerge.log file
	KDirWatch				*logWatcher;

	// Map with packages emerge duration
	EmergeTimeMap			m_statisticsMap;

	// The emerge.log
	QFile 					m_log;

	// Stream for emerge.log
	QTextStream 			stream;

	// Is kuroo emerging or just downloading the package
	bool					isEmerging;

	// To keep track of sync time
	QTime					m_syncTime;

	eLogVector				m_eLogs;

signals:
	void					signalScanHistoryCompleted();
	void					signalHistoryChanged();
};

#endif
