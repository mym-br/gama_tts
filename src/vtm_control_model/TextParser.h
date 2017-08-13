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

#ifndef VTM_CONTROL_MODEL_TEXT_PARSER_H_
#define VTM_CONTROL_MODEL_TEXT_PARSER_H_

#include <memory>
#include <string>

namespace GS {
namespace VTMControlModel {

struct Configuration;

class TextParser {
public:
	enum class Mode {
		normal,
		emphasis,
		letter
	};

	virtual ~TextParser() {}

	virtual std::string parse(const char* text) = 0;
	virtual void setMode(Mode mode) = 0;

	static std::unique_ptr<TextParser> getInstance(const std::string& configDirPath, const Configuration& config);
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_TEXT_PARSER_H_ */
