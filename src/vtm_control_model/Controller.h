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
#include "Pho1Parser.h"
#include "PhoneticStringParser.h"
#include "VocalTractModel.h"
#include "VTMControlModelConfiguration.h"



namespace GS {
namespace VTMControlModel {

class Controller {
public:
	Controller(const char* configDirPath, Model& model);
	~Controller();

	Configuration& vtmControlModelConfiguration() { return vtmControlModelConfig_; }
	EventList& eventList() { return eventList_; }
	double outputSampleRate() const { return vtm_->outputSampleRate(); }

	// If vtmParamFile is not null, the VTM parameters will be written to a file.
	void synthesizePhoneticStringToFile(const std::string& phoneticString, const char* vtmParamFile, const char* outputFile);
	// If vtmParamFile is not null, the VTM parameters will be written to a file.
	void synthesizePhoneticStringToBuffer(const std::string& phoneticString, const char* vtmParamFile, std::vector<float>& buffer);

	// If vtmParamFile is not null, the VTM parameters will be written to a file.
	void synthesizePho1ToFile(const std::string& phoneticString, const char* phonemeMapFile, const char* vtmParamFile, const char* outputFile);

	// Synthesizes speech from data contained in the event list. Sends to a file.
	// If vtmParamFile is not null, the VTM parameters will be written to a file.
	void synthesizeFromEventListToFile(const char* vtmParamFile, const char* outputFile);
	// Synthesizes speech from data contained in the event list. Sends to a buffer.
	// If vtmParamFile is not null, the VTM parameters will be written to a file.
	void synthesizeFromEventListToBuffer(const char* vtmParamFile, std::vector<float>& buffer);

	// Synthesizes from VTM parameters contained in inputStream. Sends to a file.
	void synthesizeToFile(std::istream& inputStream, const char* outputFile);

private:
	Controller(const Controller&) = delete;
	Controller& operator=(const Controller&) = delete;

	void initUtterance();

	// Chunks start with /c.
	// The text parser generates one /c at the start and another at the end of the string.
	// Text before the first /c will be ignored.
	// A /c at the end will generate an empty chunk.
	// Returns true if a valid chunk has been found.
	bool nextChunk(const std::string& phoneticString, std::size_t& index, std::size_t& size);

	void getParametersFromPhoneticString(const std::string& phoneticString);
	void getParametersFromEventList();
	void getParametersFromStream(std::istream& in);
	void synthesize();
	void synthesizeToFile(const char* outputFile);
	void synthesizeToBuffer(std::vector<float>& outputBuffer);
	void writeOutputToFile(const char* outputFile);
	void writeOutputToBuffer(std::vector<float>& outputBuffer);
	void writeVTMParameterFile(const char* vtmParamFile);

	std::string configDirPath_;
	Model& model_;
	EventList eventList_;
	std::unique_ptr<PhoneticStringParser> phoneticStringParser_;
	std::unique_ptr<Pho1Parser> pho1Parser_;
	Configuration vtmControlModelConfig_;
	std::unique_ptr<ConfigurationData> vtmConfigData_;
	std::unique_ptr<VTM::VocalTractModel> vtm_;
	std::vector<std::vector<float>> vtmParamList_;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_CONTROLLER_H_ */
