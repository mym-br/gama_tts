/***************************************************************************
 *  Copyright 2021 Marcelo Y. Matuda                                       *
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

#include "ExternalTextParser.h"

#include <cstdlib> /* system */
#include <fstream>
#include <sstream>

#include "ConfigurationData.h"
#include "Exception.h"

#define CONFIG_FILE_NAME "external_text_parser.txt"
#define TEXT_PARSER_DIR "/text_parser/"



namespace GS {
namespace TextParser {

ExternalTextParser::ExternalTextParser(const std::string& configDirPath)
{
	std::ostringstream configFilePath;
	configFilePath << configDirPath << TEXT_PARSER_DIR << CONFIG_FILE_NAME;
	ConfigurationData config(configFilePath.str());

	inputFilePath_  = configDirPath + '/' + config.value<std::string>("input_file");
	outputFilePath_ = configDirPath + '/' + config.value<std::string>("output_file");
	command_        = configDirPath + '/' + config.value<std::string>("command");
}

std::string
ExternalTextParser::parse(const char* text)
{
	{
		std::ofstream inputFile{inputFilePath_, std::ios_base::binary};
		if (!inputFile) {
			THROW_EXCEPTION(IOException, "Could not open the file " << inputFilePath_ << '.');
		}
		inputFile << text;
	}

	int retVal = std::system(command_.c_str());
	if (retVal != 0) {
		THROW_EXCEPTION(IOException, "Execution of command " << command_ << " failed.");
	}

	std::ifstream outputFile(outputFilePath_, std::ios_base::binary);
	if (!outputFile) {
		THROW_EXCEPTION(IOException, "Could not open the file " << outputFilePath_ << '.');
	}
	std::string line;
	std::ostringstream phoStr;
	while (std::getline(outputFile, line)) {
		phoStr << line << '\n';
	}
	return phoStr.str();
}

void
ExternalTextParser::setMode(Mode /*mode*/)
{
	// Ignore.
}

} /* namespace TextParser */
} /* namespace GS */
