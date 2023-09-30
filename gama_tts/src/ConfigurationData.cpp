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

#include "ConfigurationData.h"

#include <cctype> /* isspace */
#include <filesystem>
#include <fstream>
#include <limits>

namespace fs = std::filesystem;



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

	fs::path fsPath{filePath};
	dirPath_ = fsPath.has_parent_path() ? fsPath.parent_path().string() : "";

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
