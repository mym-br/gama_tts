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
// 2014-10
// This file was copied from Gnuspeech and modified by Marcelo Y. Matuda.

#include "VTMControlModelConfiguration.h"

#include <sstream>

#include "ConfigurationData.h"
#include "Exception.h"

#define CONFIG_FILE_NAME "/vtm_control_model.config"
#define VOICE_FILE_PREFIX "/voice_"

namespace GS {
namespace VTMControlModel {

Configuration::Configuration()
		: controlPeriod{}
		, controlRate{}
		, tempo{}
		, pitchOffset{}
		, driftDeviation{}
		, driftLowpassCutoff{}
		, intonation{}
		, textParserMode{}
		, notionalPitch{}
		, pretonicRange{}
		, pretonicLift{}
		, tonicRange{}
		, tonicMovement{}
		, intonationFactor{1.0}
{
}

void
Configuration::load(const char* configDirPath)
{
	std::ostringstream configFilePath;
	configFilePath << configDirPath << CONFIG_FILE_NAME;
	ConfigurationData config(configFilePath.str());

	controlPeriod      = config.value<unsigned int>("control_period");
	if (controlPeriod == 0 || controlPeriod > 4U) {
		THROW_EXCEPTION(InvalidValueException, "Invalid control period in file " << configFilePath.str() << '.');
	}
	controlRate        = 1000.0 / controlPeriod;
	tempo              = config.value<double>("tempo");
	pitchOffset        = config.value<double>("pitch_offset");
	driftDeviation     = config.value<double>("drift_deviation");
	driftLowpassCutoff = config.value<double>("drift_lowpass_cutoff");

	intonation = 0;
	if (config.value<int>("micro_intonation") != 0) {
		intonation += INTONATION_MICRO;
	}
	if (config.value<int>("macro_intonation") != 0) {
		intonation += INTONATION_MACRO;
	}
	if (config.value<int>("smooth_intonation") != 0) {
		intonation += INTONATION_SMOOTH;
	}
	if (config.value<int>("intonation_drift") != 0) {
		intonation += INTONATION_DRIFT;
	}
	if (config.value<int>("random_intonation") != 0) {
		intonation += INTONATION_RANDOMIZE;
	}

	textParserMode = config.value<int>("text_parser_mode");

	notionalPitch = config.value<double>("notional_pitch");
	pretonicRange = config.value<double>("pretonic_range");
	pretonicLift  = config.value<double>("pretonic_lift");
	tonicRange    = config.value<double>("tonic_range");
	tonicMovement = config.value<double>("tonic_movement");

	language        = config.value<std::string>("language");
	voiceName       = config.value<std::string>("voice_name");
	dictionary1File = config.value<std::string>("dictionary_1_file");
	dictionary2File = config.value<std::string>("dictionary_2_file");
	dictionary3File = config.value<std::string>("dictionary_3_file");

	// Load voice data.
	std::ostringstream voiceFilePath;
	voiceFilePath << configDirPath << VOICE_FILE_PREFIX << voiceName << ".config";
	voiceData = std::make_unique<ConfigurationData>(voiceFilePath.str());
	intonationFactor = voiceData->value<double>("intonation_factor");
}

} /* namespace VTMControlModel */
} /* namespace GS */
