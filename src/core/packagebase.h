/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright 2017  <copyright holder> <email>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PACKAGEBASE_H
#define PACKAGEBASE_H

#include <QString>
#include <QList>

#include "packageversion.h"

class PackageBase
{
public:
	PackageBase() : m_name(""), m_category("") {}
	PackageBase( const QString& name, const QString& category ) : m_name(name), m_category(category) {}

	/**
	 * Package name as kuroo in app-portage/kuroo.
	 * @return name
	 */
	inline const QString&					name() const { return m_name; }
	/**
	 * Accessor for category.
	 * @return the package category.
	 */
	inline const QString&					category() const { return m_category; }

	// Valuelist with all versions and their data
	/**
	 * Return list of versions.
	 * @return QList<PackageVersion*>
	 */
	inline QList<PackageVersion*>			versionList() const  { return m_versions; }

protected:

	// Package name-string
	QString									m_name;

	// Keep track of package's category
	QString									m_category;

	// Valuelist with all versions and their data
	QList<PackageVersion*>					m_versions;
};

#endif // PACKAGEBASE_H
