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

#include "common.h"

#include <qapplication.h>

#include <kcursor.h>

#include <assert.h>
#include <QThread>

/**
 * @class Signalist
 * @short Object which forwards signals, so they can picked up systemwide.
 * Just connect to this instance.
 */
Signalist::Signalist( QObject* m_parent )
	: QObject( m_parent ), m_busy( false ), m_isReady( false )
{
	connect(this, &Signalist::signalCursor, this, &Signalist::slotCursor);
}

Signalist::~Signalist()
{}

void Signalist::init( QObject* parent )
{
	m_parent = parent;
}

/**
 * Sanity level. No db means not ready.
 * @param isReady
 */
void Signalist::setKurooReady( const bool& isReady )
{
	m_isReady = isReady;
	emit signalKurooBusy( !isReady );
}


/**
 * Toggle busy flag for kuroo.
 * @param busy
 */
void Signalist::setKurooBusy( const bool& busy )
{
	static int busySession(0);

	if ( !busy ) {
		if ( busySession > 0 ) {
			busySession--;
			emit signalCursor(true);
		}
	}
	else {
		busySession++;
		emit signalCursor(false);
	}

	if ( busySession == 0 ) {
		m_busy = false;
		emit signalKurooBusy( false );
	}
	else {
		m_busy = true;
		emit signalKurooBusy( true );
	}
}

/**
 * This slot is called by setKurooBusy() to enable or disable the busy cursor.
 * This is separated from setKurooBusy() so that threads don't execute code inside the GUI thread
 * 
 * @param restore if True, restores the previous cursor. Set the busy cursor if false. \
 * 	  (See QApplication::setOverrideCursor() and QApplication::restoreOverrideCursor() documentation)
 */
void Signalist::slotCursor(bool restore)
{
	assert(QThread::currentThread() == qApp->thread());
	if (restore)
		QApplication::restoreOverrideCursor();
	else
		QApplication::setOverrideCursor( Qt::BusyCursor );
}

/**
 * Kuroo is done syncing.
 */
void Signalist::syncDone()
{
	emit signalSyncDone();
}

/**
 * Kuroo is busy scanning.
 */
void Signalist::scanStarted()
{
	setKurooBusy( true );
	emit signalScanStarted();
}

/**
 * Kuroo is busy scanning portage.
 */
void Signalist::scanPortageStarted()
{
	setKurooBusy( true );
	emit signalScanPortageStarted();
}

/**
 * Kuroo is busy scanning updates.
 */
void Signalist::scanUpdatesStarted()
{
	setKurooBusy( true );
	emit signalScanUpdatesStarted();
}

void Signalist::scanProgress(int steps)
{
	emit signalScanProgress(steps);
}

/**
 * Portage scan thread completed.
 */
void Signalist::cachePortageComplete()
{
	emit signalCachePortageComplete();
	setKurooBusy( false );
}

/**
 * Portage scan thread completed.
 */
void Signalist::scanPortageComplete()
{
	emit signalScanPortageComplete();
	setKurooBusy( false );
}

/**
 * "emerge -upv" completed.
 */
void Signalist::scanUpdatesComplete()
{
	emit signalScanUpdatesComplete();
	setKurooBusy( false );
}

/**
 * Db is updated with the new updates list.
 */
void Signalist::loadUpdatesComplete()
{
	emit signalLoadUpdatesComplete();
	setKurooBusy( false );
}

/**
 * Db is updated with the new history entries.
 */
void Signalist::scanHistoryComplete()
{
	emit signalScanHistoryComplete();
	setKurooBusy( false );
}

void Signalist::packageQueueChanged()
{
	emit signalPackageQueueChanged();
}

void Signalist::packageClicked( const QString& package )
{
	emit signalPackageClicked( package );
}

void Signalist::fontChanged()
{
	emit signalFontChanged();
}

void Signalist::emergeAborted()
{
	emit signalEmergeAborted();
}

