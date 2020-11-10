/***************************************************************************
*   Copyright (C) 2004 by karye										   *
*   karye@users.sourceforge.net										   *
*																		 *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or	 *
*   (at your option) any later version.								   *
*																		 *
*   This program is distributed in the hope that it will be useful,	   *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of		*
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the		 *
*   GNU General Public License for more details.						  *
*																		 *
*   You should have received a copy of the GNU General Public License	 *
*   along with this program; if not, write to the						 *
*   Free Software Foundation, Inc.,									   *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.			 *
***************************************************************************/

#ifndef VERSIONVIEW_H
#define VERSIONVIEW_H

#include <QTreeWidget>

class VersionView : public QTreeWidget
{
	Q_OBJECT
public:
	VersionView( QWidget *parent = 0/*, const QString& name = 0 */);
	~VersionView();

	/**
	 * @class VersionViewItem
	 * @short Subclass for formating text.
	 */
	class VersionItem : public QTreeWidgetItem
	{
	public:
		VersionItem( QTreeWidget* parent, const QString& version, const bool& isInstalled, const int& stability );
		~VersionItem();

		inline bool	isInstalled() { return m_isInstalled; }

	private:
		bool		m_isInstalled;
		int		m_stability;
	};
	
	void		insertItem( const QString& version, const QString& stability, const QString& size, const bool& isInstalled );
	void		usedForInstallation( const QString& version );
	inline bool	hasUpdate() const { return m_installedIndex != m_emergeIndex; }
	inline QString 	updateVersion() const { return m_emergeVersion; }
	
private:
	QString		m_emergeVersion;
	QModelIndex	m_installedIndex, m_emergeIndex;
};

#endif
