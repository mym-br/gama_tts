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

//                                                                           NJ1         NJ2         NJ3         NJ4         NJ5         NJ6
//                                                                           |           |           |           |           |           |
//                                                               VELUM=NR1   | NR2       | NR3       | NR4       | NR5       | NR6       |
//                                                       nasal   -------------------------------------------------------------------------
//                                                          _____|N1 |N2 |N3 |N4 |N5 |N6 |N7 |N8 |N9 |N10|N11|N12|N13|N14|N15|N16|N17|N18| nose
//                                                         |     -------------------------------------------------------------------------
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

#ifndef VTM_VOCAL_TRACT_MODEL_5_H_
#define VTM_VOCAL_TRACT_MODEL_5_H_

#include <algorithm> /* max, min */
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <istream>
#include <memory>
#include <sstream>
#include <string>
#include <utility> /* move */
#include <vector>

#include "BandpassFilter.h"
#include "ConfigurationData.h"
#include "Exception.h"
#include "Log.h"
#include "MovingAverageFilter.h"
#include "NoiseFilter.h"
#include "NoiseSource.h"
#include "RadiationFilter.h"
#include "ReflectionFilter.h"
#include "SampleRateConverter.h"
#include "Text.h"
#include "Throat.h"
#include "VocalTractModel.h"
#include "VocalTractModelParameterValue.h"
#include "VTMUtil.h"
#include "WAVEFileWriter.h"
#include "WavetableGlottalSource.h"

#define GS_VTM_VOCAL_TRACT_MODEL_5_MIN_RADIUS (0.001)
#define GS_VTM_VOCAL_TRACT_MODEL_5_INPUT_FILTER_PERIOD_SEC (50.0e-3)

/*  SCALING CONSTANT FOR INPUT TO VOCAL TRACT & THROAT (MATCHES DSP)  */
//#define GS_VTM_VOCAL_TRACT_MODEL_5_VT_SCALE                  0.03125     /*  2^(-5)  */
// this is a temporary fix only, to try to match dsp synthesizer
#define GS_VTM_VOCAL_TRACT_MODEL_5_VT_SCALE                  0.125     /*  2^(-3)  */

/*  FINAL OUTPUT SCALING, SO THAT .SND FILES APPROX. MATCH DSP OUTPUT  */
#define GS_VTM_VOCAL_TRACT_MODEL_5_OUTPUT_SCALE              0.95



namespace GS {
namespace VTM {

template<typename FloatType, unsigned int SectionDelay>
class VocalTractModel5 : public VocalTractModel {
public:
	VocalTractModel5(const ConfigurationData& data, bool interactive = false);
	~VocalTractModel5() {}

	double outputRate() const { return config_.outputRate; }

	// ----- Batch mode.
	void synthesizeToFile(std::istream& inputStream, const char* outputFile);
	void synthesizeToBuffer(std::istream& inputStream, std::vector<float>& outputBuffer);

	// ----- Interactive mode.
	void loadSingleInput(const VocalTractModelParameterValue pv);
	// Synthesizes at least numberOfSamples samples (may synthesize more samples).
	void synthesizeSamples(std::size_t numberOfSamples);
	// Returns the number of samples read.
	std::size_t getOutputSamples(std::size_t n, float* buffer);

private:
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
		GLOTTAL_SOURCE_SINE = 1
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
		FloatType outputRate;                  /*  output sample rate (22.05, 44.1)  */
		FloatType controlRate;                 /*  1.0-1000.0 input tables/second (Hz)  */
		int       waveform;                    /*  GS waveform type (0=PULSE, 1=SINE  */
		FloatType tp;                          /*  % glottal pulse rise time  */
		FloatType tnMin;                       /*  % glottal pulse fall time minimum  */
		FloatType tnMax;                       /*  % glottal pulse fall time maximum  */
		FloatType breathiness;                 /*  % glottal source breathiness  */
		FloatType length;                      /*  nominal tube length (10 - 20 cm)  */
		FloatType temperature;                 /*  tube temperature (25 - 40 C)  */
		FloatType lossFactor;                  /*  junction loss factor in (0 - 5 %)  */
		FloatType apertureRadius;              /*  aperture scl. radius (3.05 - 12 cm)  */
		FloatType mouthCoef;                   /*  mouth aperture coefficient  */
		FloatType noseCoef;                    /*  nose aperture coefficient  */
		// Set nasalRadius[N1] to 0.0, because it is not used.
		std::array<FloatType, TOTAL_NASAL_SECTIONS> nasalRadius; /*  fixed nasal radii (0 - 3 cm)  */
		FloatType throatCutoff;                /*  throat lp cutoff (50 - nyquist Hz)  */
		FloatType throatVol;                   /*  throat volume (0 - 48 dB) */
		int       modulation;                  /*  pulse mod. of noise (0=OFF, 1=ON)  */
		FloatType mixOffset;                   /*  noise crossmix offset (30 - 60 dB)  */
		std::array<FloatType, TOTAL_REGIONS> radiusCoef;
	};

	struct InputFilters {
		std::array<MovingAverageFilter<FloatType>, TOTAL_PARAMETERS> filter;

		InputFilters(FloatType sampleRate, FloatType period)
			: filter {
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period),
				MovingAverageFilter<FloatType>(sampleRate, period)} {}
		void reset() {
			for (int i = 0; i < TOTAL_PARAMETERS; ++i) {
				filter[i].reset();
			}
		}
	};

	struct Junction2 {
		FloatType coeff {};
		void configure(FloatType leftRadius, FloatType rightRadius) {
			const FloatType r0_2 =  leftRadius *  leftRadius;
			const FloatType r1_2 = rightRadius * rightRadius;
			coeff = (r0_2 - r1_2) / (r0_2 + r1_2);
		}
	};
	struct Junction3 {
		FloatType leftCoeff  {};
		FloatType rightCoeff {};
		FloatType upperCoeff {};
		void configure(FloatType leftRadius, FloatType rightRadius, FloatType upperRadius) {
			// Flow equations.
			const FloatType r0_2 =  leftRadius *  leftRadius;
			const FloatType r1_2 = rightRadius * rightRadius;
			const FloatType r2_2 = upperRadius * upperRadius;
			const FloatType c = 1.0f / (r0_2 + r1_2 + r2_2);
			leftCoeff  = c * (r0_2 - r1_2 - r2_2);
			rightCoeff = c * (r1_2 - r0_2 - r2_2);
			upperCoeff = c * (r2_2 - r0_2 - r1_2);
		}
	};
	struct Section {
		std::array<FloatType, SectionDelay + 1> top    {};
		std::array<FloatType, SectionDelay + 1> bottom {};
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
	void propagate(Section& left, Section& right, FloatType fricNoise) {
		// With frication and damping.
		right.top[  inPtr_] = left.top[    outPtr_] * dampingFactor_ + fricNoise;
		left.bottom[inPtr_] = right.bottom[outPtr_] * dampingFactor_;
	}
	void propagateJunction(Section& left, Junction2& junction, Section& right, FloatType fricNoise = 0.0) {
		// With frication and damping.
		// Flow equations.
		const FloatType delta = junction.coeff * (left.top[outPtr_] + right.bottom[outPtr_]);
		right.top[  inPtr_] = (left.top[    outPtr_] - delta) * dampingFactor_ + fricNoise;
		left.bottom[inPtr_] = (right.bottom[outPtr_] + delta) * dampingFactor_;
	}
	void propagateJunction(Section& left, Junction3& junction, Section& right, Section& upper, FloatType fricNoise) {
		// With frication and damping.
		// Flow equations.
		const FloatType partialInflux = left.top[outPtr_] + right.bottom[outPtr_] + upper.bottom[outPtr_];
		left.bottom[inPtr_] = (right.bottom[outPtr_] + upper.bottom[outPtr_] + junction.leftCoeff  * partialInflux) * dampingFactor_;
		right.top[  inPtr_] = ( left.top[   outPtr_] + upper.bottom[outPtr_] + junction.rightCoeff * partialInflux) * dampingFactor_ + fricNoise;
		upper.top[  inPtr_] = ( left.top[   outPtr_] + right.bottom[outPtr_] + junction.upperCoeff * partialInflux) * dampingFactor_;
	}

	VocalTractModel5(const VocalTractModel5&) = delete;
	VocalTractModel5& operator=(const VocalTractModel5&) = delete;

	void loadConfiguration(const ConfigurationData& data);
	void reset();
	void initializeSynthesizer();
	void calculateTubeCoefficients();
	void initializeNasalCavity();
	void parseInputStream(std::istream& in);
	void setFricationTaps();
	FloatType vocalTract(FloatType input, FloatType frication);
	void writeOutputToFile(const char* outputFile);
	void writeOutputToBuffer(std::vector<float>& outputBuffer);
	void synthesize();
	float calculateOutputScale();
	void synthesizeForInputSequence();

	bool interactive_;
	Configuration config_;

	/*  DERIVED VALUES  */
	int controlPeriod_;
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
	std::array<FloatType, TOTAL_FRIC_COEFFICIENTS> fricationTap_;

	FloatType dampingFactor_;               /*  calculated damping factor  */
	FloatType crossmixFactor_;              /*  calculated crossmix factor  */
	FloatType breathinessFactor_;

	FloatType prevGlotAmplitude_;

	std::vector<std::array<FloatType, TOTAL_PARAMETERS>> inputData_;
	std::array<FloatType, TOTAL_PARAMETERS> currentParameter_;
	std::array<FloatType, TOTAL_PARAMETERS> currentParameterDelta_;
	std::array<FloatType, TOTAL_PARAMETERS> singleInput_;
	std::size_t outputDataPos_;
	std::vector<float> outputData_;
	std::unique_ptr<SampleRateConverter<FloatType>>    srConv_;
	std::unique_ptr<RadiationFilter<FloatType>>        mouthRadiationFilter_;
	std::unique_ptr<ReflectionFilter<FloatType>>       mouthReflectionFilter_;
	std::unique_ptr<RadiationFilter<FloatType>>        nasalRadiationFilter_;
	std::unique_ptr<ReflectionFilter<FloatType>>       nasalReflectionFilter_;
	std::unique_ptr<Throat<FloatType>>                 throat_;
	std::unique_ptr<WavetableGlottalSource<FloatType>> glottalSource_;
	std::unique_ptr<BandpassFilter<FloatType>>         bandpassFilter_;
	std::unique_ptr<NoiseFilter<FloatType>>            noiseFilter_;
	std::unique_ptr<NoiseSource>                       noiseSource_;
	std::unique_ptr<InputFilters>                      inputFilters_;
};



template<typename FloatType, unsigned int SectionDelay>
VocalTractModel5<FloatType, SectionDelay>::VocalTractModel5(const ConfigurationData& data, bool interactive)
		: interactive_ {interactive}
{
	loadConfiguration(data);
	reset();
	initializeSynthesizer();
	inputData_.reserve(128);
	outputData_.reserve(1024);
}

template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::loadConfiguration(const ConfigurationData& data)
{
	config_.outputRate     = data.value<FloatType>("output_rate");
	config_.controlRate    = data.value<FloatType>("control_rate");
	config_.waveform       = data.value<int>("waveform");
	config_.tp             = data.value<FloatType>("glottal_pulse_tp");
	config_.tnMin          = data.value<FloatType>("glottal_pulse_tn_min");
	config_.tnMax          = data.value<FloatType>("glottal_pulse_tn_max");
	config_.breathiness    = data.value<FloatType>("breathiness");
	config_.length         = data.value<FloatType>("vocal_tract_length_offset") + data.value<FloatType>("vocal_tract_length");
	config_.temperature    = data.value<FloatType>("temperature");
	config_.lossFactor     = data.value<FloatType>("loss_factor");
	config_.mouthCoef      = data.value<FloatType>("mouth_coefficient");
	config_.noseCoef       = data.value<FloatType>("nose_coefficient");
	config_.throatCutoff   = data.value<FloatType>("throat_cutoff");
	config_.throatVol      = data.value<FloatType>("throat_volume");
	config_.modulation     = data.value<int>("noise_modulation");
	config_.mixOffset      = data.value<FloatType>("mix_offset");
	const FloatType globalRadiusCoef      = data.value<FloatType>("global_radius_coef");
	const FloatType globalNasalRadiusCoef = data.value<FloatType>("global_nasal_radius_coef");
	config_.apertureRadius = data.value<FloatType>("aperture_radius") * globalRadiusCoef;
	config_.nasalRadius[0] = 0.0;
	config_.nasalRadius[1] = data.value<FloatType>("nasal_radius_1") * globalNasalRadiusCoef;
	config_.nasalRadius[2] = data.value<FloatType>("nasal_radius_2") * globalNasalRadiusCoef;
	config_.nasalRadius[3] = data.value<FloatType>("nasal_radius_3") * globalNasalRadiusCoef;
	config_.nasalRadius[4] = data.value<FloatType>("nasal_radius_4") * globalNasalRadiusCoef;
	config_.nasalRadius[5] = data.value<FloatType>("nasal_radius_5") * globalNasalRadiusCoef;
	config_.radiusCoef[0]  = data.value<FloatType>("radius_1_coef") * globalRadiusCoef;
	config_.radiusCoef[1]  = data.value<FloatType>("radius_2_coef") * globalRadiusCoef;
	config_.radiusCoef[2]  = data.value<FloatType>("radius_3_coef") * globalRadiusCoef;
	config_.radiusCoef[3]  = data.value<FloatType>("radius_4_coef") * globalRadiusCoef;
	config_.radiusCoef[4]  = data.value<FloatType>("radius_5_coef") * globalRadiusCoef;
	config_.radiusCoef[5]  = data.value<FloatType>("radius_6_coef") * globalRadiusCoef;
	config_.radiusCoef[6]  = data.value<FloatType>("radius_7_coef") * globalRadiusCoef;
	config_.radiusCoef[7]  = data.value<FloatType>("radius_8_coef") * globalRadiusCoef;
}

template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::reset()
{
	for (auto& elem : oropharynx_) {
		elem.reset();
	}
	for (auto& elem : nasal_) {
		elem.reset();
	}
	inPtr_  = 0;
	outPtr_ = 1;
	prevGlotAmplitude_ = -1.0;
	inputData_.clear();
	singleInput_.fill(0.0);
	outputData_.clear();
	outputDataPos_ = 0;
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
	if (inputFilters_)          inputFilters_->reset();
}

template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::synthesizeToFile(std::istream& inputStream, const char* outputFile)
{
	if (!outputData_.empty()) {
		reset();
	}
	parseInputStream(inputStream);
	synthesizeForInputSequence();
	writeOutputToFile(outputFile);
}

template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::synthesizeToBuffer(std::istream& inputStream, std::vector<float>& outputBuffer)
{
	if (!outputData_.empty()) {
		reset();
	}
	parseInputStream(inputStream);
	synthesizeForInputSequence();
	writeOutputToBuffer(outputBuffer);
}

template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::parseInputStream(std::istream& in)
{
	std::string line;
	std::array<FloatType, TOTAL_PARAMETERS> value;

	unsigned int lineNumber = 1;
	while (std::getline(in, line)) {
		std::istringstream lineStream(line);

		for (int i = 0; i < TOTAL_PARAMETERS; ++i) {
			lineStream >> value[i];
		}
		if (!lineStream) {
			THROW_EXCEPTION(VTMException, "[VTM::VocalTractModel5] Error in input parameter parsing: Could not read parameters (line number " << lineNumber << ").");
		}

		// R1 - R8.
		for (int i = 0; i < TOTAL_REGIONS; ++i) {
			value[PARAM_R1 + i] = std::max(value[PARAM_R1 + i] * config_.radiusCoef[i], FloatType{GS_VTM_VOCAL_TRACT_MODEL_5_MIN_RADIUS});
		}

		inputData_.push_back(value);
		++lineNumber;
	}

	/*  DOUBLE UP THE LAST INPUT TABLE, TO HELP INTERPOLATION CALCULATIONS  */
	if (!inputData_.empty()) {
		inputData_.push_back(inputData_.back());
	}
}

/******************************************************************************
*
*  function:  initializeSynthesizer
*
*  purpose:   Initializes all variables so that the synthesis can
*             be run.
*
******************************************************************************/
template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::initializeSynthesizer()
{
	FloatType nyquist;

	/*  CALCULATE THE SAMPLE RATE, BASED ON NOMINAL TUBE LENGTH AND SPEED OF SOUND  */
	if (config_.length > 0.0) {
		const FloatType c = Util::speedOfSound(config_.temperature);
		controlPeriod_ = static_cast<int>(std::rint((c * (TOTAL_SECTIONS * SectionDelay) * 100.0f) / (config_.length * config_.controlRate)));
		sampleRate_ = static_cast<int>(config_.controlRate * controlPeriod_);
		nyquist = sampleRate_ / 2.0f;
		LOG_DEBUG("[VocalTractModel5] Internal sample rate: " << sampleRate_);
	} else {
		THROW_EXCEPTION(VTMException, "Illegal tube length.\n");
	}

	/*  CALCULATE THE BREATHINESS FACTOR  */
	breathinessFactor_ = config_.breathiness / 100.0f;

	/*  CALCULATE CROSSMIX FACTOR  */
	crossmixFactor_ = 1.0f / Util::amplitude60dB(config_.mixOffset);

	/*  CALCULATE THE DAMPING FACTOR  */
	dampingFactor_ = 1.0f - (config_.lossFactor / 100.0f);

	/*  INITIALIZE THE WAVE TABLE  */
	glottalSource_ = std::make_unique<WavetableGlottalSource<FloatType>>(
						config_.waveform == GLOTTAL_SOURCE_PULSE ?
							WavetableGlottalSource<FloatType>::TYPE_PULSE :
							WavetableGlottalSource<FloatType>::TYPE_SINE,
						sampleRate_,
						config_.tp, config_.tnMin, config_.tnMax);

	/*  INITIALIZE REFLECTION AND RADIATION FILTER COEFFICIENTS FOR MOUTH  */
	FloatType mouthApertureCoeff = (nyquist - config_.mouthCoef) / nyquist;
	mouthRadiationFilter_  = std::make_unique<RadiationFilter<FloatType>>(mouthApertureCoeff);
	mouthReflectionFilter_ = std::make_unique<ReflectionFilter<FloatType>>(mouthApertureCoeff);

	/*  INITIALIZE REFLECTION AND RADIATION FILTER COEFFICIENTS FOR NOSE  */
	FloatType nasalApertureCoeff = (nyquist - config_.noseCoef) / nyquist;
	nasalRadiationFilter_  = std::make_unique<RadiationFilter<FloatType>>(nasalApertureCoeff);
	nasalReflectionFilter_ = std::make_unique<ReflectionFilter<FloatType>>(nasalApertureCoeff);

	/*  INITIALIZE NASAL CAVITY FIXED SCATTERING COEFFICIENTS  */
	initializeNasalCavity();

	/*  INITIALIZE THE THROAT LOWPASS FILTER  */
	throat_ = std::make_unique<Throat<FloatType>>(sampleRate_, config_.throatCutoff, Util::amplitude60dB(config_.throatVol));

	/*  INITIALIZE THE SAMPLE RATE CONVERSION ROUTINES  */
	srConv_ = std::make_unique<SampleRateConverter<FloatType>>(sampleRate_, config_.outputRate, outputData_);

	/*  INITIALIZE THE OUTPUT VECTOR  */
	outputData_.clear();

	bandpassFilter_ = std::make_unique<BandpassFilter<FloatType>>();
	noiseFilter_    = std::make_unique<NoiseFilter<FloatType>>();
	noiseSource_    = std::make_unique<NoiseSource>();

	if (interactive_) {
		inputFilters_ = std::make_unique<InputFilters>(sampleRate_, GS_VTM_VOCAL_TRACT_MODEL_5_INPUT_FILTER_PERIOD_SEC);
	}
}

/******************************************************************************
*
*  function:  synthesize
*
*  purpose:   Performs the actual synthesis of sound samples.
*
******************************************************************************/
template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::synthesizeForInputSequence()
{
	const FloatType controlFreq = 1.0 / controlPeriod_;

	/*  CONTROL RATE LOOP  */
	for (int i = 1, size = inputData_.size(); i < size; ++i) {
		// Calculates the current parameter values, and their
		// associated sample-to-sample delta values.
		for (int j = 0; j < TOTAL_PARAMETERS; ++j) {
			currentParameter_[j] = inputData_[i - 1][j];
			currentParameterDelta_[j] = (inputData_[i][j] - currentParameter_[j]) * controlFreq;
		}

		/*  SAMPLE RATE LOOP  */
		for (int j = 0; j < controlPeriod_; ++j) {
			synthesize();

			/*  DO SAMPLE RATE INTERPOLATION OF CONTROL PARAMETERS  */
			for (int k = 0; k < TOTAL_PARAMETERS; ++k) {
				currentParameter_[k] += currentParameterDelta_[k];
			}
		}
	}
}

template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::synthesizeSamples(std::size_t numberOfSamples)
{
	if (!inputFilters_) {
		THROW_EXCEPTION(InvalidStateException, "Input filters have not been initialized.");
	}

	while (outputData_.size() < numberOfSamples) {
		for (int i = 0; i < TOTAL_PARAMETERS; ++i) {
			currentParameter_[i] = inputFilters_->filter[i].filter(singleInput_[i]);
		}

		synthesize();
	}

	// Normalize.
	const float scale = calculateOutputScale();
	for (auto& sample : outputData_) {
		sample *= scale;
	}
}

template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::synthesize()
{
	/*  CONVERT PARAMETERS HERE  */
	FloatType f0 = Util::frequency(currentParameter_[PARAM_GLOT_PITCH]);
	FloatType ax = Util::amplitude60dB(currentParameter_[PARAM_GLOT_VOL]);
	FloatType ah1 = Util::amplitude60dB(currentParameter_[PARAM_ASP_VOL]);
	calculateTubeCoefficients();
	setFricationTaps();
	bandpassFilter_->update(sampleRate_, currentParameter_[PARAM_FRIC_BW], currentParameter_[PARAM_FRIC_CF]);

	/*  DO SYNTHESIS HERE  */
	/*  CREATE LOW-PASS FILTERED NOISE  */
	FloatType lpNoise = noiseFilter_->filter(noiseSource_->getSample());

	/*  UPDATE THE SHAPE OF THE GLOTTAL PULSE, IF NECESSARY  */
	if (config_.waveform == GLOTTAL_SOURCE_PULSE) {
		if (ax != prevGlotAmplitude_) {
			glottalSource_->updateWavetable(ax);
		}
	}

	/*  CREATE GLOTTAL PULSE (OR SINE TONE)  */
	FloatType pulse = glottalSource_->getSample(f0);

	/*  CREATE PULSED NOISE  */
	FloatType pulsedNoise = lpNoise * pulse;

	/*  CREATE NOISY GLOTTAL PULSE  */
	pulse = ax * ((pulse * (1.0f - breathinessFactor_)) +
			(pulsedNoise * breathinessFactor_));

	FloatType signal;
	/*  CROSS-MIX PURE NOISE WITH PULSED NOISE  */
	if (config_.modulation) {
		FloatType crossmix = ax * crossmixFactor_;
		crossmix = (crossmix < 1.0f) ? crossmix : 1.0f;
		signal = (pulsedNoise * crossmix) +
				(lpNoise * (1.0f - crossmix));
	} else {
		signal = lpNoise;
	}

	/*  PUT SIGNAL THROUGH VOCAL TRACT  */
	signal = vocalTract(((pulse + (ah1 * signal)) * FloatType{GS_VTM_VOCAL_TRACT_MODEL_5_VT_SCALE}),
				bandpassFilter_->filter(signal));

	/*  PUT PULSE THROUGH THROAT  */
	signal += throat_->process(pulse * FloatType{GS_VTM_VOCAL_TRACT_MODEL_5_VT_SCALE});

	/*  OUTPUT SAMPLE HERE  */
	srConv_->dataFill(signal);

	prevGlotAmplitude_ = ax;
}

/******************************************************************************
*
*  function:  initializeNasalCavity
*
*  purpose:   Calculates the scattering coefficients for the fixed
*             sections of the nasal cavity.
*
******************************************************************************/
template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::initializeNasalCavity()
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
template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::calculateTubeCoefficients()
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
template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::setFricationTaps()
{
	const FloatType fricationAmplitude = Util::amplitude60dB(currentParameter_[PARAM_FRIC_VOL]);

	/*  CALCULATE POSITION REMAINDER AND COMPLEMENT  */
	const int integerPart = static_cast<int>(currentParameter_[PARAM_FRIC_POS]);
	const FloatType complement = currentParameter_[PARAM_FRIC_POS] - integerPart;
	const FloatType remainder = 1.0f - complement;

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
template<typename FloatType, unsigned int SectionDelay>
FloatType
VocalTractModel5<FloatType, SectionDelay>::vocalTract(FloatType input, FloatType frication)
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
	FloatType output = mouthRadiationFilter_->filter((1.0f - oropharynxJunction_[J8].coeff) * oropharynx_[S30].top[outPtr_]);

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
	output += nasalRadiationFilter_->filter((1.0f - nasalJunction_[NJ6].coeff) * nasal_[N18].top[outPtr_]);

	// Return summed output from mouth and nose.
	return output;
}

/******************************************************************************
*
*  function:  writeOutputToFile
*
*  purpose:   Scales the samples stored in the temporary file, and
*             writes them to the output file, with the appropriate
*             header. Also does master volume scaling.
*
******************************************************************************/
template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::writeOutputToFile(const char* outputFile)
{
	/*  BE SURE TO FLUSH SRC BUFFER  */
	srConv_->flushBuffer();

	LOG_DEBUG("\nNumber of samples: " << srConv_->numberSamples() <<
			"\nMaximum sample value: " << srConv_->maximumSampleValue());

	WAVEFileWriter fileWriter(outputFile, 1, srConv_->numberSamples(), config_.outputRate);

	const float scale = calculateOutputScale();
	for (unsigned int i = 0, end = srConv_->numberSamples(); i < end; ++i) {
		fileWriter.writeSample(outputData_[i] * scale);
	}
}

template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::writeOutputToBuffer(std::vector<float>& outputBuffer)
{
	/*  BE SURE TO FLUSH SRC BUFFER  */
	srConv_->flushBuffer();

	LOG_DEBUG("\nNumber of samples: " << srConv_->numberSamples() <<
			"\nMaximum sample value: " << srConv_->maximumSampleValue());

	outputBuffer.resize(srConv_->numberSamples());

	const float scale = calculateOutputScale();
	for (unsigned int i = 0, end = srConv_->numberSamples(); i < end; ++i) {
		outputBuffer[i] = outputData_[i] * scale;
	}
}

template<typename FloatType, unsigned int SectionDelay>
float
VocalTractModel5<FloatType, SectionDelay>::calculateOutputScale()
{
	const float maxValue = srConv_->maximumSampleValue();
	if (maxValue < 1.0e-10f) {
		return 0.0;
	}

	const float scale = GS_VTM_VOCAL_TRACT_MODEL_5_OUTPUT_SCALE / maxValue;
	LOG_DEBUG("\nScale: " << scale << '\n');
	return scale;
}

template<typename FloatType, unsigned int SectionDelay>
void
VocalTractModel5<FloatType, SectionDelay>::loadSingleInput(const VocalTractModelParameterValue pv)
{
	switch (pv.index) {
	case PARAM_GLOT_PITCH:
	case PARAM_GLOT_VOL:
	case PARAM_ASP_VOL:
	case PARAM_FRIC_VOL:
	case PARAM_FRIC_POS:
	case PARAM_FRIC_CF:
	case PARAM_FRIC_BW:
	case PARAM_VELUM:
		singleInput_[pv.index] = pv.value;
		break;
	case PARAM_R1:
	case PARAM_R2:
	case PARAM_R3:
	case PARAM_R4:
	case PARAM_R5:
	case PARAM_R6:
	case PARAM_R7:
	case PARAM_R8:
		singleInput_[pv.index] = std::max(
					pv.value * config_.radiusCoef[pv.index - PARAM_R1],
					FloatType{GS_VTM_VOCAL_TRACT_MODEL_5_MIN_RADIUS});
		break;
	default:
		THROW_EXCEPTION(VTMException, "Invalid parameter index: " << pv.index << '.');
	}
}

template<typename FloatType, unsigned int SectionDelay>
std::size_t
VocalTractModel5<FloatType, SectionDelay>::getOutputSamples(std::size_t n, float* buffer)
{
	if (outputData_.empty()) return 0;

	const std::size_t size = outputData_.size();
	const std::size_t initialPos = outputDataPos_;
	for (std::size_t j = 0; j < n && outputDataPos_ < size; ++j, ++outputDataPos_) {
		buffer[j] = outputData_[outputDataPos_];
	}

	const std::size_t samplesRead = outputDataPos_ - initialPos;
	if (outputDataPos_ == size) { // all samples were used
		outputData_.clear();
		outputDataPos_ = 0;
	}
	return samplesRead;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_VOCAL_TRACT_MODEL_5_H_ */