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

#include "Util.h"

namespace {

constexpr std::string_view  ltStr{"lt;"};
constexpr std::string_view  gtStr{"gt;"};
constexpr std::string_view ampStr{"amp;"};

} // namespace

namespace Util {

void
stripSSML(std::string& msg)
{
	std::string dest(msg.size(), ' ');

	std::string::size_type pos = 0;
	std::string::size_type destPos = 0;
	const std::string::size_type size = msg.size();
	while (pos < size) {
		if (msg[pos] == '<') {
			std::string::size_type findPos = msg.find('>', pos + 1);
			if (findPos == std::string::npos) {
				pos = size;
			} else {
				pos = findPos + 1;
			}
			continue;
		} else if (msg[pos] == '&') {
			const std::string::size_type nextPos = pos + 1;
			if (compare(ltStr, msg, nextPos)) {
				dest[destPos++] = '<';
				pos += ltStr.length() + 1;
			} else if (compare(gtStr, msg, nextPos)) {
				dest[destPos++] = '>';
				pos += gtStr.length() + 1;
			} else if (compare(ampStr, msg, nextPos)) {
				dest[destPos++] = '&';
				pos += ampStr.length() + 1;
			} else {
				++pos; // ignore
			}
			continue;
		}
		dest[destPos++] = msg[pos++];
	}

	// Ignore trailing spaces.
	while (destPos >= 1 && dest[destPos - 1] == ' ') --destPos;

	dest.resize(destPos);
	msg = std::move(dest);
}

std::pair<std::string, std::string>
getNameAndValue(const std::string& s)
{
	std::string::size_type pos = s.find('=');
	if (pos == std::string::npos || pos == s.size() - 1) {
		return std::make_pair(s, std::string{});
	}
	return std::make_pair(s.substr(0, pos), s.substr(pos + 1, s.size() - pos - 1));
}

void
removeLF(std::string& msg)
{
	for (char& c : msg) {
		if (c == '\n') {
			c = ' ';
		}
	}
}

bool
compare(const std::string_view& s1, const std::string& s2, std::string::size_type s2Index)
{
	if (s2Index >= s2.length()) return false;
	if (s2Index + s1.length() > s2.length()) {
		return false;
	}
	auto it1 = s1.cbegin();
	auto it2 = s2.cbegin() + s2Index;
	while (it1 != s1.cend()) {
		if (*it1 != *it2) return false;
		++it1;
		++it2;
	}
	return true;
}

} // namespace Util
