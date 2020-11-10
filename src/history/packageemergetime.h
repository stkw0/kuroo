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

class PackageEmergeTime
{
public:
	PackageEmergeTime() { }
	PackageEmergeTime( int emergeTime, int count )
		: m_eTime( emergeTime ), m_eCount( count )
	{ }
	
	int emergeTime() const { return m_eTime; }
	int count() const { return m_eCount; }
	void add( int emergeTime ) { m_eTime += emergeTime; }
	void inc() { m_eCount++; }
	
private:
	int m_eTime;
	int m_eCount;
};

