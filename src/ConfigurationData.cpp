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

#include "ConfigurationData.h"

#include <cctype> /* isspace */
#include <fstream>
#include <limits>



namespace {

const char SEPARATOR_CHAR = '=';
const char COMMENT_CHAR = '#';

[[noreturn]] void
throwException(const std::string& filePath, int lineNumber, const char* message)
{
	THROW_EXCEPTION(GS::ParsingException, "[ConfigurationData] Error in file " << filePath << " (line " << lineNumber << "): " << message << '.');
}

template<typename T>
[[noreturn]] void
throwException(const std::string& filePath, int lineNumber, const char* message, const T& complement)
{
	THROW_EXCEPTION(GS::ParsingException, "[ConfigurationData] Error in file " << filePath << " (line " << lineNumber << "): " << message << complement << '.');
}

} /* namespace */

//==============================================================================

namespace GS {

/*******************************************************************************
 * Constructor.
 */
ConfigurationData::ConfigurationData(const std::string& filePath)
		: filePath_(filePath)
{
	typedef Map::iterator MI;
	typedef Map::value_type VT;

	std::ifstream in(filePath.c_str(), std::ios_base::binary);
	if (!in) THROW_EXCEPTION(IOException, "Could not open the file: " << filePath << '.');

	std::string line;
	int lineNum = 0;
	while (getline(in, line)) {
		++lineNum;

		if (line.empty()) continue;

		auto iter = line.begin();

		// Comment (must start at the beginning of the line).
		if (*iter == COMMENT_CHAR) {
			continue;
		}

		// Read key.
		if (std::isspace(*iter)) throwException(filePath, lineNum, "Space at the beginning of the line");
		while (iter != line.end() && !std::isspace(*iter) && *iter != SEPARATOR_CHAR) {
			++iter;
		}
		if (iter == line.end()) throwException(filePath, lineNum, "Key not found");
		std::string key(line.begin(), iter);
		if (key.empty()) throwException(filePath, lineNum, "Empty key");

		// Separator.
		while (iter != line.end() && std::isspace(*iter)) ++iter; // skip spaces
		if (iter == line.end() || *iter != SEPARATOR_CHAR) throwException(filePath, lineNum, "Missing separator");
		++iter;

		// Read value.
		while (iter != line.end() && std::isspace(*iter)) ++iter; // skip spaces
		if (iter == line.end()) throwException(filePath, lineNum, "Value not found");
		std::string value(iter, line.end());
		if (value.empty()) throwException(filePath, lineNum, "Empty value");

		std::pair<MI, bool> res = valueMap_.insert(VT(key, value));
		if (!res.second) {
			throwException(filePath, lineNum, "Duplicate key: ", key);
		}
	}
}

/*******************************************************************************
 * Destructor.
 */
ConfigurationData::~ConfigurationData()
{
}



template<>
int
ConfigurationData::convertString<int>(const std::string& s)
{
	return std::stoi(s);
}

template<>
unsigned int
ConfigurationData::convertString<unsigned int>(const std::string& s)
{
	unsigned long v = std::stoul(s);
	if (v > std::numeric_limits<unsigned int>::max()) {
		throw std::out_of_range("ConfigurationData::convertString<unsigned int>");
	}
	return static_cast<unsigned int>(v);
}

template<>
long
ConfigurationData::convertString<long>(const std::string& s)
{
	return std::stol(s);
}

template<>
unsigned long
ConfigurationData::convertString<unsigned long>(const std::string& s)
{
	return std::stoul(s);
}

template<>
float
ConfigurationData::convertString<float>(const std::string& s)
{
	return std::stof(s);
}

template<>
double
ConfigurationData::convertString<double>(const std::string& s)
{
	return std::stod(s);
}

template<>
std::string
ConfigurationData::convertString<std::string>(const std::string& s)
{
	return s;
}

template<>
bool
ConfigurationData::convertString<bool>(const std::string& s)
{
	return (s == "true") ? true : false;
}

template<typename T>
void
ConfigurationData::convertValue(const T& value, std::string& s)
{
	std::ostringstream out;
	out << value;
	s = out.str();
}

template<>
void
ConfigurationData::convertValue<float>(const float& value, std::string& s)
{
	std::ostringstream out;
	out.precision(9);
	out << value;
	s = out.str();
}

template<>
void
ConfigurationData::convertValue<double>(const double& value, std::string& s)
{
	std::ostringstream out;
	out.precision(17);
	out << value;
	s = out.str();
}

template<>
void
ConfigurationData::convertValue<std::string>(const std::string& value, std::string& s)
{
	s = value;
}

template<>
void
ConfigurationData::convertValue<bool>(const bool& value, std::string& s)
{
	s = value ? "true" : "false";
}

void
ConfigurationData::put(const std::string& key, const char* value)
{
	std::string& valueString = valueMap_[key];
	convertValue(std::string(value), valueString);
}

ConfigurationData&
ConfigurationData::insert(const ConfigurationData& other)
{
	for (auto& item : other.valueMap_) {
		put(item.first, item.second);
	}
	return *this;
}

} /* namespace GS */
