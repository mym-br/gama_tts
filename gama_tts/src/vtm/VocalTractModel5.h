/***************************************************************************
 *  Copyright 1991, 1992, 1993, 1994, 1995, 1996, 2001, 2002               *
 *    David R. Hill, Leonard Manzara, Craig Schock                         *
 *  Copyright 2016, 2017 Marcelo Y. Matuda                                 *
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

//                                                                         NJ1         NJ2         NJ3         NJ4         NJ5         NJ6         NJ7
//                                                                         |           |           |           |           |           |           |
//                                                             VELUM=NR1   | NR2       | NR3       | NR4       | NR5       | NR6       | NR7       |
//                                                       nasal -------------------------------------------------------------------------------------
//                                                          ___|N1 |N2 |N3 |N4 |N5 |N6 |N7 |N8 |N9 |N10|N11|N12|N13|N14|N15|N16|N17|N18|N19|N20|N21| nose
//                                                         |   -------------------------------------------------------------------------------------
//         oropharynx                                      |
//         -------------------------------------------------------------------------------------------------------------------------
// vocal   |S1 |S2 |S3 |S4 |S5 |S6 |S7 |S8 |S9 |S10|S11|S12|S13|S14|S15|S16|S17|S18|S19|S20|S21|S22|S23|S24|S25|S26|S27|S28|S29|S30| mouth
// folds   -------------------------------------------------------------------------------------------------------------------------
//                     |       |               |           |           |           |           |               |       |           |
//           R1        | R2    | R3            | R4        | R4        | R5        | R5        | R6            | R7    | R8        |
//                     J1      J2              J3          |           J4          |           J5              J6      J7          J8
//                             FRIC 0.0                                                                                FRIC 7.0

#ifndef VTM_VOCAL_TRACT_MODEL_5_H_
#define VTM_VOCAL_TRACT_MODEL_5_H_

#include <algorithm> /* max */
#include <array>
#include <cmath> /* sqrt */
#include <cstddef> /* std::size_t */
#include <memory>
#include <vector>

#include "BandpassFilter.h"
#include "Butterworth1LowpassFilter.h"
#include "Butterworth2LowpassFilter.h"
#include "ConfigurationData.h"
#include "DifferenceFilter.h"
#include "Log.h"
#include "NoiseSource.h"
#include "ParameterLogger.h"
#include "PoleZeroRadiationImpedance.h"
#include "RosenbergBGlottalSource.h"
#include "SampleRateConverter.h"
#include "VocalTractModel.h"
#include "VTMUtil.h"

#define GS_VTM5_MIN_RADIUS (0.01)
#define GS_VTM5_MIN_FRIC_POS (0.0)
#define GS_VTM5_MAX_FRIC_POS (7.0)



namespace GS {
namespace VTM {

template<typename TFloat, unsigned int SectionDelay>
class VocalTractModel5 : public VocalTractModel {
public:
	explicit VocalTractModel5(const ConfigurationData& data, bool interactive=false);
	virtual ~VocalTractModel5() noexcept = default;

	virtual void reset() noexcept;

	virtual double internalSampleRate() const noexcept { return sampleRate_; }
	virtual double outputSampleRate() const noexcept { return config_.outputRate; }

	virtual void setParameter(int parameter, float value) noexcept;
	virtual void setAllParameters(const std::vector<float>& parameters) noexcept;

	virtual void execSynthesisStep() noexcept;
	virtual void finishSynthesis() noexcept;

	virtual std::vector<float>& outputBuffer() noexcept { return outputBuffer_; }

private:
	static constexpr TFloat MIN_VOCAL_TRACT_LENGTH = 3.0;
	static constexpr TFloat MAX_VOCAL_TRACT_LENGTH = 30.0;

	enum { /*  OROPHARYNX REGIONS  */
		R1 = 0, /*  S1  - S3   */
		R2 = 1, /*  S4  - S5   */
		R3 = 2, /*  S6  - S9   */
		R4 = 3, /*  S10 - S15  */
		R5 = 4, /*  S16 - S21  */
		R6 = 5, /*  S22 - S25  */
		R7 = 6, /*  S26 - S27  */
		R8 = 7, /*  S28 - S30  */
		TOTAL_REGIONS = 8
	};
	enum { /*  NASAL REGIONS  */
		NR1 = 0,
		NR2 = 1,
		NR3 = 2,
		NR4 = 3,
		NR5 = 4,
		NR6 = 5,
		NR7 = 6,
		TOTAL_NASAL_REGIONS = 7
	};
	enum { /*  NASAL TRACT SECTIONS  */
		N1  = 0,
		N2  = 1,
		N3  = 2,
		N4  = 3,
		N5  = 4,
		N6  = 5,
		N7  = 6,
		N8  = 7,
		N9  = 8,
		N10 = 9,
		N11 = 10,
		N12 = 11,
		N13 = 12,
		N14 = 13,
		N15 = 14,
		N16 = 15,
		N17 = 16,
		N18 = 17,
		N19 = 18,
		N20 = 19,
		N21 = 20,
		TOTAL_NASAL_SECTIONS = 21
	};
	enum Waveform {
		GLOTTAL_SOURCE_PULSE = 0,
		GLOTTAL_SOURCE_SINE  = 1
	};
	enum {
		VELUM = NR1
	};
	enum { /*  OROPHARYNX SCATTERING JUNCTION COEFFICIENTS (BETWEEN EACH REGION)  */
		J1 = R1, /*  R1-R2  */
		J2 = R2, /*  R2-R3  */
		J3 = R3, /*  R3-R4  */
		J4 = R4, /*  R4-R5  */
		J5 = R5, /*  R5-R6  */
		J6 = R6, /*  R6-R7  */
		J7 = R7, /*  R7-R8  */
		J8 = R8, /*  R8-AIR */
		TOTAL_JUNCTIONS = TOTAL_REGIONS
	};
	enum { /*  OROPHARYNX SECTIONS  */
		S1  = 0,  /*  R1  */
		S2  = 1,  /*  R1  */
		S3  = 2,  /*  R1  */

		S4  = 3,  /*  R2  */
		S5  = 4,  /*  R2  */

		S6  = 5,  /*  R3  */
		S7  = 6,  /*  R3  */
		S8  = 7,  /*  R3  */
		S9  = 8,  /*  R3  */

		S10 = 9,  /*  R4  */
		S11 = 10, /*  R4  */
		S12 = 11, /*  R4  */
		S13 = 12, /*  R4  */
		S14 = 13, /*  R4  */
		S15 = 14, /*  R4  */

		S16 = 15, /*  R5  */
		S17 = 16, /*  R5  */
		S18 = 17, /*  R5  */
		S19 = 18, /*  R5  */
		S20 = 19, /*  R5  */
		S21 = 20, /*  R5  */

		S22 = 21, /*  R6  */
		S23 = 22, /*  R6  */
		S24 = 23, /*  R6  */
		S25 = 24, /*  R6  */

		S26 = 25, /*  R7  */
		S27 = 26, /*  R7  */

		S28 = 27, /*  R8  */
		S29 = 28, /*  R8  */
		S30 = 29, /*  R8  */

		TOTAL_SECTIONS = 30
	};
	enum { /*  NASAL TRACT COEFFICIENTS  */
		NJ1 = 0, /*  N3  - N4   */
		NJ2 = 1, /*  N6  - N7   */
		NJ3 = 2, /*  N9  - N10  */
		NJ4 = 3, /*  N12 - N13  */
		NJ5 = 4, /*  N15 - N16  */
		NJ6 = 5, /*  N18 - N19  */
		NJ7 = 6, /*  N21 - AIR  */
		TOTAL_NASAL_JUNCTIONS = TOTAL_NASAL_SECTIONS / 3
	};
	enum ParameterIndex {
		PARAM_GLOT_PITCH = 0,
		PARAM_GLOT_VOL   = 1,
		PARAM_ASP_VOL    = 2,
		PARAM_FRIC_VOL   = 3,
		PARAM_FRIC_POS   = 4,
		PARAM_FRIC_CF    = 5,
		PARAM_FRIC_BW    = 6,
		PARAM_R1         = 7,
		PARAM_R2         = 8,
		PARAM_R3         = 9,
		PARAM_R4         = 10,
		PARAM_R5         = 11,
		PARAM_R6         = 12,
		PARAM_R7         = 13,
		PARAM_R8         = 14,
		PARAM_VELUM      = 15,
		TOTAL_PARAMETERS = 16
	};
	enum LogParameters {
		log_param_vtm5_pitch
	};

	struct Configuration {
		TFloat outputRate;                  // output sample rate (22.05, 44.1)
		int    waveform;                    // GS waveform type (0=PULSE, 1=SINE)
		TFloat tp;                          // % glottal pulse rise time
		TFloat tnMin;                       // % glottal pulse fall time minimum
		TFloat tnMax;                       // % glottal pulse fall time maximum
		TFloat breathiness;                 // % glottal source breathiness
		TFloat length;                      // nominal tube length (10 - 20 cm)
		TFloat temperature;                 // tube temperature (25 - 40 C)
		TFloat lossFactor;                  // junction loss factor in (0 - 5 %)
		// Set nasalRadius[N1] to 0.0, because it is not used.
		std::array<TFloat, TOTAL_NASAL_SECTIONS> nasalRadius; // fixed nasal radii (0 - 3 cm)
		int    modulation;                  // pulse mod. of noise (0=OFF, 1=ON)
		TFloat mixOffset;                   // noise crossmix offset (30 - 60 dB)
		std::array<TFloat, TOTAL_REGIONS> radiusCoef;
		TFloat glottalNoiseCutoff;          // glottal noise lowpass cutoff frequency (Hz)
		TFloat fricationNoiseCutoff;        // frication noise lowpass cutoff frequency (Hz)
		TFloat fricationFactor;
		TFloat minGlottalLoss;              // minimum loss at glottis (%)
		TFloat maxGlottalLoss;              // maximum loss at glottis (%)
		TFloat glottalLowpassCutoff;        // glottal wave lowpass cutoff frequency (Hz)
		int    bypass;
	};
	struct Junction2 {
		TFloat coeff{};
		void configure(TFloat leftRadius, TFloat rightRadius) {
			const TFloat r0_2 =  leftRadius *  leftRadius;
			const TFloat r1_2 = rightRadius * rightRadius;
			coeff = (r0_2 - r1_2) / (r0_2 + r1_2);
		}
	};
	struct Junction3 {
		TFloat leftCoeff{};
		TFloat rightCoeff{};
		TFloat upperCoeff{};
		void configure(TFloat leftRadius, TFloat rightRadius, TFloat upperRadius) {
			// Flow equations.
			const TFloat r0_2 =  leftRadius *  leftRadius;
			const TFloat r1_2 = rightRadius * rightRadius;
			const TFloat r2_2 = upperRadius * upperRadius;
			const TFloat c = 1.0f / (r0_2 + r1_2 + r2_2);
			leftCoeff  = c * (r0_2 - r1_2 - r2_2);
			rightCoeff = c * (r1_2 - r0_2 - r2_2);
			upperCoeff = c * (r2_2 - r0_2 - r1_2);
		}
	};
	struct Section {
		std::array<TFloat, SectionDelay + 1> top{};
		std::array<TFloat, SectionDelay + 1> bottom{};
		void reset() {
			top.fill(0.0);
			bottom.fill(0.0);
		}
		static void movePointers(unsigned int& in, unsigned int& out) {
			in = out;
			if (out == SectionDelay) {
				out = 0;
			} else {
				++out;
			}
		}
	};

	void propagate(Section& left, Section& right) {
		right.top[  inPtr_] = left.top[    outPtr_] * dampingFactor_;
		left.bottom[inPtr_] = right.bottom[outPtr_] * dampingFactor_;
	}
	void propagateJunction(Section& left, Junction2& junction, Section& right) {
		// Flow equations.
		const TFloat delta = junction.coeff * (left.top[outPtr_] + right.bottom[outPtr_]);
		right.top[  inPtr_] = (left.top[    outPtr_] - delta) * dampingFactor_;
		left.bottom[inPtr_] = (right.bottom[outPtr_] + delta) * dampingFactor_;
	}
	void propagateJunction(Section& left, Junction3& junction, Section& right, Section& upper) {
		// Flow equations.
		const TFloat partialInflux = left.top[outPtr_] + right.bottom[outPtr_] + upper.bottom[outPtr_];
		left.bottom[inPtr_] = (right.bottom[outPtr_] + upper.bottom[outPtr_] + junction.leftCoeff  * partialInflux) * dampingFactor_;
		right.top[  inPtr_] = ( left.top[   outPtr_] + upper.bottom[outPtr_] + junction.rightCoeff * partialInflux) * dampingFactor_;
		upper.top[  inPtr_] = ( left.top[   outPtr_] + right.bottom[outPtr_] + junction.upperCoeff * partialInflux) * dampingFactor_;
	}

	VocalTractModel5(const VocalTractModel5&) = delete;
	VocalTractModel5& operator=(const VocalTractModel5&) = delete;
	VocalTractModel5(VocalTractModel5&&) = delete;
	VocalTractModel5& operator=(VocalTractModel5&&) = delete;

	void loadConfiguration(const ConfigurationData& data);
	void initializeSynthesizer();
	void calculateTubeCoefficients();
	void initializeNasalCavity();
	TFloat vocalTract(TFloat input, TFloat frication, TFloat glottalLossFactor);

	bool interactive_;
	bool logParameters_;
	Configuration config_;

	/*  DERIVED VALUES  */
	TFloat sampleRate_;

	/*  MEMORY FOR TUBE AND TUBE COEFFICIENTS  */
	std::array<Section, TOTAL_SECTIONS> oropharynx_;
	Junction2 oropharynxJunction_[TOTAL_JUNCTIONS];
	std::array<Section, TOTAL_NASAL_SECTIONS> nasal_;
	Junction2 nasalJunction_[TOTAL_NASAL_JUNCTIONS];
	Junction3 velumJunction_;
	unsigned int inPtr_;
	unsigned int outPtr_;

	TFloat dampingFactor_;               /*  calculated damping factor  */
	TFloat crossmixFactor_;              /*  calculated crossmix factor  */
	TFloat breathinessFactor_;

	std::array<TFloat, TOTAL_PARAMETERS>                currentParameter_;
	std::vector<float>                                  outputBuffer_;
	std::unique_ptr<SampleRateConverter<TFloat>>        srConv_;
	std::unique_ptr<PoleZeroRadiationImpedance<TFloat>> mouthRadiationImpedance_;
	std::unique_ptr<PoleZeroRadiationImpedance<TFloat>> nasalRadiationImpedance_;
	std::unique_ptr<RosenbergBGlottalSource<TFloat>>    glottalSource_;
	std::unique_ptr<BandpassFilter<TFloat>>             bandpassFilter_;
	std::unique_ptr<Butterworth1LowPassFilter<TFloat>>  glottalNoiseFilter_;
	std::unique_ptr<Butterworth2LowPassFilter<TFloat>>  fricationNoiseFilter_;
	std::unique_ptr<NoiseSource>                        noiseSource_;
	std::unique_ptr<Butterworth1LowPassFilter<TFloat>>  glottalFilter_;
	DifferenceFilter<float>                             outputDiffFilter_;
	ParameterLogger<TFloat>                             paramLogger_;

	bool constantRadiusMouthImpedance_;
	TFloat mouthImpedanceRadius_;
};



template<typename TFloat, unsigned int SectionDelay>
VocalTractModel5<TFloat, SectionDelay>::VocalTractModel5(const ConfigurationData& data, bool interactive)
		: interactive_(interactive)
		, logParameters_()
		, constantRadiusMouthImpedance_()
		, mouthImpedanceRadius_()
{
	loadConfiguration(data);
	VocalTractModel5::reset();
	initializeSynthesizer();
	outputBuffer_.reserve(OUTPUT_BUFFER_RESERVE);
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel5<TFloat, SectionDelay>::loadConfiguration(const ConfigurationData& data)
{
	config_.outputRate           = data.value<TFloat>("output_rate");
	config_.waveform             = data.value<int>("waveform");
	config_.tp                   = data.value<TFloat>("glottal_pulse_tp");
	config_.tnMin                = data.value<TFloat>("glottal_pulse_tn_min");
	config_.tnMax                = data.value<TFloat>("glottal_pulse_tn_max");
	config_.breathiness          = data.value<TFloat>("breathiness");
	config_.length               = data.value<TFloat>("vocal_tract_length_offset") + data.value<TFloat>("vocal_tract_length");
	if (config_.length < MIN_VOCAL_TRACT_LENGTH) {
		config_.length = MIN_VOCAL_TRACT_LENGTH;
	} else if (config_.length > MAX_VOCAL_TRACT_LENGTH) {
		config_.length = MAX_VOCAL_TRACT_LENGTH;
	}
	config_.temperature          = data.value<TFloat>("temperature");
	config_.lossFactor           = data.value<TFloat>("loss_factor");
	config_.modulation           = data.value<int>("noise_modulation");
	config_.mixOffset            = data.value<TFloat>("mix_offset");
	const TFloat globalRadiusCoef      = data.value<TFloat>("global_radius_coef");
	const TFloat globalNasalRadiusCoef = data.value<TFloat>("global_nasal_radius_coef");
	config_.nasalRadius[NR1]     = 0.0;
	config_.nasalRadius[NR2]     = data.value<TFloat>("nasal_radius_2") * globalNasalRadiusCoef;
	config_.nasalRadius[NR3]     = data.value<TFloat>("nasal_radius_3") * globalNasalRadiusCoef;
	config_.nasalRadius[NR4]     = data.value<TFloat>("nasal_radius_4") * globalNasalRadiusCoef;
	config_.nasalRadius[NR5]     = data.value<TFloat>("nasal_radius_5") * globalNasalRadiusCoef;
	config_.nasalRadius[NR6]     = data.value<TFloat>("nasal_radius_6") * globalNasalRadiusCoef;
	config_.nasalRadius[NR7]     = data.value<TFloat>("nasal_radius_7") * globalNasalRadiusCoef;
	config_.radiusCoef[R1]       = data.value<TFloat>("radius_1_coef") * globalRadiusCoef;
	config_.radiusCoef[R2]       = data.value<TFloat>("radius_2_coef") * globalRadiusCoef;
	config_.radiusCoef[R3]       = data.value<TFloat>("radius_3_coef") * globalRadiusCoef;
	config_.radiusCoef[R4]       = data.value<TFloat>("radius_4_coef") * globalRadiusCoef;
	config_.radiusCoef[R5]       = data.value<TFloat>("radius_5_coef") * globalRadiusCoef;
	config_.radiusCoef[R6]       = data.value<TFloat>("radius_6_coef") * globalRadiusCoef;
	config_.radiusCoef[R7]       = data.value<TFloat>("radius_7_coef") * globalRadiusCoef;
	config_.radiusCoef[R8]       = data.value<TFloat>("radius_8_coef") * globalRadiusCoef;
	config_.glottalNoiseCutoff   = data.value<TFloat>("glottal_noise_cutoff");
	config_.fricationNoiseCutoff = data.value<TFloat>("frication_noise_cutoff");
	config_.fricationFactor      = data.value<TFloat>("frication_factor");
	config_.minGlottalLoss       = data.value<TFloat>("min_glottal_loss");
	config_.maxGlottalLoss       = data.value<TFloat>("max_glottal_loss");
	config_.glottalLowpassCutoff = data.value<TFloat>("glottal_lowpass_cutoff");
	config_.bypass               = data.value<int>("bypass");

	logParameters_ = interactive_ ? false : data.value<bool>("log_parameters");

	constantRadiusMouthImpedance_ = data.value<bool>("constant_radius_mouth_impedance");
	if (constantRadiusMouthImpedance_) {
		mouthImpedanceRadius_ = data.value<TFloat>("mouth_impedance_radius");
	}
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel5<TFloat, SectionDelay>::reset() noexcept
{
	for (auto& elem : oropharynx_) {
		elem.reset();
	}
	for (auto& elem : nasal_) {
		elem.reset();
	}
	inPtr_  = 0;
	outPtr_ = 1;
	currentParameter_.fill(0.0);
	outputBuffer_.clear();
	if (srConv_)                  srConv_->reset();
	if (mouthRadiationImpedance_) mouthRadiationImpedance_->reset();
	if (nasalRadiationImpedance_) nasalRadiationImpedance_->reset();
	if (glottalSource_)           glottalSource_->reset();
	if (bandpassFilter_)          bandpassFilter_->reset();
	if (glottalNoiseFilter_)      glottalNoiseFilter_->reset();
	if (fricationNoiseFilter_)    fricationNoiseFilter_->reset();
	if (noiseSource_)             noiseSource_->reset();
	outputDiffFilter_.reset();
}

/******************************************************************************
*
*  function:  initializeSynthesizer
*
*  purpose:   Initializes all variables so that the synthesis can
*             be run.
*
******************************************************************************/
template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel5<TFloat, SectionDelay>::initializeSynthesizer()
{
	/*  CALCULATE THE SAMPLE RATE, BASED ON NOMINAL TUBE LENGTH AND SPEED OF SOUND  */
	const TFloat c = Util::speedOfSound(config_.temperature);
	sampleRate_ = (c * (TOTAL_SECTIONS * SectionDelay) * 100.0f) / config_.length;
	if (!interactive_) LOG_DEBUG("[VocalTractModel5] Internal sample rate: " << sampleRate_);

	/*  CALCULATE THE BREATHINESS FACTOR  */
	breathinessFactor_ = config_.breathiness / 100.0f;

	/*  CALCULATE CROSSMIX FACTOR  */
	crossmixFactor_ = 1.0f / Util::amplitude60dB(config_.mixOffset);

	/*  CALCULATE THE DAMPING FACTOR  */
	dampingFactor_ = 1.0f - (config_.lossFactor / 100.0f);

	/*  INITIALIZE THE WAVE TABLE  */
	glottalSource_ = std::make_unique<RosenbergBGlottalSource<TFloat>>(
						config_.waveform == GLOTTAL_SOURCE_PULSE ?
							RosenbergBGlottalSource<TFloat>::Type::pulse :
							RosenbergBGlottalSource<TFloat>::Type::sine,
						sampleRate_,
						config_.tp, config_.tnMin, config_.tnMax);

	/*  INITIALIZE RADIATION IMPEDANCE FOR MOUTH  */
	mouthRadiationImpedance_ = std::make_unique<PoleZeroRadiationImpedance<TFloat>>(sampleRate_);

	if (constantRadiusMouthImpedance_) {
		mouthRadiationImpedance_->update(mouthImpedanceRadius_ * 1.0e-2f /* cm --> m */);
	}

	/*  INITIALIZE RADIATION IMPEDANCE FOR NOSE  */
	nasalRadiationImpedance_ = std::make_unique<PoleZeroRadiationImpedance<TFloat>>(sampleRate_);

	/*  INITIALIZE NASAL CAVITY FIXED SCATTERING COEFFICIENTS  */
	initializeNasalCavity();

	/*  INITIALIZE THE SAMPLE RATE CONVERSION ROUTINES  */
	if (config_.bypass == 1) {
		srConv_ = std::make_unique<SampleRateConverter<TFloat>>(
						sampleRate_,
						config_.outputRate,
						[&](float sample) {
							outputBuffer_.push_back(sample);
						});
	} else {
		srConv_ = std::make_unique<SampleRateConverter<TFloat>>(
						sampleRate_,
						config_.outputRate,
						[&](float sample) {
							// Does not use the 0.5 factor.
							outputBuffer_.push_back(outputDiffFilter_.filter(sample) * config_.outputRate);
						});
	}

	bandpassFilter_       = std::make_unique<BandpassFilter<TFloat>>();
	glottalNoiseFilter_   = std::make_unique<Butterworth1LowPassFilter<TFloat>>();
	glottalNoiseFilter_->update(sampleRate_, config_.glottalNoiseCutoff);
	fricationNoiseFilter_ = std::make_unique<Butterworth2LowPassFilter<TFloat>>();
	fricationNoiseFilter_->update(sampleRate_, config_.fricationNoiseCutoff);
	noiseSource_          = std::make_unique<NoiseSource>();
	glottalFilter_        = std::make_unique<Butterworth1LowPassFilter<TFloat>>();
	glottalFilter_->update(sampleRate_, config_.glottalLowpassCutoff);
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel5<TFloat, SectionDelay>::execSynthesisStep() noexcept
{
	/*  CONVERT PARAMETERS HERE  */
	const TFloat f0 = Util::frequency(currentParameter_[PARAM_GLOT_PITCH]);
	const TFloat glotAmplitude = Util::amplitude60dB(currentParameter_[PARAM_GLOT_VOL]);
	const TFloat aspAmplitude = Util::amplitude60dB(currentParameter_[PARAM_ASP_VOL]);
	calculateTubeCoefficients();
	bandpassFilter_->update(sampleRate_, currentParameter_[PARAM_FRIC_BW], currentParameter_[PARAM_FRIC_CF]);

	const TFloat noiseSample = noiseSource_->getSample();

	/*  DO SYNTHESIS HERE  */
	/*  CREATE LOW-PASS FILTERED NOISE  */
	const TFloat glottalNoise = glottalNoiseFilter_->filter(noiseSample);

	/*  UPDATE THE SHAPE OF THE GLOTTAL PULSE, IF NECESSARY  */
	if (config_.waveform == GLOTTAL_SOURCE_PULSE) {
		glottalSource_->setup(glotAmplitude);
	}

	/*  CREATE GLOTTAL PULSE (OR SINE TONE)  */
	const TFloat pulse = glottalFilter_->filter(glottalSource_->getSample(f0));

	/*  CREATE PULSED NOISE  */
	const TFloat pulsedNoise = glottalNoise * pulse;

	/*  CREATE NOISY GLOTTAL PULSE  */
	const TFloat noisyPulse = glotAmplitude * (pulse * (1.0f - breathinessFactor_) + pulsedNoise * breathinessFactor_);

	TFloat fricationNoise = fricationNoiseFilter_->filter(noiseSample);
	/*  CROSS-MIX PURE NOISE WITH PULSED NOISE  */
	if (config_.modulation) {
		TFloat crossmix = glotAmplitude * crossmixFactor_;
		crossmix = (crossmix < 1.0f) ? crossmix : 1.0f;
		fricationNoise = fricationNoise * (noisyPulse * crossmix + (1.0f - crossmix));
	}

	TFloat signal;
	if (config_.bypass == 1) {
		// Get glottal waveform.
		signal = noisyPulse + aspAmplitude * fricationNoise;
	} else {
		const TFloat minGlottalLossFactor = 1.0f - glotAmplitude * (config_.minGlottalLoss / 100.0f);
		const TFloat maxGlottalLossFactor = 1.0f - glotAmplitude * (config_.maxGlottalLoss / 100.0f);
		const TFloat glottalLossFactor = minGlottalLossFactor + (maxGlottalLossFactor - minGlottalLossFactor) * pulse;

		signal = vocalTract(noisyPulse + aspAmplitude * fricationNoise,
							config_.fricationFactor * bandpassFilter_->filter(fricationNoise),
							glottalLossFactor);
	}
	// Send to output.
	srConv_->dataFill(interactive_ ? signal / f0 : signal); // divide by f0 to compensate for the differentiation at the output

	if (logParameters_) GS_LOG_PARAMETER(paramLogger_, log_param_vtm5_pitch, currentParameter_[PARAM_GLOT_PITCH]);
}

/******************************************************************************
*
*  function:  initializeNasalCavity
*
*  purpose:   Calculates the scattering coefficients for the fixed
*             sections of the nasal cavity.
*
******************************************************************************/
template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel5<TFloat, SectionDelay>::initializeNasalCavity()
{
	// Configure junctions for fixed nasal sections.
	for (int i = NJ2, j = NR2; i < NJ7; ++i, ++j) {
		nasalJunction_[i].configure(config_.nasalRadius[j], config_.nasalRadius[j + 1]);
	}

	const TFloat r = std::sqrt(0.5f * config_.nasalRadius[NR7] * config_.nasalRadius[NR7]);
	nasalRadiationImpedance_->update(r * 1.0e-2f /* cm --> m */);
}

/******************************************************************************
*
*  function:  calculateTubeCoefficients
*
*  purpose:   Calculates the scattering coefficients for the vocal
*             ract according to the current radii.  Also calculates
*             the coefficients for the reflection/radiation filter
*             pair for the mouth and nose.
*
******************************************************************************/
template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel5<TFloat, SectionDelay>::calculateTubeCoefficients()
{
	// Configure oropharynx junctions.
	for (int i = J1, j = PARAM_R1; i < J8; ++i, ++j) {
		oropharynxJunction_[i].configure(currentParameter_[j], currentParameter_[j + 1]);
	}

	if (!constantRadiusMouthImpedance_) {
		mouthRadiationImpedance_->update(currentParameter_[PARAM_R8] * 1.0e-2f /* cm --> m */);
	}

	// Configure 3-way junction.
	// Note: Since junction is in middle of region 4, leftRadius = rightRadius.
	velumJunction_.configure(currentParameter_[PARAM_R4], currentParameter_[PARAM_R4], currentParameter_[PARAM_VELUM]);

	// Configure 1st nasal junction.
	nasalJunction_[NJ1].configure(currentParameter_[PARAM_VELUM], config_.nasalRadius[NR2]);
}

/******************************************************************************
*
*  function:  vocalTract
*
*  purpose:   Updates the pressure wave throughout the vocal tract,
*             and returns the summed output of the oral and nasal
*             cavities.  Also injects frication appropriately.
*
******************************************************************************/
template<typename TFloat, unsigned int SectionDelay>
TFloat
VocalTractModel5<TFloat, SectionDelay>::vocalTract(TFloat input, TFloat frication, TFloat glottalLossFactor)
{
	Section::movePointers(inPtr_, outPtr_);

	// Input to the tube.
	oropharynx_[S1].top[inPtr_] = oropharynx_[S1].bottom[outPtr_] * glottalLossFactor + input;

	propagate(oropharynx_[S1], oropharynx_[S2]);
	propagate(oropharynx_[S2], oropharynx_[S3]);
	propagateJunction(oropharynx_[S3], oropharynxJunction_[J1], oropharynx_[S4]);
	propagate(oropharynx_[S4], oropharynx_[S5]);
	propagateJunction(oropharynx_[S5], oropharynxJunction_[J2], oropharynx_[S6]);
	propagate(oropharynx_[S6], oropharynx_[S7]);
	propagate(oropharynx_[S7], oropharynx_[S8]);
	propagate(oropharynx_[S8], oropharynx_[S9]);
	propagateJunction(oropharynx_[S9], oropharynxJunction_[J3], oropharynx_[S10]);
	propagate(oropharynx_[S10], oropharynx_[S11]);
	propagate(oropharynx_[S11], oropharynx_[S12]);

	// 3-way junction between the middle of R4 and the nasal cavity.
	propagateJunction(oropharynx_[S12], velumJunction_, oropharynx_[S13], nasal_[VELUM]);

	propagate(oropharynx_[S13], oropharynx_[S14]);
	propagate(oropharynx_[S14], oropharynx_[S15]);
	propagateJunction(oropharynx_[S15], oropharynxJunction_[J4], oropharynx_[S16]);
	propagate(oropharynx_[S16], oropharynx_[S17]);
	propagate(oropharynx_[S17], oropharynx_[S18]);
	propagate(oropharynx_[S18], oropharynx_[S19]);
	propagate(oropharynx_[S19], oropharynx_[S20]);
	propagate(oropharynx_[S20], oropharynx_[S21]);
	propagateJunction(oropharynx_[S21], oropharynxJunction_[J5], oropharynx_[S22]);
	propagate(oropharynx_[S22], oropharynx_[S23]);
	propagate(oropharynx_[S23], oropharynx_[S24]);
	propagate(oropharynx_[S24], oropharynx_[S25]);
	propagateJunction(oropharynx_[S25], oropharynxJunction_[J6], oropharynx_[S26]);
	propagate(oropharynx_[S26], oropharynx_[S27]);
	propagateJunction(oropharynx_[S27], oropharynxJunction_[J7], oropharynx_[S28]);
	propagate(oropharynx_[S28], oropharynx_[S29]);
	propagate(oropharynx_[S29], oropharynx_[S30]);

	TFloat mouthOutputFlow;
	mouthRadiationImpedance_->process(oropharynx_[S30].top[outPtr_], mouthOutputFlow, oropharynx_[S30].bottom[inPtr_]);
	oropharynx_[S30].bottom[inPtr_] *= dampingFactor_;

	propagate(nasal_[N1], nasal_[N2]);
	propagate(nasal_[N2], nasal_[N3]);
	propagateJunction(nasal_[N3], nasalJunction_[NJ1], nasal_[N4]);
	propagate(nasal_[N4], nasal_[N5]);
	propagate(nasal_[N5], nasal_[N6]);
	propagateJunction(nasal_[N6], nasalJunction_[NJ2], nasal_[N7]);
	propagate(nasal_[N7], nasal_[N8]);
	propagate(nasal_[N8], nasal_[N9]);
	propagateJunction(nasal_[N9], nasalJunction_[NJ3], nasal_[N10]);
	propagate(nasal_[N10], nasal_[N11]);
	propagate(nasal_[N11], nasal_[N12]);
	propagateJunction(nasal_[N12], nasalJunction_[NJ4], nasal_[N13]);
	propagate(nasal_[N13], nasal_[N14]);
	propagate(nasal_[N14], nasal_[N15]);
	propagateJunction(nasal_[N15], nasalJunction_[NJ5], nasal_[N16]);
	propagate(nasal_[N16], nasal_[N17]);
	propagate(nasal_[N17], nasal_[N18]);
	propagateJunction(nasal_[N18], nasalJunction_[NJ6], nasal_[N19]);
	propagate(nasal_[N19], nasal_[N20]);
	propagate(nasal_[N20], nasal_[N21]);

	TFloat nasalOutputFlow;
	nasalRadiationImpedance_->process(nasal_[N21].top[outPtr_], nasalOutputFlow, nasal_[N21].bottom[inPtr_]);
	nasal_[N21].bottom[inPtr_] *= dampingFactor_;

	// Add frication noise.
	const TFloat fricOffset = (S28 - S6) * (currentParameter_[PARAM_FRIC_POS] / TFloat{GS_VTM5_MAX_FRIC_POS - GS_VTM5_MIN_FRIC_POS});
	const int fricOffsetInt = static_cast<int>(fricOffset);
	const TFloat fricRight = fricOffset - fricOffsetInt;
	const TFloat fricLeft = 1.0f - fricRight;
	const TFloat fricationAmplitude = Util::amplitude60dB(currentParameter_[PARAM_FRIC_VOL]);
	const TFloat fricValue = fricationAmplitude * frication;
	oropharynx_[S6 + fricOffsetInt].top[inPtr_] += fricValue * fricLeft;
	if (S6 + fricOffsetInt < S28) {
		oropharynx_[S6 + fricOffsetInt + 1].top[inPtr_] += fricValue * fricRight;
	}

	// Return summed output from mouth and nose.
	return mouthOutputFlow + nasalOutputFlow;
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel5<TFloat, SectionDelay>::setParameter(int parameter, float value) noexcept
{
	switch (parameter) {
	case PARAM_GLOT_PITCH:
	case PARAM_GLOT_VOL:
	case PARAM_ASP_VOL:
	case PARAM_FRIC_VOL:
	case PARAM_FRIC_POS:
	case PARAM_FRIC_CF:
	case PARAM_FRIC_BW:
	case PARAM_VELUM:
		currentParameter_[parameter] = value;
		break;
	case PARAM_R1:
	case PARAM_R2:
	case PARAM_R3:
	case PARAM_R4:
	case PARAM_R5:
	case PARAM_R6:
	case PARAM_R7:
	case PARAM_R8:
		currentParameter_[parameter] = std::max(
						value * config_.radiusCoef[parameter - PARAM_R1],
						TFloat{GS_VTM5_MIN_RADIUS});
		break;
	default:
		// Invalid parameter index.
		return; // fail silently
	}
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel5<TFloat, SectionDelay>::setAllParameters(const std::vector<float>& parameters) noexcept
{
	if (parameters.size() != TOTAL_PARAMETERS) {
		// Wrong number of parameters.
		return; // fail silently
	}

	for (std::size_t i = PARAM_GLOT_PITCH; i <= PARAM_FRIC_BW; ++i) {
		currentParameter_[i] = parameters[i];
	}

	for (std::size_t i = PARAM_R1; i <= PARAM_R8; ++i) {
		currentParameter_[i] = std::max(
					parameters[i] * config_.radiusCoef[i - PARAM_R1],
					TFloat{GS_VTM5_MIN_RADIUS});
	}

	currentParameter_[PARAM_VELUM] = parameters[PARAM_VELUM];
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel5<TFloat, SectionDelay>::finishSynthesis() noexcept
{
	srConv_->flushBuffer();
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_VOCAL_TRACT_MODEL_5_H_ */
