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

#include "common.h"
#include "statusbar.h"
#include "signalist.h"

#include <QMap>
#include <QProgressBar>
#include <QString>
#include <QTimer>
#include <QWidget>

KurooStatusBar* KurooStatusBar::s_instance = 0;

/**
* @class KurooStatusBar
* @short Singleton object to build statusbar with label and progressbar.
*
* The progressbar is either updated by a timer (eg when emerging packages) or by updated by count (eg when scanning packages).
* The progressbar is hidden when inactive.
* The messages in the label can be constant or transient. The label displays transient messages for 2 sec then returns to
* last constant message if any. The message stack is fifo.
*/
KurooStatusBar::KurooStatusBar( QWidget *parent )
	: QStatusBar( parent ), statusBarProgress( 0 )//, statusBarLabel( 0 )
{
	s_instance = this;

	statusBarProgress = new QProgressBar( this );

	addPermanentWidget( statusBarProgress, 1 );
	statusBarProgress->setAlignment( Qt::AlignRight );
	statusBarProgress->setFixedWidth( 200 );

	statusBarProgress->setMaximum( 100 );
	statusBarProgress->hide();

	// Clock timer for showing progress when emerging packages.
	m_internalTimer = new QTimer( this );
	connect(m_internalTimer, &QTimer::timeout, this, &KurooStatusBar::slotUpdateTime);

	// Progress timer for activities when total duration is not specified.
	m_diffTimer = new QTimer( this );
	connect(m_diffTimer, &QTimer::timeout, this, &KurooStatusBar::slotAdvance);

	connect(SignalistSingleton::Instance(), SIGNAL(signalScanPortageStarted()), this, SLOT(slotScanPortageStarted()));
	connect(SignalistSingleton::Instance(), SIGNAL(signalScanUpdatesStarted()), this, SLOT(slotScanUpdatesStarted()));
	//connect(SignalistSingleton::Instance(), SIGNAL(signalScanProgress(int)), this, SLOT(slotAdvance()));
	connect(SignalistSingleton::Instance(), SIGNAL(signalScanPortageComplete()), this, SLOT(slotScanPortageComplete()));
	connect(SignalistSingleton::Instance(), SIGNAL(signalScanUpdatesComplete()), this, SLOT(slotScanUpdatesComplete()));
}

KurooStatusBar::~KurooStatusBar()
{}

void KurooStatusBar::slotScanPortageStarted()
{
	qDebug() << "PORTAGE";
	setProgressStatus( "ScanPortage", i18n( "Refreshing Portage packages view..." ) );
	startProgress();
}

void KurooStatusBar::slotScanUpdatesStarted()
{
	qDebug() << "UPDATES";
	setProgressStatus( "Updates", i18n( "Checking for package updates..." ) );
	startProgress();
}

void KurooStatusBar::slotScanStarted()
{
}

void KurooStatusBar::slotScanPortageComplete()
{
	setProgressStatus("ScanPortage", i18n( "Done." ));
	setTotalSteps(0);
}

void KurooStatusBar::slotScanUpdatesComplete()
{
	setProgressStatus("Updates", i18n( "Done." ));
	setTotalSteps(0);
}

/**
* Set label text in statusbar.
*/
void KurooStatusBar::setProgressStatus( const QString& id, const QString& message )
{
	if ( id.isEmpty() ) {
		showMessage( message, 2000 );
		QTimer::singleShot( 2000, this, SLOT( slotLastMessage() ) );
		return;
	}

	if ( !m_messageMap.contains( id ) ) {
		m_messageMap.insert( id, message );
		showMessage( message );
	}
	else {
		m_messageMap.remove( id );
		showMessage( message, 2000 );
		QTimer::singleShot( 2000, this, SLOT( slotLastMessage() ) );
	}
}

/**
* View last message.
*/
void KurooStatusBar::slotLastMessage()
{
	QMap<QString, QString>::Iterator it = m_messageMap.end();
	if ( m_messageMap.size() > 0 ) {
		it--;
		showMessage( it.value() );
	} else {
		showMessage( i18n("Done.") );
	}
}

/**
* Set total for timer progress.
*/
void KurooStatusBar::setTotalSteps( int total )
{
	qDebug() << "total=" << total;

	stopTimer();
	statusBarProgress->setTextVisible( true );
	statusBarProgress->setMaximum( total );

	if ( total == 0 )
		statusBarProgress->hide();
	else
		if ( !statusBarProgress->isVisible() ) {
			statusBarProgress->show();
			m_internalTimer->start( 1000 );
			startTimer();
		}
}

void KurooStatusBar::updateTotalSteps( int total )
{
	statusBarProgress->setMaximum( m_timerSteps + total );
	m_diffTimer->stop();
	statusBarProgress->setTextVisible( true );
	disconnect(m_internalTimer, &QTimer::timeout, this, &KurooStatusBar::slotOneStep);
	connect(m_internalTimer, &QTimer::timeout, this, &KurooStatusBar::slotOneStep);
}

/**
* Set total for stepped progress.
*/
void KurooStatusBar::setThreadTotalSteps( int total )
{
	statusBarProgress->setTextVisible( true );
	statusBarProgress->setMaximum( total );

	if ( total == 0 )
		statusBarProgress->hide();
	else
		if ( !statusBarProgress->isVisible() )
			statusBarProgress->show();
}

/**
* View progress.
* @param steps		in %
*/
void KurooStatusBar::setProgress( int steps )
{
	statusBarProgress->setValue( steps );
}

/**
* Launch internal timer used when emerging packages.
*/
void KurooStatusBar::startTimer()
{
	connect(m_internalTimer, &QTimer::timeout, this, &KurooStatusBar::slotOneStep);
	m_timerSteps = 0;
}

/**
* Stop internal timer.
*/
void KurooStatusBar::stopTimer()
{
	disconnect(m_internalTimer, &QTimer::timeout, this, &KurooStatusBar::slotOneStep);
	m_diffTimer->stop();
	statusBarProgress->setValue( 0 );
	statusBarProgress->setMaximum( 100 );
	statusBarProgress->setTextVisible( true );
	statusBarProgress->hide();
	DEBUG_LINE_INFO;
}

/**
* Increase progress by 1 second.
*/
void KurooStatusBar::slotOneStep()
{
	setProgress( m_timerSteps );
	if ( m_timerSteps > statusBarProgress->maximum() ) {
		stopTimer();
		startProgress();
	}
}

void KurooStatusBar::slotUpdateTime()
{
	m_timerSteps++;
}

long KurooStatusBar::elapsedTime()
{
	return m_timerSteps;
}

void KurooStatusBar::clearElapsedTime()
{
	m_diffTimer->stop();
	m_internalTimer->stop();
	m_timerSteps = 0;
}

/**
* Start relative advance.
*/
void KurooStatusBar::startProgress()
{
	statusBarProgress->show();
	statusBarProgress->setMaximum( 0 );
	statusBarProgress->setTextVisible( false );
	m_diffTimer->start( 1000 );
}

/**
* Pause Timers
*/
void KurooStatusBar::pauseTimers()
{
	m_diffTimer->stop();
	m_internalTimer->stop();
	showMessage( i18n("Paused...") );
}

/**
* Unpause Timers
*/
void KurooStatusBar::unpauseTimers()
{
	m_diffTimer->start(1000);
	m_internalTimer->start( 1000 );
}

/**
* Show relative advance progress.
*/
void KurooStatusBar::slotAdvance()
{
	m_timerSteps++;
	statusBarProgress->setValue( statusBarProgress->value() + 2 );
}

