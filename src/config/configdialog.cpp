/**************************************************************************
*	Copyright (C) 2004 by													*
*	karye@users.sourceforge.net												*
*	Stefan Bogner <bochi@online.ms>											*
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
#include "systemtray.h"
#include "configdialog.h"

#include <QTextStream>
#include <QFile>
#include <QCheckBox>

#include <KConfigDialog>
#include <KConfig>
#include <KMessageBox>
#include <KConfigGroup>

#include "log.h"

/**
* @class ConfigDialog
* @short Kuroo preferences.
*
* Build settings widget for kuroo and make.conf.
* Parses make.conf and tries to keep user-format when saving back settings.
*/
ConfigDialog::ConfigDialog( QWidget *parent, const QString& name, KConfigSkeleton *config )
	: KConfigDialog( parent, name, config ), m_isDefault( false )
{
	QWidget::setAttribute(Qt::WA_DeleteOnClose);

	setFaceType(KPageDialog::Tabbed);

	QDialog *opt1 = new QDialog();
	form1.setupUi( opt1 ); //, i18n("General") );
	QDialog *opt2 = new QDialog();
	form2.setupUi( opt2 ); //, i18n("make.conf") );
	QDialog *opt3 = new QDialog();
	form3.setupUi( opt3 ); //, i18n("Etc-update warnings") );
	QDialog *opt4 = new QDialog();
	form4.setupUi( opt4 ); //, i18n("Housekeeping") );

	addPage( opt1, i18n("General"), QString("kuroo"), i18n("General preferences") );
	addPage( opt2, i18n("make.conf"), QString("kuroo_makeconf"), i18n("Edit your make.conf file") );
	addPage( opt3, i18n("Etc-update warnings"), QString("dialog-warning"), i18n("Edit your etc-update warning file list") );
	addPage( opt4, i18n("Housekeeping"), QString("kuroo_housekeeping"), i18n("Control automatic file cleanup and rebuilding") );

	connect(this, &ConfigDialog::settingsChanged, this, &ConfigDialog::slotSaveAll);
	connect(buttonBox()->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &ConfigDialog::slotDefaults);
/*
	int selected = 0;
	for(int i = 0; i < KurooConfig::filePackageKeywords().count(); ++i)
	{
		QString f = KurooConfig::filePackageKeywords()[i];
		if (f == KurooConfig::defaultFilePackageKeywords())
			selected = i;
		form1.keywords->addItem(f);
	}
	form1.keywords->setCurrentIndex(selected);

	selected = 0;
	for(int i = 0; i < KurooConfig::filePackageUserUse().count(); ++i)
	{
		QString f = KurooConfig::filePackageUserUse()[i];
		if (f == KurooConfig::defaultFilePackageUserUse())
			selected = i;
		form1.use->addItem(f);
	}
	form1.use->setCurrentIndex(selected);

	selected = 0;
	for(int i = 0; i < KurooConfig::filePackageUserMask().count(); ++i)
	{
		QString f = KurooConfig::filePackageUserMask()[i];
		if (f == KurooConfig::defaultFilePackageUserMask())
			selected = i;
		form1.mask->addItem(f);
	}
	form1.mask->setCurrentIndex(selected);

	selected = 0;
	for(int i = 0; i < KurooConfig::filePackageUserUnMask().count(); ++i)
	{
		QString f = KurooConfig::filePackageUserUnMask()[i];
		if (f == KurooConfig::defaultFilePackageUserUnMask())
			selected = i;
		form1.unmask->addItem(f);
	}
	form1.unmask->setCurrentIndex(selected);
*/
	parseMakeConf();
}

ConfigDialog::~ConfigDialog()
{}

/**
* Reset to defaults.
*/
void ConfigDialog::slotDefaults()
{
	DEBUG_LINE_INFO;
	parseMakeConf();
	show();
}

/**
* Save settings when user press "Apply".
*/
void ConfigDialog::slotSaveAll()
{
	Log::buffer_MaxLines=KurooConfig::logLines();
	qDebug() << "Buffer changed to " << Log::buffer_MaxLines << "\n";
	DEBUG_LINE_INFO;
	qDebug() << "HACK\n";
	/*switch( activePageIndex() ) {
		// Activate the systray directly (not needing restarting kuroo)
		case 0: {
			if ( KurooConfig::isSystrayEnabled() )
				SystemTray::instance()->activate();
			else
				SystemTray::instance()->inactivate();

			SignalistSingleton::Instance()->fontChanged();
			break;
		}

		case 1:
			if ( !saveMakeConf() ) {
				parseMakeConf();
				show();
				KMessageBox::error( this, i18n( "Failed to save %1. Please run as root.", KurooConfig::fileMakeConf() ), i18n( "Saving" ));
			}
	}*/

	/*KurooConfig::setDefaultFilePackageKeywords( form1.keywords->currentText() );
	KurooConfig::setDefaultFilePackageUserUse( form1.use->currentText() );
	KurooConfig::setDefaultFilePackageUserMask( form1.mask->currentText() );
	KurooConfig::setDefaultFilePackageUserUnMask( form1.unmask->currentText() );
	*/
}

/**
* Read 'make.conf' into stringList by taking into account the different kind of extended lines.
* @return linesConcatenated
*/
const QStringList ConfigDialog::readMakeConf()
{
	QStringList linesConcatenated;
	QRegExp rx( "\\s*(\\w*)(\\s*=\\s*)(\"?([^\"#]*)\"?)#*" );
	if ( !QFile::exists( KurooConfig::fileMakeConf() ) ) {
		qCritical() << KurooConfig::fileMakeConf() << " doesn't exist. Trying /etc/portage/make.conf";
		KurooConfig::setFileMakeConf(QLatin1String( "/etc/portage/make.conf" ));
	}
	QFile makeconf( KurooConfig::fileMakeConf() );

	if ( makeconf.open( QIODevice::ReadOnly ) ) {
		QTextStream stream( &makeconf );
		QStringList lines;

		// Collect all lines except comments
		while ( !stream.atEnd() )
			lines += stream.readLine();
		makeconf.close();

		// Concatenate extended lines
		QString extendedLine;
		QStringList linesCommented;
		foreach( QString line, lines ) {

			// Skip comment lines
			if ( line.isEmpty() || line.contains( QRegExp("^\\s*#") ) ) {
				linesCommented += line;
				continue;
			}

			line = line.simplified();
			if ( line.contains( "=" ) ) {
				linesConcatenated += extendedLine;
				extendedLine = line.section( QRegExp("\\\\s*$"), 0, 0 ).simplified();

				linesConcatenated += linesCommented;
				linesCommented.clear();
			} else {
				extendedLine += " " + line.section( QRegExp("\\\\s*$"), 0, 0 ).simplified();
			}
		}

		linesConcatenated += extendedLine;
	} else {
		qCritical() << "Error reading: " << KurooConfig::fileMakeConf();
	}

	return linesConcatenated;
}

/**
* Parse /etc/make.conf.
*/
void ConfigDialog::parseMakeConf()
{
	DEBUG_LINE_INFO;
	QStringList linesConcatenated = readMakeConf();
	if ( !linesConcatenated.isEmpty() ) {
		// Clear old entries
		KurooConfig::setAcceptKeywords( QString::null );
		KurooConfig::setAutoClean( true );
		KurooConfig::setBuildPrefix( QString::null );
		KurooConfig::setCBuild( QString::null );
		KurooConfig::setCCacheSize( QString::null );
		KurooConfig::setCFlags( QString::null );
		KurooConfig::setCXXFlags( QString::null );
		KurooConfig::setChost( QString::null );
		KurooConfig::setCleanDelay( QString::null );
		KurooConfig::setConfigProtect( QString::null );
		KurooConfig::setConfigProtectMask( QString::null );
		KurooConfig::setDebugBuild( QString::null );
		KurooConfig::setDirDist( QString::null );
		KurooConfig::setFeatures( QString::null );
		KurooConfig::setFetchCommand( QString::null );
		KurooConfig::setGentooMirrors( QString::null );
		KurooConfig::setFtpProxy( QString::null );
		KurooConfig::setHttpProxy( QString::null );
		KurooConfig::setMakeOpts( QString::null );
		KurooConfig::setNoColor( false );
		KurooConfig::setDirPkgTmp( QString::null );
		KurooConfig::setDirPkg( QString::null );
		KurooConfig::setDirPortLog( QString::null );
		KurooConfig::setPortageBinHost( QString::null );
		KurooConfig::setPortageNiceness( QString::null );
		KurooConfig::setDirPortageTmp( QString::null );
		KurooConfig::setDirPortage( "/usr/portage" );
		KurooConfig::setDirPortageOverlay( QString::null );
		KurooConfig::setResumeCommand( QString::null );
		KurooConfig::setRoot( QString::null );
		KurooConfig::setRsyncExcludeFrom( QString::null );
		KurooConfig::setRsyncProxy( QString::null );
		KurooConfig::setRsyncRetries( QString::null );
		KurooConfig::setRsyncRateLimit( QString::null );
		KurooConfig::setRsyncTimeOut( QString::null );
		KurooConfig::setDirRpm( QString::null );
		KurooConfig::setSync( QString::null );
		KurooConfig::setUse( QString::null );
		KurooConfig::setUseOrder( QString::null );

		// Parse the lines
		QRegExp rx( "\\s*(\\w*)(\\s*=\\s*)(\"?([^\"#]*)\"?)#*" );
		foreach( QString line, linesConcatenated ) {

			// Skip comment lines
			if ( line.isEmpty() || line.contains( QRegExp("^\\s*#") ) )
				continue;

			if ( line.contains( QRegExp("ACCEPT_KEYWORDS=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setAcceptKeywords( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse ACCEPT_KEYWORDS.";
				continue;
			}

			if ( line.contains( QRegExp("AUTOCLEAN=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					//KurooConfig::setAutoClean( rx.cap(4) );
					qWarning() << "KurooConfig::setAutoClean( rx.cap(4) );";
				} else {
					qWarning() << "Parsing /etc/make.conf: can not parse AUTOCLEAN.";
				}
				continue;
			}

			if ( line.contains( QRegExp("BUILD_PREFIX=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setBuildPrefix( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse BUILD_PREFIX.";
				continue;
			}

			if ( line.contains( QRegExp("CBUILD=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setCBuild( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse CBUILD.";
				continue;
			}

			if ( line.contains( QRegExp("CCACHE_SIZE=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setCCacheSize( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse CCACHE_SIZE.";
				continue;
			}

			if ( line.contains( QRegExp("CFLAGS=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setCFlags( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse CFLAGS.";
				continue;
			}

			if ( line.contains( QRegExp("CXXFLAGS=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setCXXFlags( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse CXXFLAGS.";
				continue;
			}

			if ( line.contains( QRegExp("CHOST=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setChost( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse CHOST.";
				continue;
			}

			if ( line.contains( QRegExp("CLEAN_DELAY=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setCleanDelay( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse CLEAN_DELAY.";
				continue;
			}

			if ( line.contains( QRegExp("CONFIG_PROTECT=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setConfigProtect( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse CONFIG_PROTECT.";
				continue;
			}

			if ( line.contains( QRegExp("CONFIG_PROTECT_MASK=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setConfigProtectMask( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse BUILD_PREFIX.";
				continue;
			}

			if ( line.contains( QRegExp("DEBUGBUILD=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setDebugBuild( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse DEBUGBUILD.";
				continue;
			}

			if ( line.contains( QRegExp("DISTDIR=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setDirDist( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse DISTDIR.";
				continue;
			}

			if ( line.contains( QRegExp("FEATURES=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setFeatures( rx.cap(4) );
			else
				qWarning() << "Parsing /etc/make.conf: can not parse FEATURES.";
				continue;
			}

			if ( line.contains( QRegExp("FETCHCOMMAND=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setFetchCommand( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse FETCHCOMMAND.";
				continue;
			}

			if ( line.contains( QRegExp("GENTOO_MIRRORS=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setGentooMirrors( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse GENTOO_MIRRORS.";
				continue;
			}

			if ( line.contains( QRegExp("FTP_PROXY=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setFtpProxy( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse FTP_PROXY.";
				continue;
			}

			if ( line.contains( QRegExp("HTTP_PROXY=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setHttpProxy( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse HTTP_PROXY.";
				continue;
			}

			if ( line.contains( QRegExp("MAKEOPTS=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setMakeOpts( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse MAKEOPTS.";
				continue;
			}

			/*if ( line.contains( QRegExp("NOCOLOR=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setNoColor( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse NOCOLOR.";
				continue;
			}*/

			if ( line.contains( QRegExp("PKG_TMPDIR=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setDirPkgTmp( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse PKG_TMPDIR.";
				}
				continue;
			}

			if ( line.contains( QRegExp("PKGDIR=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setDirPkg( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse PKGDIR.";
				}
				continue;
			}

			if ( line.contains( QRegExp("PORT_LOGDIR=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setDirPortLog( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse PORT_LOGDIR.";
				}
				continue;
			}

			if ( line.contains( QRegExp("PORTAGE_BINHOST=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setPortageBinHost( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse PORTAGE_BINHOST.";
				}
				continue;
			}

			if ( line.contains( QRegExp("PORTAGE_ELOG_CLASSES=") ) ) {
				/* this is kind of messy, but it gets the job done */
				if( line.contains( QRegExp("warn") ) )
					KurooConfig::setElogWarn( 1 );
				if( line.contains( QRegExp("error") ) )
					KurooConfig::setElogError( 1 );
				if( line.contains( QRegExp("info") ) )
					KurooConfig::setElogInfo( 1 );
				if( line.contains( QRegExp("log") ) )
					KurooConfig::setElogLog( 1 );
				if( line.contains( QRegExp("qa") ) )
					KurooConfig::setElogQa( 1 );
				continue;
			}

			if( line.contains( QRegExp("PORTAGE_ELOG_SYSTEM=") ) ) {
				/* this is also kindof messy, got a better way? */
				if( line.contains( QRegExp("save") ) )
					KurooConfig::setElogSysSave( 1 );
				if( line.contains( QRegExp("mail") ) )
					KurooConfig::setElogSysMail( 1 );
				if( line.contains( QRegExp("syslog") ) )
					KurooConfig::setElogSysSyslog( 1 );
				if( line.contains( QRegExp("custom") ) )
					KurooConfig::setElogSysCustom( 1 );
				if( line.contains( QRegExp("save_summary") ) )
					KurooConfig::setElogSysSaveSum( 1 );
				if( line.contains( QRegExp("mail_summary") ) )
					KurooConfig::setElogSysMailSum( 1 );
				continue;
			}

			if( line.contains( QRegExp("PORTAGE_ELOG_COMMAND=") ) ) {
				if( rx.indexIn( line ) > -1 ) {
					KurooConfig::setElogCustomCmd( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: cannot parse PORTAGE_ELOG_COMMAND";
				}
				continue;
			}

			if( line.contains( QRegExp("PORTAGE_ELOG_MAILURI=") ) ) {
				if( rx.indexIn( line ) > -1 ) {
					KurooConfig::setElogMailURI( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: cannot parse PORTAGE_ELOG_MAILURI";
				}
				continue;
			}

			if( line.contains( QRegExp("PORTAGE_ELOG_MAILFROM=") ) ) {
				if( rx.indexIn( line ) > -1 ) {
					KurooConfig::setElogMailFromURI( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: cannot parse PORTAGE_ELOG_MAILFROM";
				}
				continue;
			}

			if( line.contains( QRegExp("PORTAGE_ELOG_MAILSUBJECT=") ) ) {
				if( rx.indexIn( line ) > -1 ) {
					KurooConfig::setElogSubject( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: cannot parse PORTAGE_MAILSUBJECT";
				}
				continue;
			}

			if ( line.contains( QRegExp("PORTAGE_NICENESS=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setPortageNiceness( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse PORTAGE_NICENESS.";
				}
				continue;
			}

			if ( line.contains( QRegExp("PORTAGE_TMPDIR=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setDirPortageTmp( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse PORTAGE_TMPDIR.";
				}
				continue;
			}

			if ( line.contains( QRegExp("PORTDIR=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setDirPortage( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse PORTDIR.";
				}
				continue;
			}

			if ( line.contains( QRegExp("PORTDIR_OVERLAY=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setDirPortageOverlay( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse PORTDIR_OVERLAY.";
				}
				continue;
			}

			if ( line.contains( QRegExp("RESUMECOMMAND=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setResumeCommand( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse RESUMECOMMAND.";
				}
				continue;
			}

			if ( line.contains( QRegExp("ROOT=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setRoot( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse ROOT.";
				}
				continue;
			}

			if ( line.contains( QRegExp("RSYNC_EXCLUDEFROM=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setRsyncExcludeFrom( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse RSYNC_EXCLUDEFROM.";
				}
				continue;
			}

			if ( line.contains( QRegExp("RSYNC_PROXY=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setRsyncProxy( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse RSYNC_PROXY.";
				}
				continue;
			}

			if ( line.contains( QRegExp("RSYNC_RETRIES=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setRsyncRetries( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse RSYNC_RETRIES.";
				}
				continue;
			}

			if ( line.contains( QRegExp("RSYNC_RATELIMIT=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setRsyncRateLimit( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse RSYNC_RATELIMIT.";
				}
				continue;
			}

			if ( line.contains( QRegExp("RSYNC_TIMEOUT=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setRsyncTimeOut( rx.cap(4)  );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse RSYNC_TIMEOUT.";
				}
				continue;
			}

			if ( line.contains( QRegExp("RPMDIR=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setDirRpm( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse RPMDIR.";
				}
				continue;
			}

			if ( line.contains( QRegExp("SYNC=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setSync( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse SYNC.";
				}
				continue;
			}

			if ( line.contains( QRegExp("USE=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setUse( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse USE.";
				}
				continue;
			}

			if ( line.contains( QRegExp("USE_ORDER=") ) ) {
				if ( rx.indexIn( line ) > -1 ) {
					KurooConfig::setUseOrder( rx.cap(4) );
				}
				else {
					qWarning() << "Parsing /etc/make.conf: can not parse USE_ORDER.";
				}
				continue;
			}

			/*if ( line.contains( QRegExp("NOCOLOR=") ) ) {
				if ( rx.indexIn( line ) > -1 )
					KurooConfig::setNoColor( rx.cap(4) );
				else
					qWarning() << "Parsing /etc/make.conf: can not parse NOCOLOR.";
			}*/
		}
	}
}

/**
* Save back /etc/make.conf.
* @return success
*/
bool ConfigDialog::saveMakeConf()
{
	QStringList linesConcatenated = readMakeConf();
	if ( linesConcatenated.isEmpty() ) {
		return false;
	}

	QString line;
	QStringList lines;
	QFile file( KurooConfig::fileMakeConf() );
	QMap<QString, QString> keywords;
	QRegExp rxLine( "^\\s*(\\w*)(\\s*=\\s*)(\"?([^\"#]*)\"?)#*" );

	// Collect all keywords
	foreach ( line, linesConcatenated ) {

		if ( line.contains( QRegExp( "^\\s*(CHOST|CFLAGS|CXXFLAGS|MAKEOPTS|USE|GENTOO_MIRRORS|PORTDIR_OVERLAY|FEATURES|PORTDIR|PORTAGE_TMPDIR|"
						"DISTDIR|ACCEPT_KEYWORDS|AUTOCLEAN|BUILD_PREFIX|CBUILD|CCACHE_SIZE|CLEAN_DELAY|CONFIG_PROTECT|"
									"CONFIG_PROTECT_MASK|DEBUGBUILD|FETCHCOMMAND|HTTP_PROXY|FTP_PROXY|PKG_TMPDIR|PKGDIR|PORT_LOGDIR|PORTAGE_BINHOST|"
						"PORTAGE_NICENESS|RESUMECOMMAND|ROOT|RSYNC_EXCLUDEFROM|RSYNC_PROXY|RSYNC_RETRIES|RSYNC_RATELIMIT|"
						"RSYNC_TIMEOUT|RPMDIR|SYNC|USE_ORDER|NOCOLOR|"
						"PORTAGE_ELOG_MAILURI|PORTAGE_ELOG_MAILFROM|"
								"PORTAGE_ELOG_MAILSUBJECT|PORTAGE_ELOG_COMMAND)" ) ) ) {

			if ( rxLine.indexIn( line ) > -1 ) {
				keywords[ rxLine.cap(1) ] = rxLine.cap(4);
			}
			else {
				qWarning() << QString("Parsing %1: can not match keyword %2.").arg( KurooConfig::fileMakeConf() ).arg( rxLine.cap(1) );
			}
		}
	}

	// Update keywords from settings
	keywords[ "ACCEPT_KEYWORDS" ] = KurooConfig::acceptKeywords();

	keywords[ "CHOST" ] = KurooConfig::chost();

	keywords[ "CFLAGS" ] = KurooConfig::cFlags();

	keywords[ "CXXFLAGS" ] = KurooConfig::cXXFlags();

	keywords[ "MAKEOPTS" ] = KurooConfig::makeOpts();

	keywords[ "USE" ] = KurooConfig::use();

	keywords[ "CONFIG_PROTECT" ] = KurooConfig::configProtect();

	keywords[ "CONFIG_PROTECT_MASK" ] = KurooConfig::configProtectMask();

	keywords[ "FEATURES" ] = KurooConfig::features();

	keywords[ "USE_ORDER" ] = KurooConfig::useOrder();

	//Fix a BUG which stores NOCOLOR content translated to /etc/make.conf which should not be..
	if (KurooConfig::noColor()) {
		keywords[ "NOCOLOR" ] = "true";
	}
	else {
		keywords[ "NOCOLOR" ] = "false";
	}

	keywords[ "ROOT" ] = KurooConfig::root();

	keywords[ "PORTDIR" ] = KurooConfig::dirPortage();

	keywords[ "PORTDIR_OVERLAY" ] = KurooConfig::dirPortageOverlay();

	keywords[ "DISTDIR" ] = KurooConfig::dirDist();

	keywords[ "RPMDIR" ] = KurooConfig::dirRpm();

	keywords[ "PKG_TMPDIR" ] = KurooConfig::dirPkgTmp();

	keywords[ "PKGDIR" ] = KurooConfig::dirPkg();

	keywords[ "PORT_LOGDIR" ] = KurooConfig::dirPortLog();

	keywords[ "PORTAGE_BINHOST" ] = KurooConfig::portageBinHost();

	keywords[ "PORTAGE_NICENESS" ] = KurooConfig::portageNiceness();

	keywords[ "PORTAGE_TMPDIR" ] = KurooConfig::dirPortageTmp();

	//Fix a BUG which stores AUTOCLEAN content translated to /etc/make.conf which should not be..
	if (KurooConfig::autoClean()) {
		keywords[ "AUTOCLEAN" ] = "yes";
	}
	else {
		keywords[ "AUTOCLEAN" ] = "no";
	}

	keywords[ "BUILD_PREFIX" ] = KurooConfig::buildPrefix();

	keywords[ "CBUILD" ] = KurooConfig::cBuild();

	keywords[ "CCACHE_SIZE" ] = KurooConfig::cCacheSize();

	keywords[ "CLEAN_DELAY" ] = KurooConfig::cleanDelay();

	keywords[ "DEBUGBUILD" ] = KurooConfig::debugBuild();

	keywords[ "FETCHCOMMAND" ] = KurooConfig::fetchCommand();

	keywords[ "RESUMECOMMAND" ] = KurooConfig::resumeCommand();

	keywords[ "RSYNC_EXCLUDEFROM" ] = KurooConfig::rsyncExcludeFrom();

	keywords[ "HTTP_PROXY" ] = KurooConfig::httpProxy();

	keywords[ "FTP_PROXY" ] = KurooConfig::ftpProxy();

	keywords[ "GENTOO_MIRRORS" ] = KurooConfig::gentooMirrors();

	keywords[ "RSYNC_PROXY" ] = KurooConfig::rsyncProxy();

	keywords[ "RSYNC_RETRIES" ] = KurooConfig::rsyncRetries();

	keywords[ "RSYNC_RATELIMIT" ] = KurooConfig::rsyncRateLimit();

	keywords[ "RSYNC_TIMEOUT" ] = KurooConfig::rsyncTimeOut();

	QString elog_value = "";
	if( KurooConfig::elogWarn() ) elog_value.append("warn");
	if( KurooConfig::elogError() ) elog_value.append("error");
	if( KurooConfig::elogLog() ) elog_value.append("log");
	if( KurooConfig::elogInfo() ) elog_value.append("info");
	if( KurooConfig::elogQa() ) elog_value.append("qa");
	keywords[ "PORTAGE_ELOG_CLASSES" ] = elog_value;

	QString elog_sys = "";
	if( KurooConfig::elogSysSave() ) elog_sys.append("save");
	if( KurooConfig::elogSysMail() ) elog_sys.append("mail");
	if( KurooConfig::elogSysSyslog() ) elog_sys.append("syslog");
	if( KurooConfig::elogSysCustom() ) elog_sys.append("custom");
	if( KurooConfig::elogSysSaveSum() ) elog_sys.append("save_summary");
	if( KurooConfig::elogSysMailSum() ) elog_sys.append("mail_summary");
	keywords[ "PORTAGE_ELOG_SYSTEM" ] = elog_sys;

	keywords[ "PORTAGE_ELOG_COMMAND" ] = KurooConfig::elogCustomCmd();
	keywords[ "PORTAGE_ELOG_MAILURI" ] = KurooConfig::elogMailURI();
	keywords[ "PORTAGE_ELOG_MAILFROM" ] = KurooConfig::elogMailFromURI();
	keywords[ "PORTAGE_ELOG_MAILSUBJECT" ] = KurooConfig::elogSubject();
	keywords[ "SYNC" ] = KurooConfig::sync();

	// Write back everything
	if ( file.open( QIODevice::WriteOnly ) ) {
		QTextStream stream( &file );

		bool top( true );
		foreach ( line, linesConcatenated ) {

			// Skip first empty lines
			if ( top && line.isEmpty() ) {
				continue;
			}
			else {
				top = false;
			}

			if ( line.contains( QRegExp( "^\\s*(CHOST|CFLAGS|CXXFLAGS|MAKEOPTS|USE|GENTOO_MIRRORS|PORTDIR_OVERLAY|FEATURES|PORTDIR|PORTAGE_TMPDIR|"
										"DISTDIR|ACCEPT_KEYWORDS|AUTOCLEAN|BUILD_PREFIX|CBUILD|CCACHE_SIZE|CLEAN_DELAY|CONFIG_PROTECT|"
										"CONFIG_PROTECT_MASK|DEBUGBUILD|FETCHCOMMAND|HTTP_PROXY|FTP_PROXY|PKG_TMPDIR|PKGDIR|PORT_LOGDIR|"
										"PORTAGE_BINHOST|PORTAGE_NICENESS|RESUMECOMMAND|ROOT|RSYNC_EXCLUDEFROM|RSYNC_PROXY|RSYNC_RETRIES|"
										"RSYNC_RATELIMIT|RSYNC_TIMEOUT|RPMDIR|SYNC|USE_ORDER|NOCOLOR|"
										"PORTAGE_ELOG_CLASSES|PORTAGE_ELOG_SYSTEM|PORTAGE_ELOG_MAILURI|PORTAGE_ELOG_MAILFROM|"
										"PORTAGE_ELOG_MAILSUBJECT|PORTAGE_ELOG_COMMAND)" ) ) ) {

				if ( rxLine.indexIn( line ) > -1 ) {
					QString keyword = rxLine.cap(1);
					if ( !keywords[ keyword ].isEmpty() ) {
						stream << keyword << "=\"" << keywords[ keyword ] << "\"\n";
					}
					keywords.take( keyword );
				}
			}
			else {
				stream << line << endl;
			}
		}

		// Add the rest (new) entries into make.conf
		for ( QMap<QString, QString>::Iterator it = keywords.begin(), end = keywords.end(); it != end; ++it ) {
			if ( !it.value().isEmpty() ) {
				stream << it.key() << "=\"" << it.value() << "\"\n";
			}
		}

		file.close();
		return true;
	}
	else {
		qCritical() << QString( "Writing: %1" ).arg( KurooConfig::fileMakeConf() );
		return false;
	}
}



