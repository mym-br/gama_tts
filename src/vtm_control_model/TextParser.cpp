/***************************************************************************
 *  Copyright 2017 Marcelo Y. Matuda                                       *
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

#include "Exception.h"
#include "VTMControlModelConfiguration.h"
#include "english/EnglishTextParser.h"

namespace GS {
namespace VTMControlModel {

std::unique_ptr<TextParser>
TextParser::getInstance(const std::string& configDirPath, const Configuration& config)
{
	TextParser::Mode textParserMode;
	switch (config.textParserMode) {
	case 0: textParserMode = MODE_NORMAL  ; break;
	case 1: textParserMode = MODE_EMPHASIS; break;
	case 2: textParserMode = MODE_LETTER  ; break;
	default:
		THROW_EXCEPTION(InvalidValueException, "Invalid text parser mode: " << config.textParserMode << '.');
	}

	if (config.language == "english") {
		return std::make_unique<English::EnglishTextParser>(
							configDirPath,
							config.dictionary1File,
							config.dictionary2File,
							config.dictionary3File,
							textParserMode);
	} else {
		THROW_EXCEPTION(InvalidValueException, "[TextParser] Invalid language: " << config.language << '.');
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */
