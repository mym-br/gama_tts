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

#define CONFIG_FILE_NAME "/vtm_control_model.txt"
#define VOICE_FILE_PREFIX "/voice_"

namespace GS {
namespace VTMControlModel {

Configuration::Configuration(const char* configDirPath)
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
	initialPitch       = config.value<double>("initial_pitch");
	pitchOffset        = config.value<double>("pitch_offset");
	driftDeviation     = config.value<double>("drift_deviation");
	driftLowpassCutoff = config.value<double>("drift_lowpass_cutoff");
	voiceName          = config.value<std::string>("voice_name");

	notionalPitch             = config.value<double>("notional_pitch");
	pretonicPitchRange        = config.value<double>("pretonic_pitch_range");
	pretonicPerturbationRange = config.value<double>("pretonic_perturbation_range");
	tonicPitchRange           = config.value<double>("tonic_pitch_range");
	tonicPerturbationRange    = config.value<double>("tonic_perturbation_range");

	microIntonation  = (config.value<int>("micro_intonation" ) != 0);
	macroIntonation  = (config.value<int>("macro_intonation" ) != 0);
	smoothIntonation = (config.value<int>("smooth_intonation") != 0);
	intonationDrift  = (config.value<int>("intonation_drift" ) != 0);
	randomIntonation = (config.value<int>("random_intonation") != 0);

	// Load voice data.
	std::ostringstream voiceFilePath;
	voiceFilePath << configDirPath << VOICE_FILE_PREFIX << voiceName << ".txt";
	voiceData = std::make_unique<ConfigurationData>(voiceFilePath.str());
	intonationFactor = voiceData->value<double>("intonation_factor");

	std::string pho = config.value<std::string>("phonetic_string_format");
	if (pho == "gnuspeech") {
		phoStrFormat = PhoneticStringFormat::gnuspeech;
	} else if (pho == "mbrola") {
		phoStrFormat = PhoneticStringFormat::mbrola;
	} else {
		THROW_EXCEPTION(InvalidValueException, "Invalid phonetic string format: " << pho << '.');
	}
}

Configuration::~Configuration()
{
}

} /* namespace VTMControlModel */
} /* namespace GS */
