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

#include "StringMap.h"

#include <cstddef> /* std::size_t */
#include <fstream>
//#include <iostream>

#include "Exception.h"

#define SEPARATORS " \t"
#define COMMENT_CHAR '#'



namespace GS {

void
StringMap::load(const char* filePath)
{
	map_.clear();

	std::ifstream in{filePath, std::ios_base::binary};
	if (!in) {
		THROW_EXCEPTION(IOException, "Could not open the file " << filePath << '.');
	}

	std::string line;
	std::size_t lineNumber = 0;
	while (std::getline(in, line)) {
		++lineNumber;

		// Get first string.
		std::size_t pos1 = line.find_first_not_of(SEPARATORS);
		if (pos1 == std::string::npos) {
			continue; // empty line
		}
		if (line[pos1] == COMMENT_CHAR) {
			continue; // comment
		}
		std::size_t pos2 = line.find_first_of(SEPARATORS, pos1 + 1);
		if (pos2 == std::string::npos) {
			THROW_EXCEPTION(IOException, "Separator not found (file: " << filePath << " line: " << lineNumber << ").");
		}
		std::string string1 = line.substr(pos1, pos2 - pos1);

		// Get second string.
		pos1 = line.find_first_not_of(SEPARATORS, pos2 + 1);
		if (pos1 == std::string::npos) {
			THROW_EXCEPTION(IOException, "Second string not found (file: " << filePath << " line: " << lineNumber << ").");
		}
		pos2 = line.find_first_of(SEPARATORS, pos1 + 1);
		if (pos2 == std::string::npos) {
			pos2 = line.size();
		}
		std::string string2 = line.substr(pos1, pos2 - pos1);

		//std::cout << "STRING_MAP string1=[" << string1 << "] string2=[" << string2 << ']' << std::endl;

		auto iter = map_.find(string1);
		if (iter == map_.end()) {
			map_[string1] = string2;
		} else {
			THROW_EXCEPTION(IOException, "Duplicate key: [" << string1 << "].");
		}
	}
}

const char*
StringMap::getEntry(const char* key) const
{
	if (map_.empty()) {
		return nullptr;
	}

	auto iter = map_.find(key);
	if (iter == map_.end()) {
		return nullptr;
	} else {
		return iter->second.c_str();
	}
}

} /* namespace GS */
