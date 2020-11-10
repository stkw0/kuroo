/*
 *   Copyright (C) 2005 by Karye
 *   karye@users.sourceforge.net
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "settings.h"
#include "kuroo.h"

#include <stdlib.h>

#include <KUniqueApplication>
#include <KAboutData>

#include <KLocale>
#include <KDebug>
#include <QCommandLineParser>

static const char version[] = "kuroo-17.12.0";

int main( int argc, char **argv )
{
    QApplication app(argc, argv); // PORTING SCRIPT: move this to before the K4AboutData initialization
	KAboutData about("kuroo", i18n("Kuroo"), version, i18n("Frontend to Gentoo Portage"),
	KAboutLicense::LicenseKey::GPL, i18n("(C) 2006 karye") ); //, 0, 0, "info@kuroo.org" new email ?
	about.addAuthor(i18n("Andrew Schenck"), i18n("Maintainer"), "galiven@users.sourceforge.net");
	about.addAuthor(i18n("Karye"), i18n("Original author and maintainer"), "info@kuroo.org");
	about.addAuthor(i18n("David C. Manuelda"), i18n("Previous developer and maintainer"), "StormByte@gmail.com");
	about.addAuthor(i18n("Matthias Faconneau"), i18n("Developer, port to KDE4"), "faconneau@users.sourceforge.net");
	about.addAuthor(i18n("Detlev Casanova"), i18n("Developer, port to KDE4"), "cazou88@users.sourceforge.net");
	about.addCredit(i18n("Gombault Damien"), i18n("French translation"), "desintegr@gmail.com");
	about.addCredit(i18n("Jan Schnackenberg"), i18n("German translation"), "jan@schnackenberg.org");
	about.addCredit(i18n("Alexander Reiterer"), i18n("German translation"), "alexander.reiterer@tuwien.ac.at");
	about.addCredit(i18n("Martin Baranski"), i18n("German translation"), "eagle@eagle-cage.de");
	about.addCredit(i18n("Matteo Azzali"), i18n("Italian translation"), "kaioth@tiscalinet.it");
	about.addCredit(i18n("Alexander N. Sørnes"), i18n("Norwegian translation"), "alex@thehandofagony.com");
	about.addCredit(i18n("Konrad Mantorski"), i18n("Polish translation"), "konrad@mantorski.com");
	about.addCredit(i18n("Wolfgang Bartelme"), i18n("Kuroo icons"), "design@bartelme.at");
	about.addCredit(i18n("Jakob Petsovits"), i18n("Portage version code"), "jpetso@gmx.at");
	about.addCredit(i18n("Björn Balazs"), i18n("OpenUsability"), "B@lazs.de");
	about.addCredit(i18n("Florian Graessle"), i18n("OpenUsability"), "holehan@gmx.de");
	about.setHomepage("http://kuroo.sourceforge.net");
    QCommandLineParser parser;
    KAboutData::setApplicationData(about);
    parser.addVersionOption();
    parser.addHelpOption();
    //PORTING SCRIPT: adapt aboutdata variable if necessary
    about.setupCommandLine(&parser);
    parser.process(app); // PORTING SCRIPT: move this to after any parser.addOption
    about.processCommandLine(&parser);

	//KUniqueApplication app;

	qDebug() << "Kuroo version=" << version;

	KurooConfig::setHardVersion( version );
	//KurooConfig::writeConfig();

	// see if we are starting with session management
	if ( app.isSessionRestored() ) {
		RESTORE( Kuroo );
	} else {
		Kuroo *mainWindow = new Kuroo();
		mainWindow->setObjectName( "kuroo" );
		mainWindow->show();
		//app.connect( app, SIGNAL( lastWindowClosed() ), app, SLOT( quit() ) );
	}

	return app.exec();
}
