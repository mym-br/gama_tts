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

#ifndef CONFIGURATION_DATA_H_
#define CONFIGURATION_DATA_H_

#include <sstream>
#include <string>
#include <unordered_map>

#include "Exception.h"



namespace GS {

// Format: "key = value"
class ConfigurationData {
public:
	ConfigurationData() {}
	ConfigurationData(const std::string& filePath);

	template<typename T> T value(const std::string& key) const;
	template<typename T> T value(const std::string& key, T minValue, T maxValue) const;

	// If an entry with the same key exists, it will be overwritten.
	template<typename T> void put(const std::string& key, T value);
	void put(const std::string& key, const char* value);
	ConfigurationData& insert(const ConfigurationData& other);
private:
	typedef std::unordered_map<std::string, std::string> Map;

	template<typename T> static T convertString(const std::string& s);
	template<typename T> static void convertValue(const T& value, std::string& s);

	std::string filePath_;
	Map valueMap_;
};



template<typename T>
T
ConfigurationData::value(const std::string& key) const
{
	auto iter = valueMap_.find(key);
	if (iter == valueMap_.end()) {
		if (filePath_.empty()) {
			THROW_EXCEPTION(InvalidParameterException, "Key '" << key << "' not found.");
		} else {
			THROW_EXCEPTION(InvalidParameterException, "Key '" << key << "' not found in file " << filePath_ << '.');
		}
	}

	T value;
	try {
		value = convertString<T>(iter->second);
	} catch (const std::invalid_argument& e) {
		if (filePath_.empty()) {
			THROW_EXCEPTION(InvalidValueException, "No value conversion could be performed for key '" << key << "'.");
		} else {
			THROW_EXCEPTION(InvalidValueException, "No value conversion could be performed for key '" << key << "' in file " << filePath_ << ": " << e.what() << '.');
		}
	} catch (const std::out_of_range& e) {
		if (filePath_.empty()) {
			THROW_EXCEPTION(InvalidValueException, "The converted value would fall out of the range for key '" << key << "'.");
		} else {
			THROW_EXCEPTION(InvalidValueException, "The converted value would fall out of the range for key '" << key << "' in file " << filePath_ << ": " << e.what() << '.');
		}
	}
	return value;
}

template<typename T>
T
ConfigurationData::value(const std::string& key, T minValue, T maxValue) const
{
	T v = value<T>(key);

	if (v > maxValue) {
		THROW_EXCEPTION(InvalidValueException, "The value for key '" << key << "' must be <= " << maxValue << " in file " << filePath_ << '.');
	} else if (v < minValue) {
		THROW_EXCEPTION(InvalidValueException, "The value for key '" << key << "' must be >= " << minValue << " in file " << filePath_ << '.');
	}

	return v;
}

template<typename T>
void
ConfigurationData::put(const std::string& key, T value)
{
	std::string& valueString = valueMap_[key];
	convertValue(value, valueString);
}

template<typename T>
void
ConfigurationData::convertValue(const T& value, std::string& s)
{
	std::ostringstream out;
	out << value;
	s = out.str();
}

inline
ConfigurationData&
ConfigurationData::insert(const ConfigurationData& other)
{
	for (auto& item : other.valueMap_) {
		put(item.first, item.second);
	}
	return *this;
}

} /* namespace GS */

#endif /* CONFIGURATION_DATA_H_ */
