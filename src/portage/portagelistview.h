/***************************************************************************
 *	Copyright (C) 2010 by cazou88											*
 *	cazou88@users.sourceforge.net											*
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

#ifndef PORTAGE_LIST_VIEW
#define PORTAGE_LIST_VIEW

#include <QTreeView>
#include <QStringList>
#include <QMouseEvent>

#include "packagelistmodel.h"

class PackageListItem;

class PortageListView : public QTreeView
{
	Q_OBJECT
public:
	PortageListView(QWidget *parent = 0);
	~PortageListView();

	void								setPackages(const QStringList&);
	QList<PackageListItem*>				selectedPackages() const;
	PackageListItem*					currentPackage();
	PackageListItem*					packageItemById(const QString& id);
	QStringList							selectedPackagesByIds();
	void								nextPackage(const bool isPrevious);

	QList<PackageListItem*>				packages() const {return dynamic_cast<PackageListModel*>(model())->packages();}

signals:
	void								selectionChangedSignal();
	void								doubleClickedSignal(PackageListItem*);

protected:
	void								selectionChanged(const QItemSelection&, const QItemSelection&);
	void								mouseDoubleClickEvent(QMouseEvent*);
private:
	QList<PackageListItem*>				m_selectedPackages;
	QMap<QString, PackageListItem*>		m_packageIndex;

};

#endif
