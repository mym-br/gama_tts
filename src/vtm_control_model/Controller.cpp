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
#include <cstdio> /* printf */
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
		: configDirPath_{configDirPath}
		, model_{model}
		, eventList_{configDirPath, model_}
		, vtmControlModelConfig_{configDirPath}
{
	// Load VTM configuration.
	std::ostringstream vtmConfigFilePath;
	vtmConfigFilePath << configDirPath << VTM_CONFIG_FILE_NAME;
	vtmConfigData_ = std::make_unique<ConfigurationData>(vtmConfigFilePath.str());
	vtmConfigData_->insert(*vtmControlModelConfig_.voiceData);

	// Get the vocal tract model instance.
	vtm_ = VTM::VocalTractModel::getInstance(*vtmConfigData_);

	eventList_.setControlPeriod(vtmControlModelConfig_.controlPeriod);
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

	if (Log::debugEnabled) {
		printf("Tube Length = %f\n", vtlOffset + vocalTractLength);
		printf("Voice: %s\n", vtmControlModelConfig_.voiceName.c_str());
		printf("sampling Rate: %d\n", outputRate);
	}

	eventList_.setInitialPitch(vtmControlModelConfig_.initialPitch);
	eventList_.setMeanPitch(vtmControlModelConfig_.pitchOffset + vtmConfigData_->value<double>("reference_glottal_pitch"));
	eventList_.setGlobalTempo(vtmControlModelConfig_.tempo);
	eventList_.setUpDriftGenerator(vtmControlModelConfig_.driftDeviation, vtmControlModelConfig_.controlRate, vtmControlModelConfig_.driftLowpassCutoff);

	// Configure intonation.
	eventList_.setMicroIntonation( vtmControlModelConfig_.microIntonation);
	eventList_.setMacroIntonation( vtmControlModelConfig_.macroIntonation);
	eventList_.setSmoothIntonation(vtmControlModelConfig_.smoothIntonation);
	eventList_.setIntonationDrift( vtmControlModelConfig_.intonationDrift);
	eventList_.setRandomIntonation(vtmControlModelConfig_.randomIntonation);
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
Controller::getParametersFromPhoneticString(const std::string& phoneticString)
{
	if (!phoneticStringParser_) {
		phoneticStringParser_ = std::make_unique<PhoneticStringParser>(configDirPath_.c_str(), model_, eventList_);
	}

	vtmParamList_.clear();

	initUtterance();

	std::size_t index = 0, size = 0;
	while (index < phoneticString.size()) {
		if (nextChunk(phoneticString, index, size)) {
			eventList_.setUp();

			phoneticStringParser_->parse(&phoneticString[index], size);

			eventList_.generateEventList();
			eventList_.applyIntonation();
			eventList_.generateOutput(vtmParamList_);
		}

		index += size;
	}
}

void
Controller::getParametersFromEventList()
{
	vtmParamList_.clear();

	initUtterance();

	eventList_.clearMacroIntonation();
	eventList_.prepareMacroIntonationInterpolation();
	eventList_.generateOutput(vtmParamList_);
}

void
Controller::getParametersFromStream(std::istream& in)
{
	vtmParamList_.clear();
	std::string line;
	const std::size_t numParam = model_.parameterList().size();
	std::vector<float> param(numParam);

	unsigned int lineNumber = 1;
	while (std::getline(in, line)) {
		std::istringstream lineStream(line);

		for (std::size_t i = 0; i < numParam; ++i) {
			lineStream >> param[i];
		}
		if (!lineStream) {
			THROW_EXCEPTION(VTMException, "Could not read vocal tract parameters from stream (line number " << lineNumber << ").");
		}

		vtmParamList_.push_back(param);
		++lineNumber;
	}
}

void
Controller::synthesizePhoneticStringToFile(const std::string& phoneticString, const char* vtmParamFile, const char* outputFile)
{
	getParametersFromPhoneticString(phoneticString);
	if (vtmParamFile) writeVTMParameterFile(vtmParamList_, vtmParamFile);
	synthesizeToFile(outputFile);
}

void
Controller::synthesizePhoneticStringToBuffer(const std::string& phoneticString, const char* vtmParamFile, std::vector<float>& buffer)
{
	getParametersFromPhoneticString(phoneticString);
	if (vtmParamFile) writeVTMParameterFile(vtmParamList_, vtmParamFile);
	synthesizeToBuffer(buffer);
}

void
Controller::synthesizePho1ToFile(const std::string& phoneticString, const char* phonemeMapFile, const char* vtmParamFile, const char* outputFile)
{
	if (!pho1Parser_) {
		pho1Parser_ = std::make_unique<Pho1Parser>(configDirPath_.c_str(), model_, eventList_, phonemeMapFile);
	}

	vtmParamList_.clear();

	initUtterance();
	eventList_.setMicroIntonation(true);
	eventList_.setMacroIntonation(true);
	eventList_.setSmoothIntonation(false);

	eventList_.setUp();
	pho1Parser_->parse(phoneticString);
	eventList_.generateOutput(vtmParamList_);
	if (vtmParamFile) writeVTMParameterFile(vtmParamList_, vtmParamFile);
	synthesizeToFile(outputFile);
}

void
Controller::synthesizeFromEventListToFile(const char* vtmParamFile, const char* outputFile)
{
	getParametersFromEventList();
	if (vtmParamFile) writeVTMParameterFile(vtmParamList_, vtmParamFile);
	synthesizeToFile(outputFile);
}

void
Controller::synthesizeFromEventListToBuffer(const char* vtmParamFile, std::vector<float>& buffer)
{
	getParametersFromEventList();
	if (vtmParamFile) writeVTMParameterFile(vtmParamList_, vtmParamFile);
	synthesizeToBuffer(buffer);
}

void
Controller::synthesizeToFile(const char* outputFile)
{
	if (!outputFile) return;

	if (!vtm_->outputBuffer().empty()) vtm_->reset();
	synthesize(vtmParamList_);
	vtm_->finishSynthesis();
	writeOutputToFile(outputFile, outputScale_);
}

void
Controller::synthesizeToFile(std::vector<std::vector<float>>& vtmParamList, const char* vtmParamFile, const char* outputFile)
{
	if (vtmParamFile) writeVTMParameterFile(vtmParamList, vtmParamFile);

	if (!vtm_->outputBuffer().empty()) vtm_->reset();
	synthesize(vtmParamList);
	vtm_->finishSynthesis();
	float scale;
	writeOutputToFile(outputFile, scale);
}

void
Controller::synthesizeToBuffer(std::vector<float>& outputBuffer)
{
	if (!vtm_->outputBuffer().empty()) vtm_->reset();
	synthesize(vtmParamList_);
	vtm_->finishSynthesis();
	writeOutputToBuffer(outputBuffer, outputScale_);
}

void
Controller::synthesizeToBuffer(std::vector<std::vector<float>>& vtmParamList, const char* vtmParamFile, std::vector<float>& outputBuffer)
{
	if (vtmParamFile) writeVTMParameterFile(vtmParamList, vtmParamFile);

	if (!vtm_->outputBuffer().empty()) vtm_->reset();
	synthesize(vtmParamList);
	vtm_->finishSynthesis();
	float scale;
	writeOutputToBuffer(outputBuffer, scale);
}

void
Controller::synthesizeToFile(std::istream& inputStream, const char* outputFile)
{
	getParametersFromStream(inputStream);
	synthesizeToFile(outputFile);
}

void
Controller::synthesize(std::vector<std::vector<float>>& vtmParamList)
{
	if (vtmParamList.empty()) return;

	// Duplicate the last set of parameters, to help the interpolation.
	vtmParamList.push_back(vtmParamList.back());

	// Number of internal sample rate periods in each control rate period.
	const unsigned int controlSteps = static_cast<unsigned int>(std::rint(vtm_->internalSampleRate() / vtmControlModelConfig_.controlRate));
	const float coef = 1.0f / controlSteps;

	const std::size_t numParam = model_.parameterList().size();
	std::vector<float> currentParameter(numParam);
	std::vector<float> currentParameterDelta(numParam);

	// For each control period:
	for (std::size_t i = 1, size = vtmParamList.size(); i < size; ++i) {
		// Calculates the current parameter values, and their
		// associated sample-to-sample delta values.
		for (std::size_t j = 0; j < numParam; ++j) {
			currentParameter[j] = vtmParamList[i - 1][j];
			currentParameterDelta[j] = (vtmParamList[i][j] - currentParameter[j]) * coef;
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
Controller::writeOutputToFile(const char* outputFile, float& scale)
{
	if (!outputFile) {
		THROW_EXCEPTION(MissingValueException, "Missing output file name.");
	}
	const std::vector<float>& audioData = vtm_->outputBuffer();
	WAVEFileWriter fileWriter(outputFile, 1, audioData.size(), vtm_->outputSampleRate());

	scale = VTM::Util::calculateOutputScale(audioData);
	for (std::size_t i = 0, end = audioData.size(); i < end; ++i) {
		fileWriter.writeSample(audioData[i] * scale);
	}
}

void
Controller::writeOutputToBuffer(std::vector<float>& outputBuffer, float& scale)
{
	const std::vector<float>& audioData = vtm_->outputBuffer();
	outputBuffer.resize(audioData.size());

	scale = VTM::Util::calculateOutputScale(audioData);
	for (std::size_t i = 0, end = audioData.size(); i < end; ++i) {
		outputBuffer[i] = audioData[i] * scale;
	}
}

void
Controller::writeVTMParameterFile(const std::vector<std::vector<float>>& vtmParamList, const char* vtmParamFile)
{
	if (!vtmParamFile) {
		THROW_EXCEPTION(MissingValueException, "Missing output VTM parameter file name.");
	}
	std::ofstream out(vtmParamFile, std::ios_base::binary);
	if (!out) {
		THROW_EXCEPTION(IOException, "Could not open the file " << vtmParamFile << '.');
	}

	for (auto& param : vtmParamList) {
		if (param.empty()) {
			THROW_EXCEPTION(InvalidValueException, "Empty parameter set.");
		}
		out << param[0];
		for (unsigned int i = 1, size = param.size(); i < size; ++i) {
			out << ' ' << param[i];
		}
		out << '\n';
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */
