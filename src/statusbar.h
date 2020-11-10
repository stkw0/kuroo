/***************************************************************************
 *	Copyright (C) 2005 by Karye												*
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

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QMap>
#include <QProgressBar>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <QStatusBar>

#include "signalist.h"

/**
 * @class KurooStatusBar
 * @short Singleton statusbar with label and progressbar.
 */
class KurooStatusBar : public QStatusBar
{
Q_OBJECT
	static KurooStatusBar* s_instance;

public:
	KurooStatusBar( QWidget *parent = 0 );
	~KurooStatusBar();

	static 					KurooStatusBar* instance() { return s_instance; }

	void 					setProgressStatus( const QString& id, const QString& text );

	void					setTotalSteps( int total );
	void					updateTotalSteps( int total );

	void					startTimer();
	void					stopTimer();
	void					pauseTimers();
	void					unpauseTimers();
	long					elapsedTime();
	void					clearElapsedTime();

	void					startProgress();

	void					setThreadTotalSteps( int total );

public slots:
	void					slotOneStep();
	void					slotAdvance();
	void 					setProgress(int);

private slots:
	void					slotLastMessage();
	void					slotUpdateTime();
	void					slotScanStarted();
	void					slotScanPortageStarted();
	void					slotScanUpdatesStarted();
	void					slotScanPortageComplete();
	void					slotScanUpdatesComplete();

private:
	QMap<QString, QString> 	m_messageMap;
	QProgressBar 			*statusBarProgress;
	QTimer 					*m_internalTimer, *m_diffTimer;
	long					m_timerSteps;
};

#endif
