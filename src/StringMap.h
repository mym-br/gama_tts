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

#ifndef GS_STRING_MAP_H_
#define GS_STRING_MAP_H_

#include <string>
#include <unordered_map>

namespace GS {

class StringMap {
public:
	StringMap() = default;
	~StringMap() = default;

	void load(const char* filePath);
	const char* getEntry(const char* key) const;
private:
	StringMap(const StringMap&) = delete;
	StringMap& operator=(const StringMap&) = delete;
	StringMap(StringMap&&) = delete;
	StringMap& operator=(StringMap&&) = delete;

	std::unordered_map<std::string, std::string> map_;
};

} /* namespace GS */

#endif /* GS_STRING_MAP_H_ */
