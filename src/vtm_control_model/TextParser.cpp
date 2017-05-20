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
#include "english/EnglishTextParser.h"

namespace GS {
namespace VTMControlModel {

std::unique_ptr<TextParser>
TextParser::getInstance(const std::string& language,
		const std::string& configDirPath,
		const std::string& dictionary1Path,
		const std::string& dictionary2Path,
		const std::string& dictionary3Path)
{
	if (language == "english") {
		return std::make_unique<English::EnglishTextParser>(configDirPath, dictionary1Path, dictionary2Path, dictionary3Path);
	} else {
		THROW_EXCEPTION(InvalidValueException, "[TextParser] Invalid language: " << language << '.');
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */
