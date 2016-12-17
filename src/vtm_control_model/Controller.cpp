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

#include <cstring>
#include <sstream>

#include "Exception.h"
#include "VocalTractModel0.h"

#define VTM_CONTROL_MODEL_CONFIG_FILE_NAME "/vtm_control_model.config"
#define VTM_CONFIG_FILE_NAME "/vtm.config"
#define VOICE_FILE_PREFIX "/voice_"



namespace GS {
namespace VTMControlModel {

Controller::Controller(const char* configDirPath, Model& model)
		: model_(model)
		, eventList_(configDirPath, model_)
{
	loadConfiguration(configDirPath);
}

Controller::~Controller()
{
}

void
Controller::loadConfiguration(const char* configDirPath)
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

/*******************************************************************************
 * This function synthesizes speech from data contained in the event list.
 */
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

	VTM::VocalTractModel0<double> vtm(*vtmConfigData_);
	vtm.synthesizeToFile(vtmParamStream, outputFile);
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

	VTM::VocalTractModel0<double> vtm(*vtmConfigData_);
	vtm.synthesizeToBuffer(vtmParamStream, buffer);
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

	const float referenceGlottalPitch = vtmConfigData_->value<float>("reference_glottal_pitch");

	eventList_.setPitchMean(vtmControlModelConfig_.pitchOffset + referenceGlottalPitch);
	eventList_.setGlobalTempo(vtmControlModelConfig_.tempo);
	setIntonation(vtmControlModelConfig_.intonation);
	eventList_.setUpDriftGenerator(vtmControlModelConfig_.driftDeviation, vtmControlModelConfig_.controlRate, vtmControlModelConfig_.driftLowpassCutoff);
}

// Chunks are separated by /c.
// There is always one /c at the begin and another at the end of the string.
int
Controller::calcChunks(const char* string)
{
	int temp = 0, index = 0;
	while (string[index] != '\0') {
		if ((string[index] == '/') && (string[index + 1] == 'c')) {
			temp++;
			index += 2;
		} else {
			index++;
		}
	}
	temp--;
	if (temp < 0) temp = 0;
	return temp;
}

// Returns the position of the next /c (the position of the /).
int
Controller::nextChunk(const char* string)
{
	int index = 0;
	while (string[index] != '\0') {
		if ((string[index] == '/') && (string[index + 1] == 'c')) {
			return index;
		} else {
			index++;
		}
	}
	return 0;
}

int
Controller::validPosture(const char* token)
{
	switch(token[0]) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return 1;
	default:
		return (model_.postureList().find(token) != nullptr);
	}
}

void
Controller::setIntonation(int intonation)
{
	if (intonation & Configuration::INTONATION_MICRO) {
		eventList_.setMicroIntonation(1);
	} else {
		eventList_.setMicroIntonation(0);
	}

	if (intonation & Configuration::INTONATION_MACRO) {
		eventList_.setMacroIntonation(1);
		eventList_.setSmoothIntonation(1); // Macro and not smooth is not working.
	} else {
		eventList_.setMacroIntonation(0);
		eventList_.setSmoothIntonation(0); // Macro and not smooth is not working.
	}

	// Macro and not smooth is not working.
//	if (intonation & Configuration::INTONATION_SMOOTH) {
//		eventList_.setSmoothIntonation(1);
//	} else {
//		eventList_.setSmoothIntonation(0);
//	}

	if (intonation & Configuration::INTONATION_DRIFT) {
		eventList_.setDrift(1);
	} else {
		eventList_.setDrift(0);
	}

	if (intonation & Configuration::INTONATION_RANDOMIZE) {
		eventList_.setTgUseRandom(true);
	} else {
		eventList_.setTgUseRandom(false);
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */