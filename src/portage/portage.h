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

#ifndef PORTAGE_H
#define PORTAGE_H

#include <QObject>
#include <ThreadWeaver/ThreadWeaver>

/**
 * @class Portage
 * @short Handling of Portage.
 */
class Portage : public QObject
{
Q_OBJECT
public:
	Portage( QObject *m_parent = 0 );
	~Portage();
	
	void						init( QObject *parent = 0 );
	
	void						loadWorld();
	bool						isInWorld( const QString& package );
	
	void						appendWorld( const QStringList& packageList );
	void						removeFromWorld( const QStringList& packageList );
	
	void						pretendPackageList( const QStringList& packageIdList );
	void						uninstallInstalledPackageList( const QStringList& packageIdList );
	void						addInstalledPackage( const QString& package );
	void						removeInstalledPackage( const QString& package );
	
	void						checkUpdates( const QString& id, const QString& emergeVersion, int hasUpdate );
	
	
public slots:
	void						slotChanged();
	void						slotPackageChanged();
	bool						slotRefresh();
	
	bool						slotScan();
	void						slotScanCompleted();
	bool						slotSync();
	void						slotSyncCompleted();
	
	bool						slotRefreshUpdates();
	bool						slotLoadUpdates();

private slots:
	void						slotWeaverDone(ThreadWeaver::JobPointer);
	
signals:
	void						signalPortageChanged();
	void						signalWorldChanged();
	
private:
	QObject*					m_parent;
	
	// All packages in the @world profile
	QMap<QString, QString>		m_mapWorld;
};

#endif
