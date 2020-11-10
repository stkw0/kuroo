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

#include "common.h"
#include "queuelistitem.h"
#include "queuelistview.h"
#include "queuelistmodel.h"


QueueListItem::QueueListItem(QObject *parent)
 : PackageListItem(parent), m_parentId( "" ), m_parent(NULL), m_hasStarted(false)
 , m_isComplete(false), m_pretended(false), m_steps(0), m_testTimer(0)
{
}

QueueListItem::QueueListItem(const QString& name, const QString& id, const QString& category, const int status, const int duration, QObject *parent)
 : PackageListItem(name, id, category, QString(), status, QString(), parent), m_parentId( "" ), m_parent(NULL), m_hasStarted(false)
 , m_isComplete(false), m_pretended(false), m_steps(0), m_testTimer(0)
{
	qDebug() << "QueueListItem " << name << " constructed";
	setDuration(duration);
}

QueueListItem::~QueueListItem()
{
	qDebug() << "Destroying QueueListItem " << name();
	foreach(QueueListItem *i, m_children)
		delete i;
}

void QueueListItem::setVersion(const QString& v)
{
	m_version = v;
}

void QueueListItem::setDuration(int d)
{
	m_duration = d;
}

void QueueListItem::setSize(const QString& s)
{
	m_size = s;
}

void QueueListItem::setParentId(const QString& pid)
{
	m_parentId = pid;
}

void QueueListItem::appendChild(QueueListItem* item)
{
	//Fix bug #3163827 order of queue items
	m_children.prepend(item);// << item;//
	item->setParentItem(this);
}

void QueueListItem::setParentItem(QueueListItem* item)
{
	m_parent = item;
}

void QueueListItem::setHasStarted(bool h)
{
	if (duration() <= 0)
	{
		m_testTimer = new QTimer(this);
		m_testTimer->start(100);
		connect(m_testTimer, &QTimer::timeout, this, &QueueListItem::oneStep);
	}
	m_hasStarted = h;
}

void QueueListItem::setIsComplete(bool h)
{
	if (m_testTimer)
	{
		m_testTimer->stop();
		delete m_testTimer;
		m_testTimer = 0;
	}
	m_isComplete = h;

	QueueListView *listView = dynamic_cast<QueueListView*>(parent());
	QueueListModel *model = dynamic_cast<QueueListModel*>(listView->model());

	listView->update(model->index(packageIndex() - 1, 5));
}

void QueueListItem::oneStep()
{
	m_steps += duration() <= 0 ? 2 : 1;

	QueueListView *listView = dynamic_cast<QueueListView*>(parent());
	QueueListModel *model = dynamic_cast<QueueListModel*>(listView->model());

	listView->update(model->index(packageIndex() - 1, 5));
}

void QueueListItem::setPretended(bool p)
{
	m_pretended = p;
}
