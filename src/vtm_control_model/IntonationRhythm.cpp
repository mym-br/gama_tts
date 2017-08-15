/***************************************************************************
 *  Copyright 2017 Marcelo Y. Matuda                                       *
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

#include "IntonationRhythm.h"

#include <fstream>
#include <sstream>

#include "ConfigurationData.h"
#include "Exception.h"

#define CONFIG_SUB_DIR "/intonation_rhythm/"
#define INTONATION_CONFIG_FILE "intonation.config"
#define RHYTHM_CONFIG_FILE "rhythm.config"
#define TONE_GROUP_PARAM_FILE_STATEMENT    "tone_group_param-statement.txt"
#define TONE_GROUP_PARAM_FILE_EXCLAMATION  "tone_group_param-exclamation.txt"
#define TONE_GROUP_PARAM_FILE_QUESTION     "tone_group_param-question.txt"
#define TONE_GROUP_PARAM_FILE_CONTINUATION "tone_group_param-continuation.txt"
#define TONE_GROUP_PARAM_FILE_SEMICOLON    "tone_group_param-semicolon.txt"

#define COMMENT_CHAR '#'
#define SEPARATORS " \t"



namespace GS {
namespace VTMControlModel {

IntonationRhythm::IntonationRhythm(const char* configDirPath)
		: toneGroupParameters_(static_cast<int>(ToneGroup::numberOfGroups))
		, fixedIntonationParameters_(NUM_INTONATION_PARAM, 0.0)
		, randSrc_{randDev_()}
		, randomIntonationParamSetIndex_(static_cast<int>(ToneGroup::numberOfGroups))
		, useFixedIntonationParameters_{}
		, useRandomIntonation_{}
{
	std::string configDir{configDirPath};
	configDir += CONFIG_SUB_DIR;

	loadToneGroupParameters(ToneGroup::statement   , configDir + TONE_GROUP_PARAM_FILE_STATEMENT);
	loadToneGroupParameters(ToneGroup::exclamation , configDir + TONE_GROUP_PARAM_FILE_EXCLAMATION);
	loadToneGroupParameters(ToneGroup::question    , configDir + TONE_GROUP_PARAM_FILE_QUESTION);
	loadToneGroupParameters(ToneGroup::continuation, configDir + TONE_GROUP_PARAM_FILE_CONTINUATION);
	loadToneGroupParameters(ToneGroup::semicolon   , configDir + TONE_GROUP_PARAM_FILE_SEMICOLON);

	ConfigurationData intonationConfigData{configDir + INTONATION_CONFIG_FILE};
	intonationTimeOffset_       = intonationConfigData.value<float>("time_offset");
	pretonicBaseSlope_          = intonationConfigData.value<float>("pretonic_base_slope");
	pretonicBaseSlopeRandom_    = intonationConfigData.value<float>("pretonic_base_slope_random");
	pretonicSlopeRandomFactor_  = intonationConfigData.value<float>("pretonic_slope_random_factor");
	tonicBaseSlope_             = intonationConfigData.value<float>("tonic_base_slope");
	tonicContinuationBaseSlope_ = intonationConfigData.value<float>("tonic_continuation_base_slope");
	tonicSlopeRandomFactor_     = intonationConfigData.value<float>("tonic_slope_random_factor");
	tonicSlopeOffset_           = intonationConfigData.value<float>("tonic_slope_offset");

	ConfigurationData rhythmConfigData{configDir + RHYTHM_CONFIG_FILE};
	rhythmMarkedA_     = rhythmConfigData.value<double>("marked_a");
	rhythmMarkedB_     = rhythmConfigData.value<double>("marked_b");
	rhythmMarkedDiv_   = rhythmConfigData.value<double>("marked_div");
	rhythmUnmarkedA_   = rhythmConfigData.value<double>("unmarked_a");
	rhythmUnmarkedB_   = rhythmConfigData.value<double>("unmarked_b");
	rhythmUnmarkedDiv_ = rhythmConfigData.value<double>("unmarked_div");
	rhythmMinTempo_    = rhythmConfigData.value<double>("min_tempo");
	rhythmMaxTempo_    = rhythmConfigData.value<double>("max_tempo");
}

void
IntonationRhythm::loadToneGroupParameters(ToneGroup toneGroup, const std::string& filePath)
{
	std::vector<std::vector<float>>& data = toneGroupParameters_[static_cast<int>(toneGroup)];

	std::ifstream fileStream{filePath, std::ios_base::binary};
	if (!fileStream) {
		THROW_EXCEPTION(IOException, "Could not open the file " << filePath << '.');
	}

	std::string line;
	while (std::getline(fileStream, line)) {
		std::size_t pos = line.find_first_not_of(SEPARATORS);
		if (pos == std::string::npos) {
			continue; // empty line
		}
		if (line[pos] == COMMENT_CHAR) {
			continue; // comment
		}

		std::vector<float> paramSet(NUM_INTONATION_PARAM);

		std::istringstream lineStream{line};
		if (!(lineStream >> paramSet[INTON_PRM_NOTIONAL_PITCH])) {
			THROW_EXCEPTION(IOException, "Could not get the notional pitch.");
		}
		if (!(lineStream >> paramSet[INTON_PRM_PRETONIC_PITCH_RANGE])) {
			THROW_EXCEPTION(IOException, "Could not get the pretonic pitch range.");
		}
		if (!(lineStream >> paramSet[INTON_PRM_PRETONIC_PERTURBATION_RANGE])) {
			THROW_EXCEPTION(IOException, "Could not get the pretonic perturbation range.");
		}
		if (!(lineStream >> paramSet[INTON_PRM_TONIC_PITCH_RANGE])) {
			THROW_EXCEPTION(IOException, "Could not get the tonic pitch range.");
		}
		if (!(lineStream >> paramSet[INTON_PRM_TONIC_PERTURBATION_RANGE])) {
			THROW_EXCEPTION(IOException, "Could not get the tonic perturbation range.");
		}

		data.push_back(std::move(paramSet));
	}

	if (data.empty()) {
		THROW_EXCEPTION(IOException, "No data found for tone group " << static_cast<int>(toneGroup) << '.');
	}
}

void
IntonationRhythm::setUseRandomIntonation(bool value)
{
	if (value) {
		const unsigned int numToneGroups = static_cast<unsigned int>(ToneGroup::numberOfGroups);
		for (unsigned int i = 0; i < numToneGroups; ++i) {
			const unsigned int numParamSets = toneGroupParameters_[i].size();
			if (numParamSets > 1) {
				randomIntonationParamSetIndex_[i] = std::make_unique<std::uniform_int_distribution<>>(0, numParamSets - 1);
			}
		}
	} else {
		for (auto& item : randomIntonationParamSetIndex_) {
			item.reset();
		}
	}
	useRandomIntonation_ = value;
}

const float*
IntonationRhythm::intonationParameters(ToneGroup toneGroup)
{
	if (useFixedIntonationParameters_) {
		return fixedIntonationParameters_.data();
	}

	unsigned int toneGroupIndex = static_cast<unsigned int>(toneGroup);
	if (toneGroupIndex >= toneGroupParameters_.size()) {
		toneGroupIndex = 0;
	}
	if (toneGroupIndex < randomIntonationParamSetIndex_.size() && randomIntonationParamSetIndex_[toneGroupIndex]) {
		const int paramSetIndex = (*randomIntonationParamSetIndex_[toneGroupIndex])(randSrc_);
		return toneGroupParameters_[toneGroupIndex][paramSetIndex].data();
	} else {
		return toneGroupParameters_[toneGroupIndex][0].data();
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */
