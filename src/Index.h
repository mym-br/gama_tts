/***************************************************************************
 *  Copyright 2022 Marcelo Y. Matuda                                       *
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

#ifndef GS_INDEX_H
#define GS_INDEX_H

#include <string>

#include "ConfigurationData.h"



namespace GS {

class Index {
public:
	Index(const std::string& configDirPath);

	std::string entry(const std::string& key) const;
private:
	std::string configDirPath_;
	ConfigurationData data_;
};

} // namespace GS

#endif // GS_INDEX_H
