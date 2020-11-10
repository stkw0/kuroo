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

#ifndef PACKAGEINSPECTOR_H
#define PACKAGEINSPECTOR_H

#include <QDialog>
#include <QDialogButtonBox>
#include <KProcess>

#include "ui_inspectorbase.h"
#include "portagelistview.h"

class PackageListItem;

/**
 * @class PackageInspector
 * @short The package Inspector dialog for all advanced settings.
 */
class PackageInspector : public QDialog, public Ui::InspectorBase
{
	Q_OBJECT
public:
	PackageInspector( QWidget *parent = 0 );
	~PackageInspector();

	void				edit( PackageListItem* portagePackage, const int& view );
	/**
	 * Return the caller.
	 */
	inline bool			isParentView( const int& view ) const { return m_view == view; }
	void				showHardMaskInfo();

private:
	void				updateVersionData();
	void				rollbackSettings();
	void				loadUseFlagDescription();
	void				loadChangeLog();
	void				askApplySettings();

private slots:
	void				slotHardMaskInfo();
	void				slotPreviousPackage();
	void				slotNextPackage();
	void				showSettings();
	void				slotRefreshTabs();
	void				slotLoadInstalledFiles( const QString& version );
	void				slotApply();
	void				slotCancel();
	void				slotOk();
	void				slotSetStability( int rbStability );
	void				slotSetSpecificVersion( const QString& version );
    void				slotSetUseFlags( QTreeWidgetItem* useItem, int column );
	void				slotLoadEbuild( const QString& version );
	void				slotLoadDependencies( const QString& version );
	void				slotLoadUseFlags( const QString& version );
	void				slotCalculateUse();
    void				slotCollectPretendOutput();
    void				slotParsePackageUse();
    void				slotParseTempUse();
	void				slotQueue();
	void				slotWorld();
	void				setStable();
	void				setTesting();
	void				setMasked();

private:

	// View that called the Inspector
	int						m_view;

	// Keep track when user changes any version masking settings
	bool					m_versionSettingsChanged;

	// Keep track when user changes use settings
	bool					m_useSettingsChanged;

	// Is this package settings untouched
	bool					m_isVirginState;

	QString					m_id, m_category, m_package, m_hardMaskComment;
	QMap<QString, QString>	m_useMap;
	PackageListItem*		m_portagePackage;
	int						m_stabilityBefore;
	QString					m_versionBefore;
	QStringList				m_pretendUseLines;
	QStringList				m_useList;

	KProcess* 				eProc;
	QDialogButtonBox*		buttonBox;

signals:
	void					signalNextPackage( bool up );
	void					signalPackageChanged();
};

#endif
