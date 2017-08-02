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
	std::cout << "\nGamaTTS " << GAMA_TTS_VERSION << "\n\n";
	std::cout << "Usage:\n\n";
	std::cout << PROGRAM_NAME << " --help\n";
	std::cout << PROGRAM_NAME << " --version\n";
	std::cout << "    Shows the program version and usage.\n\n";

	std::cout << PROGRAM_NAME << " tts [-v] [-i input.txt] [-p vtm_param.txt] data_dir speech.wav\n";
	std::cout << "    Converts text to speech.\n\n";
	std::cout << "    data_dir   : The directory containing the data and configuration files.\n";
	std::cout << "    speech.wav : This file will be created, and will contain the\n";
	std::cout << "                 synthesized speech.\n\n";
	std::cout << "    Options:\n";
	std::cout << "    -v\n";
	std::cout << "        Verbose.\n";
	std::cout << "    -i input.txt\n";
	std::cout << "        Get the text from a file instead of from stdin.\n";
	std::cout << "    -p vtm_param.txt\n";
	std::cout << "        This file will be created, and will contain the parameters for the\n";
	std::cout << "        vocal tract model.\n\n";

	std::cout << PROGRAM_NAME << " pho0 [-v] [-i input.txt] [-p vtm_param.txt] data_dir speech.wav\n";
	std::cout << "    Converts phonetic string to speech.\n\n";
	std::cout << "    data_dir   : The directory containing the data and configuration files.\n";
	std::cout << "    speech.wav : This file will be created, and will contain the\n";
	std::cout << "                 synthesized speech.\n\n";
	std::cout << "    Options:\n";
	std::cout << "    -v\n";
	std::cout << "        Verbose.\n";
	std::cout << "    -i input.txt\n";
	std::cout << "        Get the phonetic string from a file instead of from stdin.\n";
	std::cout << "    -p vtm_param.txt\n";
	std::cout << "        This file will be created, and will contain the parameters for the\n";
	std::cout << "        vocal tract model.\n\n";

	std::cout << PROGRAM_NAME << " vtm [-v] data_dir vtm_param.txt speech.wav\n";
	std::cout << "    Converts vocal tract parameters to speech.\n\n";
	std::cout << "    data_dir      : The directory containing the data and configuration files.\n";
	std::cout << "    vtm_param.txt : The file with the parameters for the vocal tract model.\n";
	std::cout << "    speech.wav    : This file will be created, and will contain the\n";
	std::cout << "                    synthesized speech.\n\n";
	std::cout << "    Options:\n";
	std::cout << "    -v\n";
	std::cout << "        Verbose.\n\n";
}

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
	if (argc - i != 2) {
		showUsage(); return EXIT_FAILURE;
	}
	dataDir    = argv[i++];
	outputFile = argv[i];

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
		auto vtmControlModel = std::make_unique<GS::VTMControlModel::Model>();
		vtmControlModel->load(dataDir, VTM_CONTROL_MODEL_CONFIG_FILE);

		auto vtmController = std::make_unique<GS::VTMControlModel::Controller>(dataDir, *vtmControlModel);
		const GS::VTMControlModel::Configuration& vtmControlConfig = vtmController->vtmControlModelConfiguration();

		auto textParser = GS::VTMControlModel::TextParser::getInstance(dataDir, vtmControlConfig);
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

int
pho0(int argc, char* argv[])
{
	std::cout << PROGRAM_NAME << " pho0" << std::endl;

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
	if (argc - i != 2) {
		showUsage(); return EXIT_FAILURE;
	}
	dataDir    = argv[i++];
	outputFile = argv[i];

	std::ostringstream inputPhoneticStream;
	if (phoneticInput == nullptr) {
		std::string line;
		while (std::getline(std::cin, line)) {
			inputPhoneticStream << line << ' ';
		}
	} else {
		std::ifstream in(phoneticInput, std::ios_base::binary);
		if (!in) {
			std::cerr << "Error: Could not open the file " << phoneticInput << '.' << std::endl;
			return EXIT_FAILURE;
		}
		std::string line;
		while (std::getline(in, line)) {
			inputPhoneticStream << line << ' ';
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
		auto vtmControlModel = std::make_unique<GS::VTMControlModel::Model>();
		vtmControlModel->load(dataDir, VTM_CONTROL_MODEL_CONFIG_FILE);

		auto vtmController = std::make_unique<GS::VTMControlModel::Controller>(dataDir, *vtmControlModel);
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
		auto vtmControlModel = std::make_unique<GS::VTMControlModel::Model>();
		vtmControlModel->load(dataDir, VTM_CONTROL_MODEL_CONFIG_FILE);

		auto vtmController = std::make_unique<GS::VTMControlModel::Controller>(dataDir, *vtmControlModel);
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



int
main(int argc, char* argv[])
{
	if (argc < 2) {
		showUsage(); return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "tts") == 0) {
		return tts(argc, argv);
	} else if (strcmp(argv[1], "pho0") == 0) {
		return pho0(argc, argv);
	} else if (strcmp(argv[1], "vtm") == 0) {
		return vtm(argc, argv);
	} else if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "--help") == 0) {
		showUsage(); return EXIT_SUCCESS;
	}

	showUsage(); return EXIT_FAILURE;
}
