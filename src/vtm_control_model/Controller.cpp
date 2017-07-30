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
#include <cmath> /* rint */
#include <cstdio>
#include <fstream>
#include <sstream>

#include "Exception.h"
#include "Log.h"
#include "VTMUtil.h"
#include "WAVEFileWriter.h"

#define VTM_CONFIG_FILE_NAME "/vtm.config"



namespace GS {
namespace VTMControlModel {

Controller::Controller(const char* configDirPath, Model& model)
		: model_{model}
		, eventList_{configDirPath, model_}
		, phoneticStringParser_{configDirPath, model_, eventList_}
{
	// Load VTMControlModel configuration.
	vtmControlModelConfig_.load(configDirPath);

	// Load VTM configuration.
	std::ostringstream vtmConfigFilePath;
	vtmConfigFilePath << configDirPath << VTM_CONFIG_FILE_NAME;
	vtmConfigData_ = std::make_unique<ConfigurationData>(vtmConfigFilePath.str());
	vtmConfigData_->insert(*vtmControlModelConfig_.voiceData);

	// Get the vocal tract model instance.
	vtm_ = VTM::VocalTractModel::getInstance(*vtmConfigData_);
}

Controller::~Controller()
{
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
	eventList_.setIntonationFactor(vtmControlModelConfig_.intonationFactor);
}

bool
Controller::nextChunk(const std::string& phoneticString, std::size_t& index, std::size_t& size)
{
	static const std::string token{"/c"};

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

void
Controller::fillParameterStream(const std::string& phoneticString, std::iostream& vtmParamStream)
{
	initUtterance();

	std::size_t index = 0, size = 0;
	while (index < phoneticString.size()) {
		if (nextChunk(phoneticString, index, size)) {
			eventList_.setUp();

			phoneticStringParser_.parse(&phoneticString[index], size);

			eventList_.generateEventList();
			eventList_.applyIntonation();
			eventList_.generateOutput(vtmParamStream);
		}

		index += size;
	}

	vtmParamStream.seekg(0);
}

void
Controller::synthesizePhoneticString(const std::string& phoneticString, const char* vtmParamFile, const char* outputFile)
{
	std::fstream vtmParamStream(vtmParamFile, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (!vtmParamStream) {
		THROW_EXCEPTION(IOException, "Could not open the file " << vtmParamFile << '.');
	}

	fillParameterStream(phoneticString, vtmParamStream);

	synthesizeToFile(vtmParamStream, outputFile);
}

void
Controller::synthesizePhoneticString(const std::string& phoneticString, const char* vtmParamFile, std::vector<float>& buffer)
{
	std::fstream vtmParamStream(vtmParamFile, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (!vtmParamStream) {
		THROW_EXCEPTION(IOException, "Could not open the file " << vtmParamFile << '.');
	}

	fillParameterStream(phoneticString, vtmParamStream);

	synthesizeToBuffer(vtmParamStream, buffer);
}

void
Controller::synthesizeFromEventList(const char* vtmParamFile, const char* outputFile)
{
	std::fstream vtmParamStream(vtmParamFile, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (!vtmParamStream) {
		THROW_EXCEPTION(IOException, "Could not open the file " << vtmParamFile << '.');
	}

	initUtterance();

	eventList_.clearMacroIntonation();
	eventList_.prepareMacroIntonationInterpolation();
	eventList_.generateOutput(vtmParamStream);

	vtmParamStream.seekg(0);

	synthesizeToFile(vtmParamStream, outputFile);
}

void
Controller::synthesizeFromEventList(const char* vtmParamFile, std::vector<float>& buffer)
{
	std::fstream vtmParamStream(vtmParamFile, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (!vtmParamStream) {
		THROW_EXCEPTION(IOException, "Could not open the file " << vtmParamFile << '.');
	}

	initUtterance();

	eventList_.clearMacroIntonation();
	eventList_.prepareMacroIntonationInterpolation();
	eventList_.generateOutput(vtmParamStream);

	vtmParamStream.seekg(0);

	synthesizeToBuffer(vtmParamStream, buffer);
}

void
Controller::synthesizeToFile(std::istream& inputStream, const char* outputFile)
{
	if (!vtm_->outputBuffer().empty()) {
		vtm_->reset();
	}
	std::vector<std::vector<float>> parameterList;
	parseInputParameterStream(inputStream, parameterList);
	synthesize(parameterList);
	vtm_->finishSynthesis();
	writeOutputToFile(vtm_->outputBuffer(), outputFile, vtm_->outputSampleRate());
}

void
Controller::synthesizeToBuffer(std::istream& inputStream, std::vector<float>& outputBuffer)
{
	if (!vtm_->outputBuffer().empty()) {
		vtm_->reset();
	}
	std::vector<std::vector<float>> parameterList;
	parseInputParameterStream(inputStream, parameterList);
	synthesize(parameterList);
	vtm_->finishSynthesis();
	writeOutputToBuffer(vtm_->outputBuffer(), outputBuffer);
}

void
Controller::parseInputParameterStream(std::istream& in, std::vector<std::vector<float>>& paramList)
{
	std::string line;
	const std::size_t numParam = model_.parameterList().size();
	std::vector<float> parameters(numParam);

	unsigned int lineNumber = 1;
	while (std::getline(in, line)) {
		std::istringstream lineStream(line);

		for (std::size_t i = 0; i < numParam; ++i) {
			lineStream >> parameters[i];
		}
		if (!lineStream) {
			THROW_EXCEPTION(VTMException, "Could not read vocal tract parameters from stream (line number " << lineNumber << ").");
		}

		paramList.push_back(parameters);
		++lineNumber;
	}

	// Duplicate the last set of parameters, to help the interpolation.
	if (!paramList.empty()) {
		paramList.push_back(paramList.back());
	}
}

void
Controller::synthesize(const std::vector<std::vector<float>>& paramList)
{
	// Number of internal sample rate periods in each control rate period.
	const unsigned int controlSteps = static_cast<unsigned int>(std::rint(vtm_->internalSampleRate() / vtmControlModelConfig_.controlRate));
	const float coef = 1.0f / controlSteps;

	const std::size_t numParam = model_.parameterList().size();
	std::vector<float> currentParameter(numParam);
	std::vector<float> currentParameterDelta(numParam);

	// For each control period:
	for (std::size_t i = 1, size = paramList.size(); i < size; ++i) {
		// Calculates the current parameter values, and their
		// associated sample-to-sample delta values.
		for (std::size_t j = 0; j < numParam; ++j) {
			currentParameter[j] = paramList[i - 1][j];
			currentParameterDelta[j] = (paramList[i][j] - currentParameter[j]) * coef;
		}

		// For each step in a control period:
		for (std::size_t j = 0; j < controlSteps; ++j) {
			vtm_->setAllParameters(currentParameter);
			vtm_->execSynthesisStep();

			// Do linear interpolation.
			for (std::size_t k = 0; k < numParam; ++k) {
				currentParameter[k] += currentParameterDelta[k];
			}
		}
	}
}

void
Controller::writeOutputToFile(const std::vector<float>& data, const char* outputFile, float outputSampleRate)
{
	WAVEFileWriter fileWriter(outputFile, 1, data.size(), outputSampleRate);

	const float scale = VTM::Util::calculateOutputScale(data);
	for (std::size_t i = 0, end = data.size(); i < end; ++i) {
		fileWriter.writeSample(data[i] * scale);
	}
}

void
Controller::writeOutputToBuffer(const std::vector<float>& data, std::vector<float>& outputBuffer)
{
	outputBuffer.resize(data.size());

	const float scale = VTM::Util::calculateOutputScale(data);
	for (std::size_t i = 0, end = data.size(); i < end; ++i) {
		outputBuffer[i] = data[i] * scale;
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */
