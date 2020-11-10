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
#include "uninstallinspector.h"
#include "ui_uninstallbase.h"

#include <QTreeWidget>
#include <KConfigGroup>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

/**
 * @class UninstallInspector
 * @short Dialog for selected package and version to uninstall.
 */
UninstallInspector::UninstallInspector( QWidget *parent )
	: QDialog( parent )
{
	//i18n( "Uninstall Packages" ), false, i18n( "Uninstall Packages" ), KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, false
	QWidget *mainWidget = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout;
	setLayout(mainLayout);
	mainLayout->addWidget(mainWidget);
	buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply);
	QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
	okButton->setDefault(true);
	okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	//PORTING SCRIPT: WARNING mainLayout->addWidget(buttonBox) must be last item in layout. Please move it.
	mainLayout->addWidget(buttonBox);
	m_uninstallbase.setupUi(mainWidget);
	connect(okButton, SIGNAL(clicked()), SLOT(slotOk()));
	connect(m_uninstallbase.uninstallView, &QTreeWidget::itemChanged, this, &UninstallInspector::slotItemActivated);
}

UninstallInspector::~UninstallInspector()
{}

void UninstallInspector::slotItemActivated(QTreeWidgetItem* item/*, int col*/)
{
	if (item->childCount() == 0)
		return;

	bool active = (item->checkState(0) == Qt::Checked);

	QTreeWidgetItem *child = 0;
	for(int i = 0; (child = item->child(i)); i++)
		child->setDisabled(!active);
}

/**
 * Open dialog and list package and versions installed.
 * @param packageList
 */
void UninstallInspector::view( const QStringList& packageList )
{
	m_uninstallbase.uninstallView->clear();

	const QStringList systemFilesList = KurooConfig::systemFiles().split(" ");
	bool isPartOfSystem( false );

	for ( QStringList::ConstIterator itPackage = packageList.begin(), itPackageEnd = packageList.end(); itPackage != itPackageEnd; ++itPackage ) {
		QString id = *itPackage++;
		QString package = *itPackage;

		QTreeWidgetItem* itemPackage = new QTreeWidgetItem( m_uninstallbase.uninstallView );
		itemPackage->setText( 0, package );
		itemPackage->setFlags( Qt::ItemIsUserCheckable );
		itemPackage->setExpanded( true );
		itemPackage->setCheckState( 0, Qt::Checked );
		itemPackage->setDisabled(false);

		// Warn if package is included in gentoo base system profile
		foreach ( QString file, systemFilesList ) {
			if ( file == package ) {
				itemPackage->setIcon( 0, QIcon::fromTheme(QStringLiteral("kuroo_warning")) );
				isPartOfSystem = true;
			}
		}

		// List all versions if more that one installed version is found
		const QStringList versionsList = KurooDBSingleton::Instance()->packageVersionsInstalled( id );
		if ( versionsList.size() > 1 ) {
			foreach( QString version, versionsList ) {
				QTreeWidgetItem* itemVersion = new QTreeWidgetItem( itemPackage );
				itemVersion->setText( 0, version );
				itemVersion->setFlags( Qt::ItemIsUserCheckable );
				itemVersion->setDisabled( false );
				itemVersion->setCheckState( 0, Qt::Checked );
			}
		}
	}

	if ( isPartOfSystem ) {
		m_uninstallbase.uninstallWarning->setText( i18n("<font color=red><b>You are uninstalling packages part of your system profile!<br/>"
												"This may be damaging to your system!</b></font>") );
		m_uninstallbase.uninstallWarning->show();
	}
	else {
		m_uninstallbase.uninstallWarning->hide();
	}

	show();
}

/**
 * Collect user checked packages or versions and launch unmerge.
 */
void UninstallInspector::slotOk()
{
	qDebug() << "Unmerging packages :";
	QStringList packageList;

	QTreeWidgetItemIterator it( m_uninstallbase.uninstallView );
	while ( *it ) {
		if ( (*it)->checkState(0) == Qt::Checked && (*it)->childCount() == 0) {
			if ( (*it)->parent() ) {
				if ( (*it)->parent()->checkState(0) == Qt::Checked )
				{
					qDebug() << "	*" << (*it)->parent()->text(0) + "-" + (*it)->text(0);
					packageList += "=" + (*it)->parent()->text(0) + "-" + (*it)->text(0);
				}
			} else {
				qDebug() << "	*" << (*it)->text(0);
				packageList += (*it)->text(0);
			}
		}
		++it;
	}

	if (!packageList.isEmpty())
		EmergeSingleton::Instance()->unmerge( packageList );
	hide();
}

