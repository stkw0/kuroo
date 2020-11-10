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
#ifndef LOG_H
#define LOG_H

#include <KIO/Job>
#include <QObject>
#include <QFile>

class QCheckBox;
class QTextBrowser;

/**
 * @class Log
 * @short Log output from all actions as emerge, scanning... to log window and to file.
 */
class Log : public QObject
{
Q_OBJECT
public:
	Log( QObject *m_parent = 0 );
	~Log();
	
	void 			setGui( QTextBrowser *logBrowserGui, QCheckBox *verboseLogGui, QCheckBox *saveLogGui );
	void 			writeLog( const QString& output, int logType );
	
	const QString		init( QObject *parent = 0 );
	KIO::Job*		backupLog();
	static uint		buffer_MaxLines;
	
private:
	void addText(const QString&);
	QObject*		m_parent;
	QCheckBox 		*m_verboseLog, *m_saveLog;
	QTextBrowser 		*m_logBrowser;
	QFile 			m_logFile;
	unsigned int		numLines;
	
signals:
	void			signalLogChanged();
};

#endif
