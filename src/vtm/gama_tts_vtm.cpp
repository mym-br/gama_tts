/***************************************************************************
 *  Copyright 1991, 1992, 1993, 1994, 1995, 1996, 2001, 2002               *
 *    David R. Hill, Leonard Manzara, Craig Schock                         *
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
// 2014-09
// This file was copied from Gnuspeech and modified by Marcelo Y. Matuda.

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include "ConfigurationData.h"
#include "global.h"
#include "Log.h"
#include "VocalTractModel.h"



int
main(int argc, char* argv[])
{
	using namespace GS;

	const char* configFile = nullptr;
	const char* voiceFile = nullptr;
	const char* paramFile = nullptr;
	const char* outputFile = nullptr;

	/*  PARSE THE COMMAND LINE  */
	if (argc == 5) {
		configFile = argv[1];
		voiceFile = argv[2];
		paramFile = argv[3];
		outputFile = argv[4];
	} else if ((argc == 6) && (strcmp("-v", argv[1]) == 0)) {
		Log::debugEnabled = true;
		configFile = argv[2];
		voiceFile = argv[3];
		paramFile = argv[4];
		outputFile = argv[5];
	} else {
		std::cout << "\nGamaTTS VTM " << PROGRAM_VERSION << "\n\n";
		std::cerr << "Usage: " << argv[0] << " [-v] config_file voice_file param_file output_file.wav\n";
		std::cout << "         -v : verbose\n" << std::endl;
		return EXIT_FAILURE;
	}

	std::ifstream inputStream(paramFile, std::ios_base::binary);
	if (!inputStream) {
		std::cerr << "Could not open the file " << paramFile << '.' << std::endl;
		return EXIT_FAILURE;
	}

	ConfigurationData vtmConfigData{voiceFile};
	vtmConfigData.insert(ConfigurationData(configFile));

	auto vtm = VTM::VocalTractModel::getInstance(vtmConfigData);
	vtm->synthesizeToFile(inputStream, outputFile);

	LOG_DEBUG("\nWrote scaled samples to file: " << outputFile);

	return EXIT_SUCCESS;
}
