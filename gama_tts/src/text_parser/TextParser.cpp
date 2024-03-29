/***************************************************************************
 *  Copyright 2017, 2021 Marcelo Y. Matuda                                 *
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

#include "TextParser.h"

#include <sstream>

#include "ConfigurationData.h"
#include "english/EnglishTextParser.h"
#include "Exception.h"
#include "ExternalTextParser.h"
#include "Index.h"

#define CONFIG_FILE_NAME "text_parser.txt"



namespace GS {
namespace TextParser {

TextParserConfiguration::TextParserConfiguration(const Index& index)
{
	ConfigurationData config(index.entry("text_parser_dir") + CONFIG_FILE_NAME);

	language        = config.value<std::string>("language");
	dictionary1File = config.value<std::string>("dictionary_1_file");
	dictionary2File = config.value<std::string>("dictionary_2_file");
	dictionary3File = config.value<std::string>("dictionary_3_file");

	const int modeIndex = config.value<int>("mode");
	switch (modeIndex) {
	case 0: mode = TextParser::Mode::normal  ; break;
	case 1: mode = TextParser::Mode::emphasis; break;
	case 2: mode = TextParser::Mode::letter  ; break;
	default:
		THROW_EXCEPTION(InvalidValueException, "Invalid text parser mode: " << modeIndex << '.');
	}
}

std::unique_ptr<TextParser>
TextParser::getInstance(const Index& index, VTMControlModel::PhoneticStringFormat phoStrFormat)
{
	if (phoStrFormat == VTMControlModel::PhoneticStringFormat::mbrola) {
		return std::make_unique<ExternalTextParser>(index);
	} else {
		TextParserConfiguration textParserConfig{index};
		if (textParserConfig.language == "english") {
			return std::make_unique<English::EnglishTextParser>(index, textParserConfig);
		} else {
			THROW_EXCEPTION(InvalidValueException, "[TextParser] Invalid language: " << textParserConfig.language << '.');
		}
	}
}

} /* namespace TextParser */
} /* namespace GS */
