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

#ifndef EXTERNAL_TEXT_PARSER_H_
#define EXTERNAL_TEXT_PARSER_H_

#include <string>

#include "TextParser.h"



namespace GS {
namespace TextParser {

class ExternalTextParser : public TextParser {
public:
	ExternalTextParser(const std::string& configDirPath);
	virtual ~ExternalTextParser() = default;

	virtual std::string parse(const char* text);
	virtual void setMode(Mode mode);
private:
	ExternalTextParser(const ExternalTextParser&) = delete;
	ExternalTextParser& operator=(const ExternalTextParser&) = delete;
	ExternalTextParser(ExternalTextParser&&) = delete;
	ExternalTextParser& operator=(ExternalTextParser&&) = delete;

	std::string inputFilePath_;
	std::string outputFilePath_;
	std::string command_;
};

} /* namespace TextParser */
} /* namespace GS */

#endif /* EXTERNAL_TEXT_PARSER_H_ */
