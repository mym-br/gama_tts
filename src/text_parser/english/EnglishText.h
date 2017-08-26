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

#ifndef ENGLISH_TEXT_H_
#define ENGLISH_TEXT_H_



namespace GS {
namespace English {
namespace Text {

// These functions work with UTF-8 encoding, but the result is only correct for ASCII characters.
inline bool isAscii(unsigned char c) { return c < 128U; }
bool isAlpha(unsigned char c);
bool isPrint(unsigned char c);
bool isUpper(unsigned char c);
bool isLower(unsigned char c);
bool isAlphaNum(unsigned char c);
char toUpper(char c);
char toLower(char c);

} /* namespace Text */
} /* namespace English */
} /* namespace GS */

#endif /* ENGLISH_TEXT_H_ */
