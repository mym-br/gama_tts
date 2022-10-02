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

#ifndef VTM_CONTROL_MODEL_CONFIGURATION_H_
#define VTM_CONTROL_MODEL_CONFIGURATION_H_

#include <memory>
#include <string>



namespace GS {

class ConfigurationData;
class Index;

namespace VTMControlModel {

enum class PhoneticStringFormat {
	gnuspeech,
	mbrola
};

struct Configuration {
	unsigned int controlPeriod; // 1, 2, 3 or 4 (ms)
	double controlRate;         // 1000.0 / controlPeriod (Hz)
	double tempo;
	double initialPitch;
	double pitchOffset;
	double driftDeviation;
	double driftLowpassCutoff;
	std::string voiceName;

	// Intonation parameters.
	double notionalPitch;
	double pretonicPitchRange;
	double pretonicPerturbationRange;
	double tonicPitchRange;
	double tonicPerturbationRange;
	double intonationFactor;
	bool microIntonation;
	bool macroIntonation;
	bool smoothIntonation;
	bool intonationDrift;
	bool randomIntonation;

	std::unique_ptr<ConfigurationData> voiceData;

	PhoneticStringFormat phoStrFormat;

	explicit Configuration(const Index& index);
	~Configuration();
	Configuration(const Configuration&) = delete;
	Configuration& operator=(const Configuration&) = delete;
	Configuration(Configuration&&) = delete;
	Configuration& operator=(Configuration&&) = delete;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_CONFIGURATION_H_ */
