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

#ifndef TEXT_PARSER_H_
#define TEXT_PARSER_H_

#include <memory>
#include <string>

#include "VTMControlModelConfiguration.h"



namespace GS {

class Index;

namespace TextParser {

class TextParser {
public:
	enum class Mode {
		normal,
		emphasis,
		letter
	};

	TextParser() = default;
	virtual ~TextParser() = default;

	virtual std::string parse(const char* text) = 0;
	virtual void setMode(Mode mode) = 0;

	static std::unique_ptr<TextParser> getInstance(const Index& index, VTMControlModel::PhoneticStringFormat phoStrFormat);
private:
	TextParser(const TextParser&) = delete;
	TextParser& operator=(const TextParser&) = delete;
	TextParser(TextParser&&) = delete;
	TextParser& operator=(TextParser&&) = delete;
};

struct TextParserConfiguration {
	std::string language;
	std::string dictionary1File;
	std::string dictionary2File;
	std::string dictionary3File;
	TextParser::Mode mode;

	explicit TextParserConfiguration(const Index& index);
};

} /* namespace TextParser */
} /* namespace GS */

#endif /* TEXT_PARSER_H_ */
