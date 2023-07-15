/***************************************************************************
 *  Copyright 2014 Marcelo Y. Matuda                                       *
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

#ifndef TEXT_H_
#define TEXT_H_

#include <sstream>
#include <string>

#include "Exception.h"



namespace GS {
namespace Text {

std::string trim(const std::string& s);
template<typename T> T parseString(const std::string& s);

inline bool isAscii(unsigned char c) { return c < 128U; }
bool isNonprintableAscii(unsigned char c);



template<typename T>
T
parseString(const std::string& s) {
	std::istringstream in(s);
	T res;
	in >> res;
	if (!in) {
		THROW_EXCEPTION(InvalidValueException, "Wrong format: " << s << '.');
	}
	if (!in.eof()) {
		THROW_EXCEPTION(InvalidValueException, "Invalid text at the end of: " << s << '.');
	}
	return res;
}

} /* namespace Text */
} /* namespace GS */

#endif /* TEXT_H_ */
