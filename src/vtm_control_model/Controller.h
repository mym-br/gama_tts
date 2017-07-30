/***************************************************************************
 *  Copyright 1991, 1992, 1993, 1994, 1995, 1996, 2001, 2002               *
 *    David R. Hill, Leonard Manzara, Craig Schock                         *
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
// 2014-09
// This file was copied from Gnuspeech and modified by Marcelo Y. Matuda.

#ifndef VTM_CONTROL_MODEL_CONTROLLER_H_
#define VTM_CONTROL_MODEL_CONTROLLER_H_

#include <cstddef> /* std::size_t */
#include <istream>
#include <memory>
#include <string>
#include <vector>

#include "ConfigurationData.h"
#include "EventList.h"
#include "Model.h"
#include "PhoneticStringParser.h"
#include "VocalTractModel.h"
#include "VTMControlModelConfiguration.h"



namespace GS {
namespace VTMControlModel {

class Controller {
public:
	Controller(const char* configDirPath, Model& model);
	~Controller();

	void fillParameterStream(const std::string& phoneticString, std::iostream& vtmParamStream);

	// Synthesizes speech from phonetic string. Sends to file.
	// The VTM parameters will be written to a file.
	void synthesizePhoneticString(const std::string& phoneticString, const char* vtmParamFile, const char* outputFile);
	// Synthesizes speech from phonetic string. Sends to buffer.
	// The VTM parameters will be written to a file.
	void synthesizePhoneticString(const std::string& phoneticString, const char* vtmParamFile, std::vector<float>& buffer);

	// Synthesizes speech from data contained in the event list. Sends to file.
	void synthesizeFromEventList(const char* vtmParamFile, const char* outputFile);
	// Synthesizes speech from data contained in the event list. Sends to buffer.
	void synthesizeFromEventList(const char* vtmParamFile, std::vector<float>& buffer);

	EventList& eventList() { return eventList_; }
	Configuration& vtmControlModelConfiguration() { return vtmControlModelConfig_; }
	const ConfigurationData& vtmConfigurationData() const { return *vtmConfigData_; }

	void synthesizeToFile(std::istream& inputStream, const char* outputFile);
	void synthesizeToBuffer(std::istream& inputStream, std::vector<float>& outputBuffer);
private:
	enum {
		MAX_VOICES = 5
	};

	Controller(const Controller&) = delete;
	Controller& operator=(const Controller&) = delete;

	void initUtterance();

	// Chunks start with /c.
	// The text parser generates one /c at the start and another at the end of the string.
	// Text before the first /c will be ignored.
	// A /c at the end will generate an empty chunk.
	// Returns true if a valid chunk has been found.
	bool nextChunk(const std::string& phoneticString, std::size_t& index, std::size_t& size);

	void parseInputParameterStream(std::istream& in, std::vector<std::vector<float>>& paramList);
	void synthesize(const std::vector<std::vector<float>>& paramList);
	void writeOutputToFile(const std::vector<float>& data, const char* outputFile, float outputSampleRate);
	void writeOutputToBuffer(const std::vector<float>& data, std::vector<float>& outputBuffer);

	Model& model_;
	EventList eventList_;
	PhoneticStringParser phoneticStringParser_;
	Configuration vtmControlModelConfig_;
	std::unique_ptr<ConfigurationData> vtmConfigData_;
	std::unique_ptr<VTM::VocalTractModel> vtm_;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_CONTROLLER_H_ */
