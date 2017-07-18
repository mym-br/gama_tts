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
#include <iostream>
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
#define VTM_CONFIG_FILE "vtm.config"



void
showUsage()
{
	std::cout << "\nGamaTTS " << GAMA_TTS_VERSION << "\n\n";
	std::cout << "Usage:\n\n";
	std::cout << PROGRAM_NAME << " --help\n";
	std::cout << PROGRAM_NAME << " --version\n";
	std::cout << "        Shows the program version and usage.\n\n";

	std::cout << PROGRAM_NAME << " tts [-v] config_dir input_text vtm_param_file output_file.wav\n";
	std::cout << "        Converts text to speech.\n\n";
	std::cout << "        -v : Verbose.\n";
	std::cout << "        config_dir : The directory containing the configuration files.\n";
	std::cout << "        input_text : Name of the file with the input text,\n";
	std::cout << "            or \"stdin\" to read from standard input.\n";
	std::cout << "        vtm_param_file : This file will be created, and will contain\n";
	std::cout << "            the parameters for the vocal tract model.\n";
	std::cout << "        output_file.wav : This file will be created, and will contain\n";
	std::cout << "            the synthesized speech.\n\n";

	std::cout << PROGRAM_NAME << " pho0 [-v] config_dir phonetic_string vtm_param_file output_file.wav\n";
	std::cout << "        Converts phonetic string to speech.\n\n";
	std::cout << "        -v : Verbose.\n";
	std::cout << "        config_dir : The directory containing the configuration files.\n";
	std::cout << "        phonetic_string : Name of the file with the phonetic string,\n";
	std::cout << "            or \"stdin\" to read from standard input.\n";
	std::cout << "        vtm_param_file : This file will be created, and will contain\n";
	std::cout << "            the parameters for the vocal tract model.\n";
	std::cout << "        output_file.wav : This file will be created, and will contain\n";
	std::cout << "            the synthesized speech.\n\n";

	std::cout << PROGRAM_NAME << " vtm [-v] config_dir voice_file vtm_param_file output_file.wav\n";
	std::cout << "        Converts vocal tract parameters to speech.\n\n";
	std::cout << "        -v : Verbose.\n";
	std::cout << "        config_dir : The directory containing the configuration files.\n";
	std::cout << "        voice_file : Name of the voice file, relative to config_dir.\n";
	std::cout << "        vtm_param_file : This file will be read. It must contain\n";
	std::cout << "            the parameters for the vocal tract model.\n";
	std::cout << "        output_file.wav : This file will be created, and will contain\n";
	std::cout << "            the synthesized speech.\n\n";
}

int
tts(int argc, char* argv[])
{
	const char* configDir    = nullptr;
	const char* textInput    = nullptr;
	const char* vtmParamFile = nullptr;
	const char* outputFile   = nullptr;

	if (argc == 6) {
		configDir    = argv[2];
		textInput    = argv[3];
		vtmParamFile = argv[4];
		outputFile   = argv[5];
	} else if ((argc == 7) && (strcmp("-v", argv[2]) == 0)) {
		GS::Log::debugEnabled = true;
		configDir    = argv[3];
		textInput    = argv[4];
		vtmParamFile = argv[5];
		outputFile   = argv[6];
	} else {
		showUsage();
		return EXIT_FAILURE;
	}

	std::ostringstream inputTextStream;
	if (strcmp(textInput, "stdin") == 0) {
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
		vtmControlModel->load(configDir, VTM_CONTROL_MODEL_CONFIG_FILE);

		auto vtmController = std::make_unique<GS::VTMControlModel::Controller>(configDir, *vtmControlModel);
		const GS::VTMControlModel::Configuration& vtmControlConfig = vtmController->vtmControlModelConfiguration();

		auto textParser = GS::VTMControlModel::TextParser::getInstance(configDir, vtmControlConfig);
		std::string phoneticString = textParser->parse(text.c_str());

		vtmController->synthesizePhoneticString(phoneticString, vtmParamFile, outputFile);

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
	const char* configDir     = nullptr;
	const char* phoneticInput = nullptr;
	const char* vtmParamFile  = nullptr;
	const char* outputFile    = nullptr;

	if (argc == 6) {
		configDir     = argv[2];
		phoneticInput = argv[3];
		vtmParamFile  = argv[4];
		outputFile    = argv[5];
	} else if ((argc == 7) && (strcmp("-v", argv[2]) == 0)) {
		GS::Log::debugEnabled = true;
		configDir     = argv[3];
		phoneticInput = argv[4];
		vtmParamFile  = argv[5];
		outputFile    = argv[6];
	} else {
		showUsage();
		return EXIT_FAILURE;
	}

	std::ostringstream inputPhoneticStream;
	if (strcmp(phoneticInput, "stdin") == 0) {
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
		vtmControlModel->load(configDir, VTM_CONTROL_MODEL_CONFIG_FILE);

		auto vtmController = std::make_unique<GS::VTMControlModel::Controller>(configDir, *vtmControlModel);
		vtmController->synthesizePhoneticString(phoneticString, vtmParamFile, outputFile);

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
	const char* configDir    = nullptr;
	const char* voiceFile    = nullptr;
	const char* vtmParamFile = nullptr;
	const char* outputFile   = nullptr;

	if (argc == 6) {
		configDir    = argv[2];
		voiceFile    = argv[3];
		vtmParamFile = argv[4];
		outputFile   = argv[5];
	} else if ((argc == 7) && (strcmp("-v", argv[2]) == 0)) {
		GS::Log::debugEnabled = true;
		configDir    = argv[3];
		voiceFile    = argv[4];
		vtmParamFile = argv[5];
		outputFile   = argv[6];
	} else {
		showUsage();
		return EXIT_FAILURE;
	}

	std::ifstream paramInputStream(vtmParamFile, std::ios_base::binary);
	if (!paramInputStream) {
		std::cerr << "Could not open the file " << vtmParamFile << '.' << std::endl;
		return EXIT_FAILURE;
	}

	try {
		std::ostringstream configFilePath;
		configFilePath << configDir << '/' << VTM_CONFIG_FILE;
		GS::ConfigurationData vtmConfigData{configFilePath.str()};

		std::ostringstream voiceFilePath;
		voiceFilePath << configDir << '/' << voiceFile;
		vtmConfigData.insert(GS::ConfigurationData{voiceFilePath.str()});

		auto vtm = GS::VTM::VocalTractModel::getInstance(vtmConfigData);
		vtm->synthesizeToFile(paramInputStream, outputFile);

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
		showUsage();
		return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "tts") == 0) {
		return tts(argc, argv);
	} else if (strcmp(argv[1], "pho0") == 0) {
		return pho0(argc, argv);
	} else if (strcmp(argv[1], "vtm") == 0) {
		return vtm(argc, argv);
	} else if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "--help") == 0) {
		showUsage();
		return EXIT_SUCCESS;
	}

	showUsage();
	return EXIT_FAILURE;
}
