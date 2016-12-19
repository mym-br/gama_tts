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

#include <cstdio>
#include <fstream>
#include <istream>
#include <memory>
#include <vector>

#include "ConfigurationData.h"
#include "EventList.h"
#include "Log.h"
#include "Model.h"
#include "VTMControlModelConfiguration.h"
#include "VocalTractModel0.h"



namespace GS {
namespace VTMControlModel {

class Controller {
public:
	Controller(const char* configDirPath, Model& model);
	~Controller();

	template<typename T> void synthesizePhoneticString(T& phoneticStringParser, const char* phoneticString, const char* vtmParamFile, const char* outputFile);
	template<typename T> void synthesizePhoneticString(T& phoneticStringParser, const char* phoneticString, const char* vtmParamFile, std::vector<float>& buffer);
	template<typename T> void synthesizePhoneticString(T& phoneticStringParser, const char* phoneticString, std::iostream& vtmParamStream);
	void synthesizeFromEventList(const char* vtmParamFile, const char* outputFile);
	void synthesizeFromEventList(const char* vtmParamFile, std::vector<float>& buffer);

	Model& model() { return model_; }
	EventList& eventList() { return eventList_; }
	Configuration& vtmControlModelConfiguration() { return vtmControlModelConfig_; }
	const ConfigurationData& vtmConfigurationData() const { return *vtmConfigData_; }
private:
	enum {
		MAX_VOICES = 5
	};

	Controller(const Controller&) = delete;
	Controller& operator=(const Controller&) = delete;

	void loadConfiguration(const char* configDirPath);
	void initUtterance();
	int calcChunks(const char* string);
	int nextChunk(const char* string);
	void printVowelTransitions();

	int validPosture(const char* token);
	void setIntonation(int intonation);

	template<typename T> void synthesizePhoneticStringChunk(T& phoneticStringParser, const char* phoneticStringChunk, std::ostream& vtmParamStream);

	Model& model_;
	EventList eventList_;
	Configuration vtmControlModelConfig_;
	std::unique_ptr<ConfigurationData> vtmConfigData_;
};



template<typename T>
void
Controller::synthesizePhoneticString(T& phoneticStringParser, const char* phoneticString, const char* vtmParamFile, const char* outputFile)
{
	std::fstream vtmParamStream(vtmParamFile, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (!vtmParamStream) {
		THROW_EXCEPTION(IOException, "Could not open the file " << vtmParamFile << '.');
	}

	synthesizePhoneticString(phoneticStringParser, phoneticString, vtmParamStream);

	VTM::VocalTractModel0<double> vtm(*vtmConfigData_);
	vtm.synthesizeToFile(vtmParamStream, outputFile);
}

template<typename T>
void
Controller::synthesizePhoneticString(T& phoneticStringParser, const char* phoneticString, const char* vtmParamFile, std::vector<float>& buffer)
{
	std::fstream vtmParamStream(vtmParamFile, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (!vtmParamStream) {
		THROW_EXCEPTION(IOException, "Could not open the file " << vtmParamFile << '.');
	}

	synthesizePhoneticString(phoneticStringParser, phoneticString, vtmParamStream);

	VTM::VocalTractModel0<double> vtm(*vtmConfigData_);
	vtm.synthesizeToBuffer(vtmParamStream, buffer);
}

template<typename T>
void
Controller::synthesizePhoneticString(T& phoneticStringParser, const char* phoneticString, std::iostream& vtmParamStream)
{
	int chunks = calcChunks(phoneticString);

	initUtterance();

	int index = 0;
	while (chunks > 0) {
		if (Log::debugEnabled) {
			printf("Speaking \"%s\"\n", &phoneticString[index]);
		}

		synthesizePhoneticStringChunk(phoneticStringParser, &phoneticString[index], vtmParamStream);

		index += nextChunk(&phoneticString[index + 2]) + 2;
		chunks--;
	}

	vtmParamStream.seekg(0);
}

template<typename T>
void
Controller::synthesizePhoneticStringChunk(T& phoneticStringParser, const char* phoneticStringChunk, std::ostream& vtmParamStream)
{
	eventList_.setUp();

	phoneticStringParser.parseString(phoneticStringChunk);

	eventList_.generateEventList();

	eventList_.applyIntonation();
	eventList_.applyIntonationSmooth();

	eventList_.generateOutput(vtmParamStream);
}

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_CONTROLLER_H_ */
