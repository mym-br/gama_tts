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

//                                                                         NJ1         NJ2         NJ3         NJ4         NJ5         NJ6
//                                                                         |           |           |           |           |           |
//                                                             VELUM=NR1   | NR2       | NR3       | NR4       | NR5       | NR6       |
//                                                       nasal -------------------------------------------------------------------------
//                                                          ___|N1 |N2 |N3 |N4 |N5 |N6 |N7 |N8 |N9 |N10|N11|N12|N13|N14|N15|N16|N17|N18| nose
//                                                         |   -------------------------------------------------------------------------
//         oropharynx                                      |
//         -------------------------------------------------------------------------------------------------------------------------
// vocal   |S1 |S2 |S3 |S4 |S5 |S6 |S7 |S8 |S9 |S10|S11|S12|S13|S14|S15|S16|S17|S18|S19|S20|S21|S22|S23|S24|S25|S26|S27|S28|S29|S30| mouth
// folds   -------------------------------------------------------------------------------------------------------------------------
//                     |       |               |           |           |           |           |               |       |           |
//           R1        | R2    | R3            | R4        | R4        | R5        | R5        | R6            | R7    | R8        |
//                     J1      J2              J3          |           J4          |           J5              J6      J7          J8
//                             FC1             FC2         FC3         FC4         FC5         FC6             FC7     FC8

// Note:
// - NoiseFilter has not been adapted for the increased samplerate.

#ifndef VTM_VOCAL_TRACT_MODEL_4_H_
#define VTM_VOCAL_TRACT_MODEL_4_H_

#include <algorithm> /* max */
#include <array>
#include <cstddef> /* std::size_t */
#include <memory>
#include <vector>

#include "BandpassFilter.h"
#include "ConfigurationData.h"
#include "Log.h"
#include "NoiseFilter.h"
#include "NoiseSource.h"
#include "RadiationFilter.h"
#include "ReflectionFilter.h"
#include "SampleRateConverter.h"
#include "Throat.h"
#include "VocalTractModel.h"
#include "VTMUtil.h"
#include "WavetableGlottalSource.h"

#define GS_VTM4_MIN_RADIUS (0.01)

/*  SCALING CONSTANT FOR INPUT TO VOCAL TRACT & THROAT (MATCHES DSP)  */
//#define GS_VTM4_VT_SCALE                  0.03125     /*  2^(-5)  */
// this is a temporary fix only, to try to match dsp synthesizer
#define GS_VTM4_VT_SCALE                  0.125     /*  2^(-3)  */



namespace GS {
namespace VTM {

template<typename TFloat, unsigned int SectionDelay>
class VocalTractModel4 : public VocalTractModel {
public:
	explicit VocalTractModel4(const ConfigurationData& data, bool interactive=false);
	virtual ~VocalTractModel4() noexcept = default;

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
		TOTAL_NASAL_REGIONS = 6
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
		TOTAL_NASAL_SECTIONS = 18
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
		NJ6 = 5, /*  N18 - AIR  */
		TOTAL_NASAL_JUNCTIONS = TOTAL_NASAL_SECTIONS / 3
	};
	enum { /*  FRICATION INJECTION COEFFICIENTS  */
		FC1 = 0, /*  S6   */
		FC2 = 1, /*  S10  */
		FC3 = 2, /*  S13  */
		FC4 = 3, /*  S16  */
		FC5 = 4, /*  S19  */
		FC6 = 5, /*  S22  */
		FC7 = 6, /*  S26  */
		FC8 = 7, /*  S28  */
		TOTAL_FRIC_COEFFICIENTS = 8
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

	struct Configuration {
		TFloat outputRate;                  /*  output sample rate (22.05, 44.1)  */
		int    waveform;                    /*  GS waveform type (0=PULSE, 1=SINE  */
		TFloat tp;                          /*  % glottal pulse rise time  */
		TFloat tnMin;                       /*  % glottal pulse fall time minimum  */
		TFloat tnMax;                       /*  % glottal pulse fall time maximum  */
		TFloat breathiness;                 /*  % glottal source breathiness  */
		TFloat length;                      /*  nominal tube length (10 - 20 cm)  */
		TFloat temperature;                 /*  tube temperature (25 - 40 C)  */
		TFloat lossFactor;                  /*  junction loss factor in (0 - 5 %)  */
		TFloat apertureRadius;              /*  aperture scl. radius (3.05 - 12 cm)  */
		TFloat mouthCoef;                   /*  mouth aperture coefficient  */
		TFloat noseCoef;                    /*  nose aperture coefficient  */
		// Set nasalRadius[N1] to 0.0, because it is not used.
		std::array<TFloat, TOTAL_NASAL_SECTIONS> nasalRadius; /*  fixed nasal radii (0 - 3 cm)  */
		TFloat throatCutoff;                /*  throat lp cutoff (50 - nyquist Hz)  */
		TFloat throatVol;                   /*  throat volume (0 - 48 dB) */
		int    modulation;                  /*  pulse mod. of noise (0=OFF, 1=ON)  */
		TFloat mixOffset;                   /*  noise crossmix offset (30 - 60 dB)  */
		std::array<TFloat, TOTAL_REGIONS> radiusCoef;
	};

	struct Junction2 {
		TFloat coeff{};
		void configure(TFloat leftRadius, TFloat rightRadius) {
			const TFloat r0_2 = leftRadius * leftRadius;
			const TFloat r1_2 = rightRadius * rightRadius;
			coeff = (r0_2 - r1_2) / (r0_2 + r1_2);
		}
	};
	struct Junction3 {
		TFloat leftAlpha{};
		TFloat rightAlpha{};
		TFloat upperAlpha{};
		void configure(TFloat leftRadius, TFloat rightRadius, TFloat upperRadius) {
			const TFloat r0_2 = leftRadius * leftRadius;
			const TFloat r1_2 = rightRadius * rightRadius;
			const TFloat r2_2 = upperRadius * upperRadius;
			const TFloat sum = 2.0f / (r0_2 + r1_2 + r2_2);
			leftAlpha  = sum * r0_2;
			rightAlpha = sum * r1_2;
			upperAlpha = sum * r2_2;
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
		// Simple copy.
		right.top[  inPtr_] = left.top[    outPtr_];
		left.bottom[inPtr_] = right.bottom[outPtr_];
	}
	void propagate(Section& left, Section& right, TFloat fricNoise) {
		// With frication and damping.
		right.top[  inPtr_] = left.top[    outPtr_] * dampingFactor_ + fricNoise;
		left.bottom[inPtr_] = right.bottom[outPtr_] * dampingFactor_;
	}
	void propagateJunction(Section& left, Junction2& junction, Section& right, TFloat fricNoise = 0.0) {
		// With frication and damping.
		const TFloat delta = junction.coeff * (left.top[outPtr_] - right.bottom[outPtr_]);
		right.top[  inPtr_] = (left.top[    outPtr_] + delta) * dampingFactor_ + fricNoise;
		left.bottom[inPtr_] = (right.bottom[outPtr_] + delta) * dampingFactor_;
	}
	void propagateJunction(Section& left, Junction3& junction, Section& right, Section& upper, TFloat fricNoise) {
		// With frication and damping.
		const TFloat junctionPressure =
				junction.leftAlpha  *  left.top[   outPtr_] +
				junction.rightAlpha * right.bottom[outPtr_] +
				junction.upperAlpha * upper.bottom[outPtr_];
		left.bottom[inPtr_] = (junctionPressure -  left.top[   outPtr_]) * dampingFactor_;
		right.top[  inPtr_] = (junctionPressure - right.bottom[outPtr_]) * dampingFactor_ + fricNoise;
		upper.top[  inPtr_] = (junctionPressure - upper.bottom[outPtr_]) * dampingFactor_;
	}

	VocalTractModel4(const VocalTractModel4&) = delete;
	VocalTractModel4& operator=(const VocalTractModel4&) = delete;
	VocalTractModel4(VocalTractModel4&&) = delete;
	VocalTractModel4& operator=(VocalTractModel4&&) = delete;

	void loadConfiguration(const ConfigurationData& data);
	void initializeSynthesizer();
	void calculateTubeCoefficients();
	void initializeNasalCavity();
	void setFricationTaps();
	TFloat vocalTract(TFloat input, TFloat frication);

	bool interactive_;
	Configuration config_;

	/*  DERIVED VALUES  */
	int sampleRate_;

	/*  MEMORY FOR TUBE AND TUBE COEFFICIENTS  */
	std::array<Section, TOTAL_SECTIONS> oropharynx_;
	Junction2 oropharynxJunction_[TOTAL_JUNCTIONS];
	std::array<Section, TOTAL_NASAL_SECTIONS> nasal_;
	Junction2 nasalJunction_[TOTAL_NASAL_JUNCTIONS];
	Junction3 velumJunction_;
	unsigned int inPtr_;
	unsigned int outPtr_;

	/*  MEMORY FOR FRICATION TAPS  */
	std::array<TFloat, TOTAL_FRIC_COEFFICIENTS> fricationTap_;

	TFloat dampingFactor_;               /*  calculated damping factor  */
	TFloat crossmixFactor_;              /*  calculated crossmix factor  */
	TFloat breathinessFactor_;

	std::array<TFloat, TOTAL_PARAMETERS>             currentParameter_;
	std::vector<float>                               outputBuffer_;
	std::unique_ptr<SampleRateConverter<TFloat>>     srConv_;
	std::unique_ptr<RadiationFilter<TFloat>>         mouthRadiationFilter_;
	std::unique_ptr<ReflectionFilter<TFloat>>        mouthReflectionFilter_;
	std::unique_ptr<RadiationFilter<TFloat>>         nasalRadiationFilter_;
	std::unique_ptr<ReflectionFilter<TFloat>>        nasalReflectionFilter_;
	std::unique_ptr<Throat<TFloat>>                  throat_;
	std::unique_ptr<WavetableGlottalSource<TFloat>>  glottalSource_;
	std::unique_ptr<BandpassFilter<TFloat>>          bandpassFilter_;
	std::unique_ptr<NoiseFilter<TFloat>>             noiseFilter_;
	std::unique_ptr<NoiseSource>                     noiseSource_;
};



template<typename TFloat, unsigned int SectionDelay>
VocalTractModel4<TFloat, SectionDelay>::VocalTractModel4(const ConfigurationData& data, bool interactive)
		: interactive_(interactive)
{
	loadConfiguration(data);
	VocalTractModel4::reset();
	initializeSynthesizer();
	outputBuffer_.reserve(OUTPUT_BUFFER_RESERVE);
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel4<TFloat, SectionDelay>::loadConfiguration(const ConfigurationData& data)
{
	config_.outputRate     = data.value<TFloat>("output_rate");
	config_.waveform       = data.value<int>("waveform");
	config_.tp             = data.value<TFloat>("glottal_pulse_tp");
	config_.tnMin          = data.value<TFloat>("glottal_pulse_tn_min");
	config_.tnMax          = data.value<TFloat>("glottal_pulse_tn_max");
	config_.breathiness    = data.value<TFloat>("breathiness");
	config_.length         = data.value<TFloat>("vocal_tract_length_offset") + data.value<TFloat>("vocal_tract_length");
	if (config_.length < MIN_VOCAL_TRACT_LENGTH) {
		config_.length = MIN_VOCAL_TRACT_LENGTH;
	} else if (config_.length > MAX_VOCAL_TRACT_LENGTH) {
		config_.length = MAX_VOCAL_TRACT_LENGTH;
	}
	config_.temperature    = data.value<TFloat>("temperature");
	config_.lossFactor     = data.value<TFloat>("loss_factor");
	config_.mouthCoef      = data.value<TFloat>("mouth_coefficient");
	config_.noseCoef       = data.value<TFloat>("nose_coefficient");
	config_.throatCutoff   = data.value<TFloat>("throat_cutoff");
	config_.throatVol      = data.value<TFloat>("throat_volume");
	config_.modulation     = data.value<int>("noise_modulation");
	config_.mixOffset      = data.value<TFloat>("mix_offset");
	const TFloat globalRadiusCoef      = data.value<TFloat>("global_radius_coef");
	const TFloat globalNasalRadiusCoef = data.value<TFloat>("global_nasal_radius_coef");
	config_.apertureRadius = data.value<TFloat>("aperture_radius") * globalRadiusCoef;
	config_.nasalRadius[0] = 0.0;
	config_.nasalRadius[1] = data.value<TFloat>("nasal_radius_1") * globalNasalRadiusCoef;
	config_.nasalRadius[2] = data.value<TFloat>("nasal_radius_2") * globalNasalRadiusCoef;
	config_.nasalRadius[3] = data.value<TFloat>("nasal_radius_3") * globalNasalRadiusCoef;
	config_.nasalRadius[4] = data.value<TFloat>("nasal_radius_4") * globalNasalRadiusCoef;
	config_.nasalRadius[5] = data.value<TFloat>("nasal_radius_5") * globalNasalRadiusCoef;
	config_.radiusCoef[0]  = data.value<TFloat>("radius_1_coef") * globalRadiusCoef;
	config_.radiusCoef[1]  = data.value<TFloat>("radius_2_coef") * globalRadiusCoef;
	config_.radiusCoef[2]  = data.value<TFloat>("radius_3_coef") * globalRadiusCoef;
	config_.radiusCoef[3]  = data.value<TFloat>("radius_4_coef") * globalRadiusCoef;
	config_.radiusCoef[4]  = data.value<TFloat>("radius_5_coef") * globalRadiusCoef;
	config_.radiusCoef[5]  = data.value<TFloat>("radius_6_coef") * globalRadiusCoef;
	config_.radiusCoef[6]  = data.value<TFloat>("radius_7_coef") * globalRadiusCoef;
	config_.radiusCoef[7]  = data.value<TFloat>("radius_8_coef") * globalRadiusCoef;
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel4<TFloat, SectionDelay>::reset() noexcept
{
	for (auto& elem : oropharynx_) {
		elem.reset();
	}
	for (auto& elem : nasal_) {
		elem.reset();
	}
	inPtr_  = 0;
	outPtr_ = 1;
	outputBuffer_.clear();
	if (srConv_)                srConv_->reset();
	if (mouthRadiationFilter_)  mouthRadiationFilter_->reset();
	if (mouthReflectionFilter_) mouthReflectionFilter_->reset();
	if (nasalRadiationFilter_)  nasalRadiationFilter_->reset();
	if (nasalReflectionFilter_) nasalReflectionFilter_->reset();
	if (throat_)                throat_->reset();
	if (glottalSource_)         glottalSource_->reset();
	if (bandpassFilter_)        bandpassFilter_->reset();
	if (noiseFilter_)           noiseFilter_->reset();
	if (noiseSource_)           noiseSource_->reset();
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
VocalTractModel4<TFloat, SectionDelay>::initializeSynthesizer()
{
	TFloat nyquist;

	/*  CALCULATE THE SAMPLE RATE, BASED ON NOMINAL TUBE LENGTH AND SPEED OF SOUND  */
	const TFloat c = Util::speedOfSound(config_.temperature);
	sampleRate_ = static_cast<int>((c * (TOTAL_SECTIONS * SectionDelay) * 100.0f) / config_.length);
	nyquist = sampleRate_ / 2.0f;
	if (!interactive_) LOG_DEBUG("[VocalTractModel4] Internal sample rate: " << sampleRate_);

	/*  CALCULATE THE BREATHINESS FACTOR  */
	breathinessFactor_ = config_.breathiness / 100.0f;

	/*  CALCULATE CROSSMIX FACTOR  */
	crossmixFactor_ = 1.0f / Util::amplitude60dB(config_.mixOffset);

	/*  CALCULATE THE DAMPING FACTOR  */
	dampingFactor_ = 1.0f - (config_.lossFactor / 100.0f);

	/*  INITIALIZE THE WAVE TABLE  */
	glottalSource_ = std::make_unique<WavetableGlottalSource<TFloat>>(
						config_.waveform == GLOTTAL_SOURCE_PULSE ?
							WavetableGlottalSource<TFloat>::Type::pulse :
							WavetableGlottalSource<TFloat>::Type::sine,
						sampleRate_,
						config_.tp, config_.tnMin, config_.tnMax);

	/*  INITIALIZE REFLECTION AND RADIATION FILTER COEFFICIENTS FOR MOUTH  */
	TFloat mouthApertureCoeff = (nyquist - config_.mouthCoef) / nyquist;
	mouthRadiationFilter_  = std::make_unique<RadiationFilter<TFloat>>(mouthApertureCoeff);
	mouthReflectionFilter_ = std::make_unique<ReflectionFilter<TFloat>>(mouthApertureCoeff);

	/*  INITIALIZE REFLECTION AND RADIATION FILTER COEFFICIENTS FOR NOSE  */
	TFloat nasalApertureCoeff = (nyquist - config_.noseCoef) / nyquist;
	nasalRadiationFilter_  = std::make_unique<RadiationFilter<TFloat>>(nasalApertureCoeff);
	nasalReflectionFilter_ = std::make_unique<ReflectionFilter<TFloat>>(nasalApertureCoeff);

	/*  INITIALIZE NASAL CAVITY FIXED SCATTERING COEFFICIENTS  */
	initializeNasalCavity();

	/*  INITIALIZE THE THROAT LOWPASS FILTER  */
	throat_ = std::make_unique<Throat<TFloat>>(sampleRate_, config_.throatCutoff, Util::amplitude60dB(config_.throatVol));

	/*  INITIALIZE THE SAMPLE RATE CONVERSION ROUTINES  */
	srConv_ = std::make_unique<SampleRateConverter<TFloat>>(
					sampleRate_,
					config_.outputRate,
					[&](float sample) {
						outputBuffer_.push_back(sample);
					});

	bandpassFilter_ = std::make_unique<BandpassFilter<TFloat>>();
	noiseFilter_    = std::make_unique<NoiseFilter<TFloat>>();
	noiseSource_    = std::make_unique<NoiseSource>();
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel4<TFloat, SectionDelay>::execSynthesisStep() noexcept
{
	/*  CONVERT PARAMETERS HERE  */
	TFloat f0 = Util::frequency(currentParameter_[PARAM_GLOT_PITCH]);
	TFloat ax = Util::amplitude60dB(currentParameter_[PARAM_GLOT_VOL]);
	TFloat ah1 = Util::amplitude60dB(currentParameter_[PARAM_ASP_VOL]);
	calculateTubeCoefficients();
	setFricationTaps();
	bandpassFilter_->update(sampleRate_, currentParameter_[PARAM_FRIC_BW], currentParameter_[PARAM_FRIC_CF]);

	/*  DO SYNTHESIS HERE  */
	/*  CREATE LOW-PASS FILTERED NOISE  */
	TFloat lpNoise = noiseFilter_->filter(noiseSource_->getSample());

	/*  UPDATE THE SHAPE OF THE GLOTTAL PULSE, IF NECESSARY  */
	if (config_.waveform == GLOTTAL_SOURCE_PULSE) {
		glottalSource_->setup(ax);
	}

	/*  CREATE GLOTTAL PULSE (OR SINE TONE)  */
	TFloat pulse = glottalSource_->getSample(f0);

	/*  CREATE PULSED NOISE  */
	TFloat pulsedNoise = lpNoise * pulse;

	/*  CREATE NOISY GLOTTAL PULSE  */
	pulse = ax * ((pulse * (1.0f - breathinessFactor_)) +
			(pulsedNoise * breathinessFactor_));

	TFloat signal;
	/*  CROSS-MIX PURE NOISE WITH PULSED NOISE  */
	if (config_.modulation) {
		TFloat crossmix = ax * crossmixFactor_;
		crossmix = (crossmix < 1.0f) ? crossmix : 1.0f;
		signal = (pulsedNoise * crossmix) +
				(lpNoise * (1.0f - crossmix));
	} else {
		signal = lpNoise;
	}

	/*  PUT SIGNAL THROUGH VOCAL TRACT  */
	signal = vocalTract(((pulse + (ah1 * signal)) * TFloat{GS_VTM4_VT_SCALE}),
				bandpassFilter_->filter(signal));

	/*  PUT PULSE THROUGH THROAT  */
	signal += throat_->process(pulse * TFloat{GS_VTM4_VT_SCALE});

	/*  OUTPUT SAMPLE HERE  */
	srConv_->dataFill(signal);
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
VocalTractModel4<TFloat, SectionDelay>::initializeNasalCavity()
{
	// Configure junctions for fixed nasal sections.
	for (int i = NJ2, j = NR2; i < NJ6; ++i, ++j) {
		nasalJunction_[i].configure(config_.nasalRadius[j], config_.nasalRadius[j + 1]);
	}

	// Configure junction for the nose aperture.
	nasalJunction_[NJ6].configure(config_.nasalRadius[NR6], config_.apertureRadius);
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
VocalTractModel4<TFloat, SectionDelay>::calculateTubeCoefficients()
{
	// Configure oropharynx junctions.
	for (int i = J1, j = PARAM_R1; i < J8; ++i, ++j) {
		oropharynxJunction_[i].configure(currentParameter_[j], currentParameter_[j + 1]);
	}

	// Configure junction for the mouth aperture.
	oropharynxJunction_[J8].configure(currentParameter_[PARAM_R8], config_.apertureRadius);

	// Configure 3-way junction.
	// Note: Since junction is in middle of region 4, leftRadius = rightRadius.
	velumJunction_.configure(currentParameter_[PARAM_R4], currentParameter_[PARAM_R4], currentParameter_[PARAM_VELUM]);

	// Configure 1st nasal junction.
	nasalJunction_[NJ1].configure(currentParameter_[PARAM_VELUM], config_.nasalRadius[NR2]);
}

/******************************************************************************
*
*  function:  setFricationTaps
*
*  purpose:   Sets the frication taps according to the current
*             position and amplitude of frication.
*
******************************************************************************/
template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel4<TFloat, SectionDelay>::setFricationTaps()
{
	const TFloat fricationAmplitude = Util::amplitude60dB(currentParameter_[PARAM_FRIC_VOL]);

	/*  CALCULATE POSITION REMAINDER AND COMPLEMENT  */
	const int integerPart = static_cast<int>(currentParameter_[PARAM_FRIC_POS]);
	const TFloat complement = currentParameter_[PARAM_FRIC_POS] - integerPart;
	const TFloat remainder = 1.0f - complement;

	/*  SET THE FRICATION TAPS  */
	for (int i = FC1; i < TOTAL_FRIC_COEFFICIENTS; i++) {
		if (i == integerPart) {
			fricationTap_[i] = remainder * fricationAmplitude;
			if ((i + 1) < TOTAL_FRIC_COEFFICIENTS) {
				fricationTap_[++i] = complement * fricationAmplitude;
			}
		} else {
			fricationTap_[i] = 0.0;
		}
	}

#if 0
	/*  PRINT OUT  */
	printf("fricationTaps:  ");
	for (int i = FC1; i < TOTAL_FRIC_COEFFICIENTS; i++)
		printf("%.6f  ", fricationTap_[i]);
	printf("\n");
#endif
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
VocalTractModel4<TFloat, SectionDelay>::vocalTract(TFloat input, TFloat frication)
{
	Section::movePointers(inPtr_, outPtr_);

	// Input to the tube.
	oropharynx_[S1].top[inPtr_] = oropharynx_[S1].bottom[outPtr_] * dampingFactor_ + input;

	propagate(oropharynx_[S1], oropharynx_[S2]);
	propagate(oropharynx_[S2], oropharynx_[S3]);
	propagateJunction(oropharynx_[S3], oropharynxJunction_[J1], oropharynx_[S4]);
	propagate(oropharynx_[S4], oropharynx_[S5]);
	propagateJunction(oropharynx_[S5], oropharynxJunction_[J2], oropharynx_[S6], fricationTap_[FC1] * frication);
	propagate(oropharynx_[S6], oropharynx_[S7]);
	propagate(oropharynx_[S7], oropharynx_[S8]);
	propagate(oropharynx_[S8], oropharynx_[S9]);
	propagateJunction(oropharynx_[S9], oropharynxJunction_[J3], oropharynx_[S10], fricationTap_[FC2] * frication);
	propagate(oropharynx_[S10], oropharynx_[S11]);
	propagate(oropharynx_[S11], oropharynx_[S12]);

	// 3-way junction between the middle of R4 and the nasal cavity.
	propagateJunction(oropharynx_[S12], velumJunction_, oropharynx_[S13], nasal_[VELUM], fricationTap_[FC3] * frication);

	propagate(oropharynx_[S13], oropharynx_[S14]);
	propagate(oropharynx_[S14], oropharynx_[S15]);
	propagateJunction(oropharynx_[S15], oropharynxJunction_[J4], oropharynx_[S16], fricationTap_[FC4] * frication);
	propagate(oropharynx_[S16], oropharynx_[S17]);
	propagate(oropharynx_[S17], oropharynx_[S18]);
	propagate(oropharynx_[S18], oropharynx_[S19], fricationTap_[FC5] * frication);
	propagate(oropharynx_[S19], oropharynx_[S20]);
	propagate(oropharynx_[S20], oropharynx_[S21]);
	propagateJunction(oropharynx_[S21], oropharynxJunction_[J5], oropharynx_[S22] , fricationTap_[FC6] * frication);
	propagate(oropharynx_[S22], oropharynx_[S23]);
	propagate(oropharynx_[S23], oropharynx_[S24]);
	propagate(oropharynx_[S24], oropharynx_[S25]);
	propagateJunction(oropharynx_[S25], oropharynxJunction_[J6], oropharynx_[S26] , fricationTap_[FC7] * frication);
	propagate(oropharynx_[S26], oropharynx_[S27]);
	propagateJunction(oropharynx_[S27], oropharynxJunction_[J7], oropharynx_[S28] , fricationTap_[FC8] * frication);
	propagate(oropharynx_[S28], oropharynx_[S29]);
	propagate(oropharynx_[S29], oropharynx_[S30]);

	// Reflected signal at the mouth goes through a lowpass filter.
	oropharynx_[S30].bottom[inPtr_] = dampingFactor_ * mouthReflectionFilter_->filter(oropharynxJunction_[J8].coeff * oropharynx_[S30].top[outPtr_]);

	// Output from mouth goes through a highpass filter.
	TFloat output = mouthRadiationFilter_->filter((1.0f + oropharynxJunction_[J8].coeff) * oropharynx_[S30].top[outPtr_]);

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

	// Reflected signal at the nose goes through a lowpass filter.
	nasal_[N18].bottom[inPtr_] = dampingFactor_ * nasalReflectionFilter_->filter(nasalJunction_[NJ6].coeff * nasal_[N18].top[outPtr_]);

	// Output from nose goes through a highpass filter.
	output += nasalRadiationFilter_->filter((1.0f + nasalJunction_[NJ6].coeff) * nasal_[N18].top[outPtr_]);

	// Return summed output from mouth and nose.
	return output;
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel4<TFloat, SectionDelay>::setParameter(int parameter, float value) noexcept
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
						TFloat{GS_VTM4_MIN_RADIUS});
		break;
	default:
		// Invalid parameter index.
		return; // fail silently
	}
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel4<TFloat, SectionDelay>::setAllParameters(const std::vector<float>& parameters) noexcept
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
					TFloat{GS_VTM4_MIN_RADIUS});
	}

	currentParameter_[PARAM_VELUM] = parameters[PARAM_VELUM];
}

template<typename TFloat, unsigned int SectionDelay>
void
VocalTractModel4<TFloat, SectionDelay>::finishSynthesis() noexcept
{
	srConv_->flushBuffer();
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_VOCAL_TRACT_MODEL_4_H_ */
