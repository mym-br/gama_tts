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

#include "Index.h"



namespace GS {

Index::Index(const std::string& configDirPath)
		: configDirPath_(configDirPath.back() == '/' ? configDirPath : configDirPath + '/')
		, data_(configDirPath_ + "_index.txt")
{
}

std::string
Index::entry(const std::string& key) const
{
	return configDirPath_ + data_.value<std::string>(key);
}

} // namespace GS
