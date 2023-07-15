/***************************************************************************
 *  Copyright 2014, 2017, 2021 Marcelo Y. Matuda                           *
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

#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "Controller.h"
#include "Exception.h"
#include "global.h"
#include "Index.h"
#include "Log.h"
#include "Model.h"
#include "TextParser.h"
#include "VTMControlModelConfiguration.h"

#define PROGRAM_NAME "gama_tts"



bool
isOption(const char* arg)
{
	if (strlen(arg) > 0 && arg[0] == '-') {
		return true;
	} else {
		return false;
	}
}

void
showUsage()
{
	std::cout <<
		"\nGamaTTS " << GAMA_TTS_VERSION << "\n\n"

		"Usage:\n\n"

		PROGRAM_NAME << " --help\n"
		PROGRAM_NAME << " --version\n"
		"    Shows the program version and usage.\n\n"

		PROGRAM_NAME << " tts [-v] [-i input.txt] [-p vtm_param.txt] data_dir [speech.wav]\n"
		"    Converts text to speech.\n\n"
		"    data_dir   : The directory containing the data and configuration files.\n"
		"    speech.wav : This file will be created, and will contain the\n"
		"                 synthesized speech.\n\n"

		"    Options:\n"
		"    -v\n"
		"        Verbose.\n"
		"    -i input.txt\n"
		"        Get the text from a file instead of from stdin.\n"
		"    -p vtm_param.txt\n"
		"        This file will be created, and will contain the parameters for the\n"
		"        vocal tract model.\n\n"

		PROGRAM_NAME << " pho [-v] [-i input.txt] [-p vtm_param.txt] data_dir [speech.wav]\n"
		"    Converts phonetic string to speech.\n\n"
		"    data_dir   : The directory containing the data and configuration files.\n"
		"    speech.wav : This file will be created, and will contain the\n"
		"                 synthesized speech.\n\n"

		"    Options:\n"
		"    -v\n"
		"        Verbose.\n"
		"    -i input.txt\n"
		"        Get the phonetic string from a file instead of from stdin.\n"
		"    -p vtm_param.txt\n"
		"        This file will be created, and will contain the parameters for the\n"
		"        vocal tract model.\n\n"

		PROGRAM_NAME << " vtm [-v] data_dir vtm_param.txt speech.wav\n"
		"    Converts vocal tract parameters to speech.\n\n"
		"    data_dir      : The directory containing the data and configuration files.\n"
		"    vtm_param.txt : The file with the parameters for the vocal tract model.\n"
		"    speech.wav    : This file will be created, and will contain the\n"
		"                    synthesized speech.\n\n"

		"    Options:\n"
		"    -v\n"
		"        Verbose.\n\n"
		<< std::endl;
}

//==============================================================================

int
tts(int argc, char* argv[])
{
	std::cout << PROGRAM_NAME << " tts" << std::endl;

	const char* textInput    = nullptr;
	const char* vtmParamFile = nullptr;
	const char* dataDir      = nullptr;
	const char* outputFile   = nullptr;

	int i = 2;
	while (argc - i > 0 && isOption(argv[i])) {
		if (strcmp("-v", argv[i]) == 0) {
			GS::Log::debugEnabled = true;
		} else if (strcmp("-i", argv[i]) == 0) {
			++i;
			if (argc - i < 1) {
				showUsage(); return EXIT_FAILURE;
			}
			textInput = argv[i];
		} else if (strcmp("-p", argv[i]) == 0) {
			++i;
			if (argc - i < 1) {
				showUsage(); return EXIT_FAILURE;
			}
			vtmParamFile = argv[i];
		} else {
			showUsage(); return EXIT_FAILURE;
		}
		++i;
	}
	if (argc - i == 2) {
		dataDir    = argv[i++];
		outputFile = argv[i];
	} else if (argc - i == 1) {
		dataDir    = argv[i];
	} else {
		showUsage(); return EXIT_FAILURE;
	}

	std::ostringstream inputTextStream;
	if (textInput == nullptr) {
		std::string line;
		while (std::getline(std::cin, line)) {
			inputTextStream << line << ' ';
		}
	} else {
		std::ifstream in(textInput, std::ios_base::binary);
		if (!in) {
			std::cerr << "Error: Could not open the file " << textInput << '.' << std::endl;
			return EXIT_FAILURE;
		}
		std::string line;
		while (std::getline(in, line)) {
			inputTextStream << line << ' ';
		}
	}
	std::string text = inputTextStream.str();
	if (text.empty()) {
		std::cerr << "Error: Empty input text." << std::endl;
		return EXIT_FAILURE;
	}
	if (GS::Log::debugEnabled) {
		std::cout << "INPUT TEXT [" << text << ']' << std::endl;
	}

	try {
		const GS::Index index{dataDir};

		auto vtmControlModel = std::make_unique<GS::VTMControlModel::Model>();
		vtmControlModel->load(index);

		auto vtmController = std::make_unique<GS::VTMControlModel::Controller>(index, *vtmControlModel);
		auto textParser = GS::TextParser::TextParser::getInstance(
								index,
								vtmController->vtmControlModelConfiguration().phoStrFormat);
		std::string phoneticString = textParser->parse(text.c_str());
		vtmController->synthesizePhoneticStringToFile(phoneticString, vtmParamFile, outputFile);

	} catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	} catch (...) {
		std::cerr << "Unknown exception." << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

//==============================================================================

int
pho(int argc, char* argv[])
{
	std::cout << PROGRAM_NAME << " pho" << std::endl;

	const char* phoneticInput = nullptr;
	const char* vtmParamFile  = nullptr;
	const char* dataDir       = nullptr;
	const char* outputFile    = nullptr;

	int i = 2;
	while (argc - i > 0 && isOption(argv[i])) {
		if (strcmp("-v", argv[i]) == 0) {
			GS::Log::debugEnabled = true;
		} else if (strcmp("-i", argv[i]) == 0) {
			++i;
			if (argc - i < 1) {
				showUsage(); return EXIT_FAILURE;
			}
			phoneticInput = argv[i];
		} else if (strcmp("-p", argv[i]) == 0) {
			++i;
			if (argc - i < 1) {
				showUsage(); return EXIT_FAILURE;
			}
			vtmParamFile = argv[i];
		} else {
			showUsage(); return EXIT_FAILURE;
		}
		++i;
	}
	if (argc - i == 2) {
		dataDir    = argv[i++];
		outputFile = argv[i];
	} else if (argc - i == 1) {
		dataDir    = argv[i];
	} else {
		showUsage(); return EXIT_FAILURE;
	}

	std::ostringstream inputPhoneticStream;
	if (phoneticInput == nullptr) {
		std::string line;
		while (std::getline(std::cin, line)) {
			inputPhoneticStream << line << '\n';
		}
	} else {
		std::ifstream in(phoneticInput, std::ios_base::binary);
		if (!in) {
			std::cerr << "Error: Could not open the file " << phoneticInput << '.' << std::endl;
			return EXIT_FAILURE;
		}
		std::string line;
		while (std::getline(in, line)) {
			inputPhoneticStream << line << '\n';
		}
	}
	std::string phoneticString = inputPhoneticStream.str();
	if (phoneticString.empty()) {
		std::cerr << "Error: Empty phonetic string." << std::endl;
		return EXIT_FAILURE;
	}
	if (GS::Log::debugEnabled) {
		std::cout << "INPUT PHONETIC STRING [" << phoneticString << ']' << std::endl;
	}

	try {
		const GS::Index index{dataDir};

		auto vtmControlModel = std::make_unique<GS::VTMControlModel::Model>();
		vtmControlModel->load(index);

		auto vtmController = std::make_unique<GS::VTMControlModel::Controller>(index, *vtmControlModel);
		vtmController->synthesizePhoneticStringToFile(phoneticString, vtmParamFile, outputFile);

	} catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	} catch (...) {
		std::cerr << "Unknown exception." << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

//==============================================================================

int
vtm(int argc, char* argv[])
{
	std::cout << PROGRAM_NAME << " vtm" << std::endl;

	const char* dataDir      = nullptr;
	const char* vtmParamFile = nullptr;
	const char* outputFile   = nullptr;

	int i = 2;
	if (argc - i < 1) {
		showUsage(); return EXIT_FAILURE;
	}
	if (strcmp("-v", argv[i]) == 0) {
		GS::Log::debugEnabled = true;
		++i;
	}
	if (argc - i != 3) {
		showUsage(); return EXIT_FAILURE;
	}
	dataDir      = argv[i++];
	vtmParamFile = argv[i++];
	outputFile   = argv[i];
	if (isOption(dataDir) || isOption(vtmParamFile) || isOption(outputFile)) {
		showUsage(); return EXIT_FAILURE;
	}

	std::ifstream paramInputStream(vtmParamFile, std::ios_base::binary);
	if (!paramInputStream) {
		std::cerr << "Could not open the file " << vtmParamFile << '.' << std::endl;
		return EXIT_FAILURE;
	}

	try {
		const GS::Index index{dataDir};

		auto vtmControlModel = std::make_unique<GS::VTMControlModel::Model>();
		vtmControlModel->load(index);

		auto vtmController = std::make_unique<GS::VTMControlModel::Controller>(index, *vtmControlModel);
		vtmController->synthesizeToFile(paramInputStream, outputFile);

	} catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	} catch (...) {
		std::cerr << "Unknown exception." << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

//==============================================================================

int
main(int argc, char* argv[])
{
	if (argc < 2) {
		showUsage(); return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "tts") == 0) {
		return tts(argc, argv);
	} else if (strcmp(argv[1], "pho") == 0) {
		return pho(argc, argv);
	} else if (strcmp(argv[1], "vtm") == 0) {
		return vtm(argc, argv);
	} else if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "--help") == 0) {
		showUsage(); return EXIT_SUCCESS;
	}

	showUsage(); return EXIT_FAILURE;
}
