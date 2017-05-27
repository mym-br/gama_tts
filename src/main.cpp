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



void
showUsage(const char* programName)
{
	std::cout << "\nGamaTTS " << PROGRAM_VERSION << "\n\n";
	std::cout << "Usage:\n\n";
	std::cout << programName << " --version\n";
	std::cout << "        Shows the program version.\n\n";
	std::cout << programName << " [-v] -c config_dir -p vtm_param_file.txt -o output_file.wav \"Hello world.\"\n";
	std::cout << "        Synthesizes text from the command line.\n";
	std::cout << "        -v : verbose\n\n";
	std::cout << programName << " [-v] -c config_dir -i input_text.txt -p vtm_param_file.txt -o output_file.wav\n";
	std::cout << "        Synthesizes text from a file.\n";
	std::cout << "        -v : verbose\n" << std::endl;
}

int
main(int argc, char* argv[])
{
	if (argc < 2) {
		showUsage(argv[0]);
		return EXIT_FAILURE;
	}

	const char* configDirPath = nullptr;
	const char* inputFile = nullptr;
	const char* outputFile = nullptr;
	const char* vtmParamFile = nullptr;
	std::ostringstream inputTextStream;

	int i = 1;
	while (i < argc) {
		if (strcmp(argv[i], "-v") == 0) {
			++i;
			GS::Log::debugEnabled = true;
		} else if (strcmp(argv[i], "-c") == 0) {
			++i;
			if (i == argc) {
				showUsage(argv[0]);
				return EXIT_FAILURE;
			}
			configDirPath = argv[i];
			++i;
		} else if (strcmp(argv[i], "-i") == 0) {
			++i;
			if (i == argc) {
				showUsage(argv[0]);
				return EXIT_FAILURE;
			}
			inputFile = argv[i];
			++i;
		} else if (strcmp(argv[i], "-p") == 0) {
			++i;
			if (i == argc) {
				showUsage(argv[0]);
				return EXIT_FAILURE;
			}
			vtmParamFile = argv[i];
			++i;
		} else if (strcmp(argv[i], "-o") == 0) {
			++i;
			if (i == argc) {
				showUsage(argv[0]);
				return EXIT_FAILURE;
			}
			outputFile = argv[i];
			++i;
		} else if (strcmp(argv[i], "--version") == 0) {
			++i;
			showUsage(argv[0]);
			return EXIT_SUCCESS;
		} else {
			for ( ; i < argc; ++i) {
				inputTextStream << argv[i] << ' ';
			}
		}
	}

	if (configDirPath == nullptr || vtmParamFile == nullptr || outputFile == nullptr) {
		showUsage(argv[0]);
		return EXIT_FAILURE;
	}

	if (inputFile != nullptr) {
		std::ifstream in(inputFile, std::ios_base::binary);
		if (!in) {
			std::cerr << "Error: Could not open the file " << inputFile << '.' << std::endl;
			return EXIT_FAILURE;
		}
		std::string line;
		while (std::getline(in, line)) {
			inputTextStream << line << ' ';
		}
	}
	std::string inputText = inputTextStream.str();
	if (inputText.empty()) {
		std::cerr << "Error: Empty input text." << std::endl;
		return EXIT_FAILURE;
	}
	if (GS::Log::debugEnabled) {
		std::cout << "INPUT TEXT [" << inputText << ']' << std::endl;
	}

	try {
		auto vtmControlModel = std::make_unique<GS::VTMControlModel::Model>();
		vtmControlModel->load(configDirPath, VTM_CONTROL_MODEL_CONFIG_FILE);

		auto vtmController = std::make_unique<GS::VTMControlModel::Controller>(configDirPath, *vtmControlModel);
		const GS::VTMControlModel::Configuration& vtmControlConfig = vtmController->vtmControlModelConfiguration();

		auto textParser = GS::VTMControlModel::TextParser::getInstance(configDirPath, vtmControlConfig);
		std::string phoneticString = textParser->parse(inputText.c_str());

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
