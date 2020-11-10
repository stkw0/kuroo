/***************************************************************************
*	Copyright (C) 2004 by karye												*
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
#include "global.h"



/**
* Regexp to parse emerge output.
*/
const QRegExp rxEmerge()
{
	if ( KurooConfig::portageVersion21() ) {
		return QRegExp( "^\\[ebuild([\\s\\w~#*]*)\\]\\s+"	//Also allow * for merging hardmasked packages
						"((\\S+)/(\\S+))"
						"(?:\\s*\\[([^\\]]*)\\])?"
						"(?:\\s*\\[([^\\]]*)\\])?"
						"(?:\\s*USE=\"([^\"]*)\")?"
						"(?:\\s*LINGUAS=\"(?:[^\"]*)\")?"
						"(?:\\s*\\w+=\"(?:[^\"]*)\")*"		//Capture and discard all extra use-expands
						"(?:\\s*(\\d*(,\\d*)*)\\s(?:Ki|k)B)?" );
	} else {
		return QRegExp( "^\\[ebuild([\\s\\w]*)\\]\\s+"
						"((\\S+)/(\\S+))"
						"(?:\\s*\\[([^\\]]*)\\])?"
						"(?:\\s*\\[([^\\]]*)\\])?"
						"((?:\\s*[\\(\\-\\+]+\\w+[\\)%]?)*)"
						"(?:\\s(\\d*,?\\d*)\\skB)?" );
	}
}

/**
* Parse out category, package name and version parts from package.
* @param sring to parse out cpv from
*/
const QStringList parsePackage( const QString& packageString )
{
	QRegExp rx( "(?:[a-z]|[A-Z]|[0-9]|-)*"
				"("
				"(-(?:\\d+\\.)*\\d+[a-z]?)"
				"(?:_(?=alpha|beta|pre|rc|p)\\d*)?"
				"(?:-r\\d*)?"
				")" );
	QStringList list;
	QString nameVersion;
	QString package( packageString );
	package.remove( ' ' );

	// Parse out the category first
	if ( package.contains( '/' ) ) {
		list << package.section( "/", 0, 0 );
		nameVersion = package.section( "/", 1, 1 );
	} else {
		list << QString::null;
		nameVersion = package;
	}

	// Now package name and version
	if ( rx.indexIn( nameVersion ) != -1 ) {
		QString name = nameVersion.section( rx.cap( 1 ), 0, 0 );
		list << name;
		list << nameVersion.section( name + "-", 1 );
		return list;
	} else
		return QStringList();
}

/**
* Return duration in seconds formated as "d hh.mm.ss".
*/
const QString formatTime( const long& duration )
{
	KLocale *loc = KLocale::global();
	QString totalDays;
	unsigned durationDays, totalSeconds;

	durationDays = duration / 86400;
	totalSeconds = duration % 86400;

	if ( durationDays > 0 ) {
		totalDays = i18n( "%1d ", QString::number( durationDays ) );
	}

	QTime emergeTime( 0, 0, 0 );
	emergeTime = emergeTime.addSecs( totalSeconds );
	return totalDays + loc->formatTime( emergeTime, true, true );
}
