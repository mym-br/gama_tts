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

#ifndef RAPIDXML_UTIL_H_
#define RAPIDXML_UTIL_H_

#include <string>

#include "rapidxml.hpp"

#include "Exception.h"



namespace GS {
namespace RapidXmlUtil {

std::string readXMLFile(const std::string& filePath);

inline
rapidxml::xml_node<char>*
firstChild(rapidxml::xml_node<char>* elem)
{
	return elem->first_node();
}

inline
rapidxml::xml_node<char>*
firstChild(rapidxml::xml_node<char>* elem, const std::string& name)
{
	return elem->first_node(&name[0], name.size());
}

inline
rapidxml::xml_node<char>*
nextSibling(rapidxml::xml_node<char>* elem)
{
	return elem->next_sibling();
}

inline
rapidxml::xml_node<char>*
nextSibling(rapidxml::xml_node<char>* elem, const std::string& name)
{
	return elem->next_sibling(&name[0], name.size());
}

inline
const char*
attributeValue(rapidxml::xml_node<char>* elem, const std::string& name, bool optional=false)
{
	rapidxml::xml_attribute<char>* attr = elem->first_attribute(&name[0], name.size());
	if (attr == 0) {
		if (optional) {
			return "";
		} else {
			THROW_EXCEPTION(XMLException, "\"" << name << "\" attribute not found in \"" << elem->name() << "\" element.");
		}
	}
	return attr->value();
}

// Returns true if the names are equal.
inline
bool
compareElementName(rapidxml::xml_node<char>* elem, const std::string& name)
{
	return rapidxml::internal::compare(elem->name(), elem->name_size(), &name[0], name.size(), true);
}

} /* namespace RapidXmlUtil */
} /* namespace GS */

#endif /*RAPIDXML_UTIL_H_*/
