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

#ifndef EMERGE_H
#define EMERGE_H

#include "package.h"

#include <QProcess>
#include <QObject>
#include <QStringList>
#include <KProcess>

/**
 * @class Emerge
 * @short The Gentoo emerge command.
 */
class Emerge : public QObject
{
Q_OBJECT
public:
	Emerge( QObject *m_parent = 0 );
	~Emerge();

	void						init( QObject *parent = 0 );
	void						inputText( const QString& text );
	bool						stop();
	bool 						isRunning() const;
	
	bool 						pretend( const QStringList& packageList );
	bool 						queue( const QStringList& packageList );
	bool 						unmerge( const QStringList& packageList );
	bool						quickpkg( const QStringList& packageList );
	bool						sync();
	bool						checkUpdates();
	/**
	 * Are we paused?
	 * @return bool
	 */
	inline bool					isPaused() const { return m_isPaused; }
	/**
	 * Can we pasue?
	 * @return bool
	 */
	inline bool					canPause() const { return m_pausable; }
	
	/**
	 * @return list of packages parsed out from emerge output.
	 */
	inline const 	EmergePackageList 		packageList() const { return m_emergePackageList; }
	const 	QString					packageMessage();
	/**
	 * Set Skip Housekeeping
	 */
	inline void					setSkipHousekeeping(const bool& x) { m_skipHousekeeping = x;}

public slots:
	void						slotPause();
	void						slotUnpause();

private:
	void						cleanup();
	bool						countEtcUpdates( const QString& line );
	void						askUnmaskPackage( const QString& packageKeyword );
	/**
	 * Do we Skip housekeeping?
	 * @return bool
	 */
	inline bool					skipHousekeeping() const { return m_skipHousekeeping; }

private slots:
    void 						slotEmergeOutput();
    void 						slotCleanupQueue(int, QProcess::ExitStatus);
    void 						slotCleanupPretend(int, QProcess::ExitStatus);
    void 						slotCleanupUnmerge(int, QProcess::ExitStatus);
    void 						slotCleanupSync(int, QProcess::ExitStatus);
    void 						slotCleanupCheckUpdates(int, QProcess::ExitStatus);
	void						slotTryEmerge();
    void						slotBackupComplete(int, QProcess::ExitStatus);
    void						slotEmergeDistfilesComplete();
    void						slotEClean2Complete(int, QProcess::ExitStatus);
    void						slotRevdepRebuildComplete();

	
signals:
	void						signalEmergeComplete();
	
private:
	QObject* m_parent;
	KProcess* eProc;
	KProcess* eClean1;
	KProcess* eClean2;
	KProcess* ioRevdepRebuild;
        
	// Used to collect ewarn and einfo messages spaning multiple lines
	bool						m_completedFlag;

	// Used to track a quickpkg backup
	bool						m_backupComplete;
	bool						m_backingUp;

	// Can we pause this eProc?
	bool						m_pausable;
	bool						m_isPaused;

	// should we be ecleaning?
	bool						m_doeclean;
        bool                                            m_dorevdeprebuild;
	
	// Package with the important message
	QString						m_importantMessagePackage;
	
	// Collects messages from emerge, like masked errors, ewarn and einfos
	QString 					m_importantMessage;
	
	// The current package
	QString						m_packageMessage;
	
	// The parsed package emerge says need unmasking
	QString						m_unmasked;
	
	// Collect all blocking packages
	QStringList 				m_blocks;
	
	// Remember packages emerge started with, used when auto-unmasking
	QStringList					m_lastEmergeList;
	
	// List of parsed packages
	EmergePackageList			m_emergePackageList;
	
	//Remembers the beginning of lines before they are fully read.
	QByteArray				m_buffer;
	
	// Count of etc-updates files to merge
	int							m_etcUpdateCount;
        
        bool                                            m_skipHousekeeping;
	void cleanupQueue();
};

#endif
