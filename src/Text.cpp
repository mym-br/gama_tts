/***************************************************************************
 *  Copyright 2015 Marcelo Y. Matuda                                       *
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

#include "Text.h"

#include <cctype> /* isalnum, isalpha, islower, isprint, isupper, tolower, toupper */



namespace GS {
namespace Text {

std::string
trim(const std::string& s)
{
	const char* charList = " \t";
	std::string::size_type firstPos = s.find_first_not_of(charList);
	if (firstPos == std::string::npos) {
		return std::string();
	}

	std::string::size_type lastPos = s.find_last_not_of(charList);
	if (firstPos == 0 && lastPos == s.size() - 1) {
		return s;
	} else {
		return s.substr(firstPos, lastPos - firstPos + 1);
	}
}

bool
isAlpha(unsigned char c)
{
	if (isAscii(c)) {
		return std::isalpha(c);
	} else {
		//NOTE: Non-ASCII characters are considered alphabetic.
		return true;
	}
}

bool
isPrint(unsigned char c)
{
	if (isAscii(c)) {
		return std::isprint(c);
	} else {
		//NOTE: Non-ASCII characters are considered printable.
		return true;
	}
}

bool
isUpper(unsigned char c)
{
	if (isAscii(c)) {
		return std::isupper(c);
	} else {
		//NOTE: Non-ASCII characters are considered lowercase.
		return false;
	}
}

bool
isLower(unsigned char c)
{
	if (isAscii(c)) {
		return std::islower(c);
	} else {
		//NOTE: Non-ASCII characters are considered lowercase.
		return true;
	}
}

bool
isAlphaNum(unsigned char c)
{
	if (isAscii(c)) {
		return std::isalnum(c);
	} else {
		//NOTE: Non-ASCII characters are considered alphanumeric.
		return true;
	}
}

char
toUpper(char c)
{
	if (isAscii(c)) {
		return std::toupper(static_cast<unsigned char>(c));
	} else {
		//NOTE: Non-ASCII characters are not converted to uppercase.
		return c;
	}
}

char
toLower(char c)
{
	if (isAscii(c)) {
		return std::tolower(static_cast<unsigned char>(c));
	} else {
		//NOTE: Non-ASCII characters are not converted to lowercase.
		return c;
	}
}

} /* namespace Text */
} /* namespace GS */
