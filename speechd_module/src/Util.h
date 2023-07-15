/***************************************************************************
 *  Copyright 2015, 2017 Marcelo Y. Matuda                                 *
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

#ifndef UTIL_H_
#define UTIL_H_

#include <sstream>
#include <string>
#include <string_view>
#include <utility> /* make_pair, pair */



namespace Util {

void stripSSML(std::string& msg);
std::pair<std::string, std::string> getNameAndValue(const std::string& s);
void removeLF(std::string& msg);
template<typename T> T convertString(const std::string& s);
bool compare(const std::string_view& s1, const std::string& s2, std::string::size_type s2Index);
template<typename T> T bound(T min, T value, T max);



template<typename T>
T
convertString(const std::string& s) {
	std::istringstream in(s);
	T res;
	in >> res;
	if (!in) return 0;
	return res;
}

template<typename T>
T
bound(T min, T value, T max)
{
	if (value > max) {
		return max;
	} else if (value < min) {
		return min;
	}
	return value;
}

} // namespace Util

#endif /* UTIL_H_ */
