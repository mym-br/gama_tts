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

#include "Controller.h"

#include <cctype> /* isspace */
#include <cstring>
#include <sstream>

#define VTM_CONTROL_MODEL_CONFIG_FILE_NAME "/vtm_control_model.config"
#define VTM_CONFIG_FILE_NAME "/vtm.config"
#define VOICE_FILE_PREFIX "/voice_"



namespace GS {
namespace VTMControlModel {

Controller::Controller(const char* configDirPath, Model& model)
		: model_(model)
		, eventList_(configDirPath, model_)
{
	// Load VTMControlModel::Configuration.

	std::ostringstream vtmControlModelConfigFilePath;
	vtmControlModelConfigFilePath << configDirPath << VTM_CONTROL_MODEL_CONFIG_FILE_NAME;
	vtmControlModelConfig_.load(vtmControlModelConfigFilePath.str());

	// Load VTM::Configuration.

	std::ostringstream vtmConfigFilePath;
	vtmConfigFilePath << configDirPath << VTM_CONFIG_FILE_NAME;

	std::ostringstream voiceFilePath;
	voiceFilePath << configDirPath << VOICE_FILE_PREFIX << vtmControlModelConfig_.voiceName << ".config";

	vtmConfigData_ = std::make_unique<ConfigurationData>(voiceFilePath.str());
	vtmConfigData_->insert(ConfigurationData(vtmConfigFilePath.str()));
}

Controller::~Controller()
{
}

void
Controller::synthesizeFromEventList(const char* vtmParamFile, const char* outputFile)
{
	std::fstream vtmParamStream(vtmParamFile, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (!vtmParamStream) {
		THROW_EXCEPTION(IOException, "Could not open the file " << vtmParamFile << '.');
	}

	initUtterance();

	eventList_.generateOutput(vtmParamStream);

	vtmParamStream.seekg(0);

	auto vtm = VTM::VocalTractModel::getInstance(*vtmConfigData_);
	vtm->synthesizeToFile(vtmParamStream, outputFile);
}

void
Controller::synthesizeFromEventList(const char* vtmParamFile, std::vector<float>& buffer)
{
	std::fstream vtmParamStream(vtmParamFile, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (!vtmParamStream) {
		THROW_EXCEPTION(IOException, "Could not open the file " << vtmParamFile << '.');
	}

	initUtterance();

	eventList_.generateOutput(vtmParamStream);

	vtmParamStream.seekg(0);

	auto vtm = VTM::VocalTractModel::getInstance(*vtmConfigData_);
	vtm->synthesizeToBuffer(vtmParamStream, buffer);
}

void
Controller::initUtterance()
{
	int outputRate = vtmConfigData_->value<int>("output_rate");
	const float vtlOffset = vtmConfigData_->value<float>("vocal_tract_length_offset");
	const float vocalTractLength = vtmConfigData_->value<float>("vocal_tract_length");

	if ((outputRate != 22050) && (outputRate != 44100)) {
		vtmConfigData_->put("output_rate", 44100);
		outputRate = 44100;
	}
	if ((vtlOffset + vocalTractLength) < 15.9f) {
		vtmConfigData_->put("output_rate", 44100);
		outputRate = 44100;
	}

	if (Log::debugEnabled) {
		printf("Tube Length = %f\n", vtlOffset + vocalTractLength);
		printf("Voice: %s\n", vtmControlModelConfig_.voiceName.c_str());
		printf("sampling Rate: %d\n", outputRate);
	}

	eventList_.setPitchMean(vtmControlModelConfig_.pitchOffset + vtmConfigData_->value<float>("reference_glottal_pitch"));
	eventList_.setGlobalTempo(vtmControlModelConfig_.tempo);
	eventList_.setUpDriftGenerator(vtmControlModelConfig_.driftDeviation, vtmControlModelConfig_.controlRate, vtmControlModelConfig_.driftLowpassCutoff);

	// Configure intonation.
	eventList_.setMicroIntonation( vtmControlModelConfig_.intonation & Configuration::INTONATION_MICRO);
	eventList_.setMacroIntonation( vtmControlModelConfig_.intonation & Configuration::INTONATION_MACRO);
	eventList_.setSmoothIntonation(vtmControlModelConfig_.intonation & Configuration::INTONATION_SMOOTH);
	eventList_.setIntonationDrift( vtmControlModelConfig_.intonation & Configuration::INTONATION_DRIFT);
	eventList_.setTgUseRandom(     vtmControlModelConfig_.intonation & Configuration::INTONATION_RANDOMIZE);
}

bool
Controller::nextChunk(const std::string& phoneticString, std::size_t& index, std::size_t& size)
{
	static const std::string token {"/c"};

	const std::size_t startPos = phoneticString.find(token, index);
	if (startPos == std::string::npos) {
		index = phoneticString.size();
		size = 0;
		return false;
	}
	index = startPos;

	const std::size_t endPos = phoneticString.find(token, startPos + token.size());
	if (endPos == std::string::npos) {
		size = phoneticString.size() - startPos;
	} else {
		size = endPos                - startPos;
	}

	for (std::size_t i = index + token.size(), end = index + size; i < end; ++i) {
		if (!std::isspace(phoneticString[i])) {
			LOG_DEBUG("[Controller::nextChunk] Phonetic string chunk: \"" <<
					std::string(phoneticString.begin() + index, phoneticString.begin() + index + size) << '"');
			return true;
		}
	}

	LOG_DEBUG("[Controller::nextChunk] Empty chunk.");

	// The chunk contains only spaces or is empty.
	return false;
}

} /* namespace VTMControlModel */
} /* namespace GS */
