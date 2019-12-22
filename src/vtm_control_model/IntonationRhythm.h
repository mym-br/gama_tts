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
#ifndef VTM_CONTROL_MODEL_INTONATION_RHYTHM_H_
#define VTM_CONTROL_MODEL_INTONATION_RHYTHM_H_

#include <memory>
#include <random>
#include <string>
#include <vector>



namespace GS {
namespace VTMControlModel {

class IntonationRhythm {
public:
	enum class ToneGroup {
		statement,
		exclamation,
		question,
		continuation,
		semicolon,
		numberOfGroups,
		none
	};
	enum IntonationParam {
		INTON_PRM_NOTIONAL_PITCH,
		INTON_PRM_PRETONIC_PITCH_RANGE,
		INTON_PRM_PRETONIC_PERTURBATION_RANGE,
		INTON_PRM_TONIC_PITCH_RANGE,
		INTON_PRM_TONIC_PERTURBATION_RANGE,
		NUM_INTONATION_PARAM
	};

	explicit IntonationRhythm(const char* configDirPath);
	~IntonationRhythm() = default;

	void setUseFixedIntonationParameters(bool value) { useFixedIntonationParameters_ = value; }
	void setFixedIntonationParameter(IntonationParam param, float value) { // range not checked
		fixedIntonationParameters_[param] = value;
	}

	void setRandomIntonation(bool value);
	bool randomIntonation() const { return randomIntonation_; }

	const float* intonationParameters(ToneGroup toneGroup);

	double randomReal() { return randRealDist_(randSrc_); }

	float intonationTimeOffset()       const { return intonationTimeOffset_; }
	float pretonicBaseSlope()          const { return pretonicBaseSlope_; }
	float pretonicBaseSlopeRandom()    const { return pretonicBaseSlopeRandom_; }
	float pretonicSlopeRandomFactor()  const { return pretonicSlopeRandomFactor_; }
	float tonicBaseSlope()             const { return tonicBaseSlope_; }
	float tonicContinuationBaseSlope() const { return tonicContinuationBaseSlope_; }
	float tonicSlopeRandomFactor()     const { return tonicSlopeRandomFactor_; }
	float tonicSlopeOffset()           const { return tonicSlopeOffset_; }

	double rhythmMarkedA()     const { return rhythmMarkedA_; }
	double rhythmMarkedB()     const { return rhythmMarkedB_; }
	double rhythmMarkedDiv()   const { return rhythmMarkedDiv_; }
	double rhythmUnmarkedA()   const { return rhythmUnmarkedA_; }
	double rhythmUnmarkedB()   const { return rhythmUnmarkedB_; }
	double rhythmUnmarkedDiv() const { return rhythmUnmarkedDiv_; }
	double rhythmMinTempo()    const { return rhythmMinTempo_; }
	double rhythmMaxTempo()    const { return rhythmMaxTempo_; }

private:
	IntonationRhythm(const IntonationRhythm&) = delete;
	IntonationRhythm& operator=(const IntonationRhythm&) = delete;
	IntonationRhythm(IntonationRhythm&&) = delete;
	IntonationRhythm& operator=(IntonationRhythm&&) = delete;

	void loadToneGroupParameters(ToneGroup toneGroup, const std::string& filePath);

	std::vector<std::vector<std::vector<float>>> toneGroupParameters_;
	std::vector<float> fixedIntonationParameters_;
	std::random_device randDev_;
	std::mt19937 randSrc_;
	std::uniform_real_distribution<> randRealDist_;
	std::vector<std::unique_ptr<std::uniform_int_distribution<>>> randomIntonationParamSetIndex_;

	float intonationTimeOffset_;
	float pretonicBaseSlope_;
	float pretonicBaseSlopeRandom_;
	float pretonicSlopeRandomFactor_;
	float tonicBaseSlope_;
	float tonicContinuationBaseSlope_;
	float tonicSlopeRandomFactor_;
	float tonicSlopeOffset_;

	double rhythmMarkedA_;
	double rhythmMarkedB_;
	double rhythmMarkedDiv_;
	double rhythmUnmarkedA_;
	double rhythmUnmarkedB_;
	double rhythmUnmarkedDiv_;
	double rhythmMinTempo_;
	double rhythmMaxTempo_;

	bool useFixedIntonationParameters_;
	bool randomIntonation_;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_INTONATION_RHYTHM_H_ */
