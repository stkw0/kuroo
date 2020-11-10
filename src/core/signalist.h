/***************************************************************************
 *   Copyright (C) 2005 by Karye   *
 *   karye@users.sourceforge.net   *
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

#ifndef SIGNALIST_H
#define SIGNALIST_H

#include <qobject.h>

/**
 * @class Signalist
 * @short Object which forwards signals, so they can picked up systemwide.
 * Just connect to this instance.
 */
class Signalist : public QObject
{
Q_OBJECT
public:
	Signalist( QObject *m_parent = 0 );
	~Signalist();

	void		init( QObject *parent = 0 );
	void		setKurooReady( const bool& isReady );
	/**
	 * Return kuroo ready state.
	 */
	inline bool	isKurooReady() const { return m_isReady; }
	void		setKurooBusy( const bool& busy );
	/**
	 * Job wasn't successful.
	 */
	inline void	scanAborted() { setKurooBusy( false ); }
	/**
	 * Kuroo is busy while scanning for packages or emerging.
	 * @return busy
	 */
	inline bool	isKurooBusy() const { return m_busy; }
	void		syncDone();
	void		cachePortageComplete();
	void 		scanPortageComplete();
	void		scanUpdatesComplete();
	void		loadUpdatesComplete();
	void		scanHistoryComplete();
	void		packageQueueChanged();
	void		packageClicked( const QString& package );
	void		fontChanged();
	void		scanProgress(int);
	void		scanPortageStarted();
	void		scanUpdatesStarted();
	void		scanStarted();
	void		emergeAborted();

private slots:
	void 		slotCursor(bool);
	
signals:
	void		signalKurooBusy( bool b );
	void		signalCachePortageComplete();
	void 		signalScanPortageComplete();
	void 		signalScanInstalledComplete();
	void 		signalScanUpdatesComplete();
	void 		signalLoadUpdatesComplete();
	void 		signalEmergeQueue();
	void		signalSyncDone();
	void 		signalScanHistoryComplete();
	void		signalPackageQueueChanged();
	void		signalPackageClicked( const QString& package );
	void		signalFontChanged();
	void		signalScanPortageStarted();
	void		signalScanUpdatesStarted();
	void		signalScanStarted();
	void		signalScanProgress(int);
	void 		signalCursor(bool);
	void		signalEmergeAborted();
	
private:
	QObject*	m_parent;
	bool		m_busy, m_isReady;
};

#endif
