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

#include "RapidXmlUtil.h"

#include <fstream>



namespace GS {
namespace RapidXmlUtil {

std::string
readXMLFile(const char* filePath)
{
	std::ifstream in(filePath, std::ios_base::binary);
	if (!in) {
		THROW_EXCEPTION(XMLException, "Could not open the file: " << filePath << '.');
	}

	std::string source;
	in.seekg(0, std::ios::end);
	source.resize(static_cast<std::string::size_type>(in.tellg()));
	in.seekg(0, std::ios::beg);
	in.read(&source[0], source.size());
	in.close();
	if (source.empty()) {
		THROW_EXCEPTION(XMLException, "Empty XML file.");
	}

	return source;
}

} /* namespace RapidXmlUtil */
} /* namespace GS */
