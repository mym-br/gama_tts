/*
 * Copyright 2014 Marcelo Y. Matuda
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
	explicit ConfigurationData(const std::string& filePath);
	~ConfigurationData() = default;

	template<typename T> T value(const std::string& key) const;
	template<typename T> T value(const std::string& key, T minValue, T maxValue) const;

	// If an entry with the same key exists, it will be overwritten.
	template<typename T> void put(const std::string& key, T value);
	void put(const std::string& key, const char* value);
	ConfigurationData& insert(const ConfigurationData& other);

	const std::string& dirPath() const { return dirPath_; }
private:
	typedef std::unordered_map<std::string, std::string> Map;

	template<typename T> static T convertString(const std::string& s);
	template<typename T> static void convertValue(const T& value, std::string& s);

	std::string filePath_;
	std::string dirPath_;
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

} /* namespace GS */

#endif /* CONFIGURATION_DATA_H_ */
