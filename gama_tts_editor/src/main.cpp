/***************************************************************************
 *  Copyright 2014, 2017 Marcelo Y. Matuda                                 *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include <xmmintrin.h> /* SSE */
#include <pmmintrin.h> /* SSE3 */

#include <iostream>
#include <locale>

#include <QApplication>
#include <QIcon>
#include <QLocale>

#include "Log.h"
#include "MainWindow.h"



int
main(int argc, char* argv[])
{
	// Disable denormals.
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);         // requires xmmintrin.h
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON); // requires pmmintrin.h

	GS::Log::debugEnabled = true;

	try {
		QApplication app(argc, argv);
		app.setWindowIcon(QIcon{":/img/window_icon.png"});

		QCoreApplication::setOrganizationName("GamaTTS");
		QCoreApplication::setOrganizationDomain("gamatts.org");
		QCoreApplication::setApplicationName("GamaTTS_Editor");

		// Force "C" locale.
		QLocale::setDefault(QLocale::c());
		std::locale::global(std::locale::classic());

		GS::MainWindow w;
		w.show();
		app.exec();

		return EXIT_SUCCESS;

	} catch (std::exception& e) {
		std::cerr << "Caught exception: " << e.what() << '.' << std::endl;
	} catch (...) {
		std::cerr << "Caught unexpected exception." << std::endl;
	}

	return EXIT_FAILURE;
}
