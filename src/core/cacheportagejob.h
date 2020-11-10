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

#ifndef CACHEPORTAGEJOB_H
#define CACHEPORTAGEJOB_H

#include <QObject>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/JobPointer>
#include <ThreadWeaver/QObjectDecorator>
#include <ThreadWeaver/Thread>

class DbConnection;

/**
 * @class CachePortageJob
 * @short Thread to cache package information from the Portage directory to speed up portage view refreshing.
 */
class CachePortageJob : public ThreadWeaver::QObjectDecorator
{
public:
	CachePortageJob();
};
class CachePortageJobImpl : public ThreadWeaver::Job
{
public:
	CachePortageJobImpl();
	~CachePortageJobImpl();

protected:
	virtual void				run( ThreadWeaver::JobPointer, ThreadWeaver::Thread* );
	//void 						completeJob();

private:
	DbConnection* const 		m_db;
};

#endif
