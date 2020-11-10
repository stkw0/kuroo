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

#include <QWizard>
#include <QCheckBox>
#include <QPushButton>

#include <KLocale>
#include <KDebug>
#include <KMessageBox>
#include <kio/job.h>

#include "singleton.h"
#include "global.h"
#include "introdlg.h"
#include "settings.h"

/**
 * @class IntroDlg
 * @short Kuroo introduction wizard.
 */
IntroDlg::IntroDlg( /*QWidget* parent */)
{
	setupUi( this );
	setWindowTitle( "Kuroo-" + KurooConfig::hardVersion() );
	setOption( NoBackButtonOnStartPage, true );
	setButtonText( FinishButton, i18n("Ok") );
	setOption( HaveHelpButton, false );

	introText->setText("<h2>Kuroo-" + KurooConfig::hardVersion().section( "_", 0, 0 ) + "</h2><p>" +
					   i18n("Kuroo - A KDE Portage frontend that allows you to do most common software maintenance tasks on gentoo systems</p>"));

	QString backupFilesText = QString("<qt><table width=100%><tr><td>");
	backupFilesText += i18n("Make copies into %1 of following files:", kurooDir + "backup/");
	backupFilesText += QString("</td></tr>");
	foreach(QString f, KurooConfig::filePackageKeywords())
		backupFilesText += "<tr><td>" + f + "</td></tr>";
	foreach(QString f, KurooConfig::filePackageUserUnMask())
		backupFilesText += "<tr><td>" + f + "</td></tr>";
	foreach(QString f, KurooConfig::filePackageUserMask())
		backupFilesText += "<tr><td>" + f + "</td></tr>";
	foreach(QString f, KurooConfig::filePackageUserUse())
		backupFilesText += "<tr><td>" + f + "</td></tr>";
	backupFilesText += "<tr><td>" + KurooConfig::fileWorld() + "</td></tr>" +
			   "<tr><td>" + KurooConfig::fileMakeConf() + "</td></tr></table></qt>";

	backupFiles->setText(backupFilesText);
	adjustSize();
}

IntroDlg::~IntroDlg() {}

void IntroDlg::accept()
{
	// Backup all portage files changeable by kuroo
	if ( cbBackup->isChecked() ) {
		QString dt = "_" + QDateTime::currentDateTime().toString( "yyyyMMdd_hhmm" );
		//QString filePackageKeywords( KurooConfig::filePackageKeywords() );
		foreach(QString f, KurooConfig::filePackageKeywords())
			KIO::file_copy( QUrl::fromLocalFile( f ), QUrl::fromLocalFile( kurooDir + "backup/package.keywords-" + f.section( "/", -1 ) + dt ),
					-1, KIO::Overwrite | KIO::HideProgressInfo);
		//QString filePackageUserUnMask( KurooConfig::filePackageUserUnMask() );
		foreach(QString f, KurooConfig::filePackageUserUnMask())
			KIO::file_copy( QUrl::fromLocalFile( f ), QUrl::fromLocalFile( kurooDir + "backup/package.unmask-" + f.section( "/", -1 ) + dt ),
					-1, KIO::Overwrite | KIO::HideProgressInfo );
		//QString filePackageUserMask( KurooConfig::filePackageUserMask() );
		foreach(QString f, KurooConfig::filePackageUserMask())
			KIO::file_copy( QUrl::fromLocalFile( f ), QUrl::fromLocalFile( kurooDir + "backup/package.mask-" + f.section( "/", -1 ) + dt ),
					-1, KIO::Overwrite | KIO::HideProgressInfo );
		//QString filePackageUserUse( KurooConfig::filePackageUserUse() );
		foreach(QString f, KurooConfig::filePackageUserUse())
			KIO::file_copy( QUrl::fromLocalFile( f ), QUrl::fromLocalFile( kurooDir + "backup/package.use-" + f.section( "/", -1 ) + dt ),
					-1, KIO::Overwrite | KIO::HideProgressInfo );
		QString fileWorld( KurooConfig::fileWorld() );
		KIO::file_copy( QUrl::fromLocalFile( fileWorld ), QUrl::fromLocalFile( kurooDir + "backup/" + fileWorld.section( "/", -1 ) + dt ),
				-1, KIO::Overwrite | KIO::HideProgressInfo );
		QString fileMakeConf( KurooConfig::fileMakeConf() );
		KIO::file_copy( QUrl::fromLocalFile( fileMakeConf ), QUrl::fromLocalFile( kurooDir + "backup/" + fileMakeConf.section( "/", -1 ) + dt ),
				-1, KIO::Overwrite | KIO::HideProgressInfo );
	}

	KMessageBox::enableAllMessages();
	QWizard::accept();
}


