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

#ifndef VTM_VOCAL_TRACT_MODEL0_H_
#define VTM_VOCAL_TRACT_MODEL0_H_

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
#include "VocalTractModelParameterValue.h"
#include "VTMUtil.h"
#include "WAVEFileWriter.h"
#include "WavetableGlottalSource.h"

#define GS_VTM_VOCAL_TRACT_MODEL_0_MIN_RADIUS (0.001)
#define GS_VTM_VOCAL_TRACT_MODEL_0_INPUT_FILTER_PERIOD_SEC (50.0e-3)

/*  SCALING CONSTANT FOR INPUT TO VOCAL TRACT & THROAT (MATCHES DSP)  */
//#define GS_VTM_VOCAL_TRACT_MODEL_0_VT_SCALE                  0.03125     /*  2^(-5)  */
// this is a temporary fix only, to try to match dsp synthesizer
#define GS_VTM_VOCAL_TRACT_MODEL_0_VT_SCALE                  0.125     /*  2^(-3)  */

/*  FINAL OUTPUT SCALING, SO THAT .SND FILES APPROX. MATCH DSP OUTPUT  */
#define GS_VTM_VOCAL_TRACT_MODEL_0_OUTPUT_SCALE              0.95



namespace GS {
namespace VTM {

template<typename FloatType>
class VocalTractModel0 {
public:
	enum { /*  OROPHARYNX REGIONS  */
		R1 = 0, /*  S1  */
		R2 = 1, /*  S2  */
		R3 = 2, /*  S3  */
		R4 = 3, /*  S4 & S5  */
		R5 = 4, /*  S6 & S7  */
		R6 = 5, /*  S8  */
		R7 = 6, /*  S9  */
		R8 = 7, /*  S10  */
		TOTAL_REGIONS = 8
	};
	enum { /*  NASAL TRACT SECTIONS  */
		N1 = 0,
		N2 = 1,
		N3 = 2,
		N4 = 3,
		N5 = 4,
		N6 = 5,
		TOTAL_NASAL_SECTIONS = 6
	};
	enum Waveform {
		GLOTTAL_SOURCE_PULSE = 0,
		GLOTTAL_SOURCE_SINE = 1
	};

	VocalTractModel0(const ConfigurationData& data, bool interactive = false);
	~VocalTractModel0() {}

	double outputRate() const { return config_.outputRate; }

	// ----- Batch mode.
	void synthesizeToFile(std::istream& inputStream, const char* outputFile);
	void synthesizeToBuffer(std::istream& inputStream, std::vector<float>& outputBuffer);

	// ----- Interactive mode.
	void loadSingleInput(const VocalTractModelParameterValue pv);
	void synthesizeSamples(int numberOfSamples);
	// Returns the number of samples read.
	std::size_t getOutputSamples(std::size_t n, float* buffer);

private:
	enum {
		VELUM = N1
	};
	enum { /*  OROPHARYNX SCATTERING JUNCTION COEFFICIENTS (BETWEEN EACH REGION)  */
		C1 = R1, /*  R1-R2 (S1-S2)  */
		C2 = R2, /*  R2-R3 (S2-S3)  */
		C3 = R3, /*  R3-R4 (S3-S4)  */
		C4 = R4, /*  R4-R5 (S5-S6)  */
		C5 = R5, /*  R5-R6 (S7-S8)  */
		C6 = R6, /*  R6-R7 (S8-S9)  */
		C7 = R7, /*  R7-R8 (S9-S10)  */
		C8 = R8, /*  R8-AIR (S10-AIR)  */
		TOTAL_COEFFICIENTS = TOTAL_REGIONS
	};
	enum { /*  OROPHARYNX SECTIONS  */
		S1  = 0, /*  R1  */
		S2  = 1, /*  R2  */
		S3  = 2, /*  R3  */
		S4  = 3, /*  R4  */
		S5  = 4, /*  R4  */
		S6  = 5, /*  R5  */
		S7  = 6, /*  R5  */
		S8  = 7, /*  R6  */
		S9  = 8, /*  R7  */
		S10 = 9, /*  R8  */
		TOTAL_SECTIONS = 10
	};
	enum { /*  NASAL TRACT COEFFICIENTS  */
		NC1 = N1, /*  N1-N2  */
		NC2 = N2, /*  N2-N3  */
		NC3 = N3, /*  N3-N4  */
		NC4 = N4, /*  N4-N5  */
		NC5 = N5, /*  N5-N6  */
		NC6 = N6, /*  N6-AIR  */
		TOTAL_NASAL_COEFFICIENTS = TOTAL_NASAL_SECTIONS
	};
	enum { /*  THREE-WAY JUNCTION ALPHA COEFFICIENTS  */
		LEFT  = 0,
		RIGHT = 1,
		UPPER = 2,
		TOTAL_ALPHA_COEFFICIENTS = 3
	};
	enum { /*  FRICATION INJECTION COEFFICIENTS  */
		FC1 = 0, /*  S3  */
		FC2 = 1, /*  S4  */
		FC3 = 2, /*  S5  */
		FC4 = 3, /*  S6  */
		FC5 = 4, /*  S7  */
		FC6 = 5, /*  S8  */
		FC7 = 6, /*  S9  */
		FC8 = 7, /*  S10  */
		TOTAL_FRIC_COEFFICIENTS = 8
	};
	enum { /*  BI-DIRECTIONAL TRANSMISSION LINE POINTERS  */
		TOP    = 0,
		BOTTOM = 1
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
		// Set noseRadius[N1] to 0.0, because it is not used.
		std::array<FloatType, TOTAL_NASAL_SECTIONS> noseRadius; /*  fixed nose radii (0 - 3 cm)  */
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

	VocalTractModel0(const VocalTractModel0&) = delete;
	VocalTractModel0& operator=(const VocalTractModel0&) = delete;

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
	FloatType actualTubeLength_;            /*  actual length in cm  */

	/*  MEMORY FOR TUBE AND TUBE COEFFICIENTS  */
	FloatType oropharynx_[TOTAL_SECTIONS][2][2];
	FloatType oropharynxCoeff_[TOTAL_COEFFICIENTS];

	FloatType nasal_[TOTAL_NASAL_SECTIONS][2][2];
	FloatType nasalCoeff_[TOTAL_NASAL_COEFFICIENTS];

	FloatType alpha_[TOTAL_ALPHA_COEFFICIENTS];
	int currentPtr_;
	int prevPtr_;

	/*  MEMORY FOR FRICATION TAPS  */
	FloatType fricationTap_[TOTAL_FRIC_COEFFICIENTS];

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



template<typename FloatType>
VocalTractModel0<FloatType>::VocalTractModel0(const ConfigurationData& data, bool interactive)
		: interactive_ {interactive}
{
	loadConfiguration(data);
	reset();
	initializeSynthesizer();
	inputData_.reserve(128);
	outputData_.reserve(1024);
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::loadConfiguration(const ConfigurationData& data)
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
	const FloatType globalRadiusCoef     = data.value<FloatType>("global_radius_coef");
	const FloatType globalNoseRadiusCoef = data.value<FloatType>("global_nose_radius_coef");
	config_.apertureRadius = data.value<FloatType>("aperture_radius") * globalRadiusCoef;
	config_.noseRadius[0]  = 0.0;
	config_.noseRadius[1]  = data.value<FloatType>("nose_radius_1") * globalNoseRadiusCoef;
	config_.noseRadius[2]  = data.value<FloatType>("nose_radius_2") * globalNoseRadiusCoef;
	config_.noseRadius[3]  = data.value<FloatType>("nose_radius_3") * globalNoseRadiusCoef;
	config_.noseRadius[4]  = data.value<FloatType>("nose_radius_4") * globalNoseRadiusCoef;
	config_.noseRadius[5]  = data.value<FloatType>("nose_radius_5") * globalNoseRadiusCoef;
	config_.radiusCoef[0]  = data.value<FloatType>("radius_1_coef") * globalRadiusCoef;
	config_.radiusCoef[1]  = data.value<FloatType>("radius_2_coef") * globalRadiusCoef;
	config_.radiusCoef[2]  = data.value<FloatType>("radius_3_coef") * globalRadiusCoef;
	config_.radiusCoef[3]  = data.value<FloatType>("radius_4_coef") * globalRadiusCoef;
	config_.radiusCoef[4]  = data.value<FloatType>("radius_5_coef") * globalRadiusCoef;
	config_.radiusCoef[5]  = data.value<FloatType>("radius_6_coef") * globalRadiusCoef;
	config_.radiusCoef[6]  = data.value<FloatType>("radius_7_coef") * globalRadiusCoef;
	config_.radiusCoef[7]  = data.value<FloatType>("radius_8_coef") * globalRadiusCoef;
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::reset()
{
	controlPeriod_    = 0;
	sampleRate_       = 0;
	actualTubeLength_ = 0.0;
	memset(&oropharynx_[0][0][0], 0, sizeof(FloatType) * TOTAL_SECTIONS * 2 * 2);
	memset(oropharynxCoeff_,      0, sizeof(FloatType) * TOTAL_COEFFICIENTS);
	memset(&nasal_[0][0][0],      0, sizeof(FloatType) * TOTAL_NASAL_SECTIONS * 2 * 2);
	memset(nasalCoeff_,           0, sizeof(FloatType) * TOTAL_NASAL_COEFFICIENTS);
	memset(alpha_,                0, sizeof(FloatType) * TOTAL_ALPHA_COEFFICIENTS);
	currentPtr_ = 1;
	prevPtr_    = 0;
	memset(fricationTap_, 0, sizeof(FloatType) * TOTAL_FRIC_COEFFICIENTS);
	dampingFactor_     = 0.0;
	crossmixFactor_    = 0.0;
	breathinessFactor_ = 0.0;
	prevGlotAmplitude_ = -1.0;
	inputData_.clear();
	currentParameter_.fill(0.0);
	currentParameterDelta_.fill(0.0);
	singleInput_.fill(0.0);
	outputDataPos_ = 0;
	outputData_.clear();

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

template<typename FloatType>
void
VocalTractModel0<FloatType>::synthesizeToFile(std::istream& inputStream, const char* outputFile)
{
	if (!outputData_.empty()) {
		reset();
	}
	parseInputStream(inputStream);
	synthesizeForInputSequence();
	writeOutputToFile(outputFile);
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::synthesizeToBuffer(std::istream& inputStream, std::vector<float>& outputBuffer)
{
	if (!outputData_.empty()) {
		reset();
	}
	parseInputStream(inputStream);
	synthesizeForInputSequence();
	writeOutputToBuffer(outputBuffer);
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::parseInputStream(std::istream& in)
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
			THROW_EXCEPTION(VTMException, "[VTM::VocalTractModel0] Error in input parameter parsing: Could not read parameters (line number " << lineNumber << ").");
		}

		// R1 - R8.
		for (int i = 0; i < TOTAL_REGIONS; ++i) {
			value[PARAM_R1 + i] = std::max(value[PARAM_R1 + i] * config_.radiusCoef[i], GS_VTM_VOCAL_TRACT_MODEL_0_MIN_RADIUS);
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
template<typename FloatType>
void
VocalTractModel0<FloatType>::initializeSynthesizer()
{
	FloatType nyquist;

	/*  CALCULATE THE SAMPLE RATE, BASED ON NOMINAL TUBE LENGTH AND SPEED OF SOUND  */
	if (config_.length > 0.0) {
		const FloatType c = Util::speedOfSound(config_.temperature);
		controlPeriod_ = static_cast<int>(std::rint((c * TOTAL_SECTIONS * 100.0f) / (config_.length * config_.controlRate)));
		sampleRate_ = static_cast<int>(config_.controlRate * controlPeriod_);
		actualTubeLength_ = (c * TOTAL_SECTIONS * 100.0f) / sampleRate_;
		nyquist = sampleRate_ / 2.0f;
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
		inputFilters_ = std::make_unique<InputFilters>(sampleRate_, GS_VTM_VOCAL_TRACT_MODEL_0_INPUT_FILTER_PERIOD_SEC);
	}
}

/******************************************************************************
*
*  function:  synthesize
*
*  purpose:   Performs the actual synthesis of sound samples.
*
******************************************************************************/
template<typename FloatType>
void
VocalTractModel0<FloatType>::synthesizeForInputSequence()
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

template<typename FloatType>
void
VocalTractModel0<FloatType>::synthesizeSamples(int numberOfSamples)
{
	if (!inputFilters_) {
		THROW_EXCEPTION(InvalidStateException, "Input filters have not been initialized.");
	}

	while (outputData_.size() < numberOfSamples) {
		for (int i = 0; i < TOTAL_PARAMETERS; ++i) {
			currentParameter_[i] = inputFilters_.filter[i].filter(singleInput_[i]);
		}

		synthesize();
	}

	// Normalize.
	const float scale = calculateOutputScale();
	for (auto& sample : outputData_) {
		sample *= scale;
	}
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::synthesize()
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
	signal = vocalTract(((pulse + (ah1 * signal)) * FloatType{GS_VTM_VOCAL_TRACT_MODEL_0_VT_SCALE}),
				bandpassFilter_->filter(signal));

	/*  PUT PULSE THROUGH THROAT  */
	signal += throat_->process(pulse * FloatType{GS_VTM_VOCAL_TRACT_MODEL_0_VT_SCALE});

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
template<typename FloatType>
void
VocalTractModel0<FloatType>::initializeNasalCavity()
{
	/*  CALCULATE COEFFICIENTS FOR INTERNAL FIXED SECTIONS OF NASAL CAVITY  */
	for (int i = N2, j = NC2; i < N6; i++, j++) {
		const FloatType radA2 = config_.noseRadius[i]     * config_.noseRadius[i];
		const FloatType radB2 = config_.noseRadius[i + 1] * config_.noseRadius[i + 1];
		nasalCoeff_[j] = (radA2 - radB2) / (radA2 + radB2);
	}

	/*  CALCULATE THE FIXED COEFFICIENT FOR THE NOSE APERTURE  */
	const FloatType radA2 = config_.noseRadius[N6] * config_.noseRadius[N6];
	const FloatType radB2 = config_.apertureRadius * config_.apertureRadius;
	nasalCoeff_[NC6] = (radA2 - radB2) / (radA2 + radB2);
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
template<typename FloatType>
void
VocalTractModel0<FloatType>::calculateTubeCoefficients()
{
	/*  CALCULATE COEFFICIENTS FOR THE OROPHARYNX  */
	for (int i = 0; i < (TOTAL_REGIONS - 1); i++) {
		const FloatType radA2 = currentParameter_[PARAM_R1 + i]     * currentParameter_[PARAM_R1 + i];
		const FloatType radB2 = currentParameter_[PARAM_R1 + i + 1] * currentParameter_[PARAM_R1 + i + 1];
		oropharynxCoeff_[i] = (radA2 - radB2) / (radA2 + radB2);
	}

	/*  CALCULATE THE COEFFICIENT FOR THE MOUTH APERTURE  */
	FloatType radA2 = currentParameter_[PARAM_R8] * currentParameter_[PARAM_R8];
	FloatType radB2 = config_.apertureRadius * config_.apertureRadius;
	oropharynxCoeff_[C8] = (radA2 - radB2) / (radA2 + radB2);

	/*  CALCULATE ALPHA COEFFICIENTS FOR 3-WAY JUNCTION  */
	/*  NOTE:  SINCE JUNCTION IS IN MIDDLE OF REGION 4, r0_2 = r1_2  */
	const FloatType r1_2 = currentParameter_[PARAM_R4] * currentParameter_[PARAM_R4];
	const FloatType r0_2 = r1_2;
	const FloatType r2_2 = currentParameter_[PARAM_VELUM] * currentParameter_[PARAM_VELUM];
	const FloatType sum = 2.0f / (r0_2 + r1_2 + r2_2);
	alpha_[LEFT]  = sum * r0_2;
	alpha_[RIGHT] = sum * r1_2;
	alpha_[UPPER] = sum * r2_2;

	/*  AND 1ST NASAL PASSAGE COEFFICIENT  */
	radA2 = r2_2;
	radB2 = config_.noseRadius[N2] * config_.noseRadius[N2];
	nasalCoeff_[NC1] = (radA2 - radB2) / (radA2 + radB2);
}

/******************************************************************************
*
*  function:  setFricationTaps
*
*  purpose:   Sets the frication taps according to the current
*             position and amplitude of frication.
*
******************************************************************************/
template<typename FloatType>
void
VocalTractModel0<FloatType>::setFricationTaps()
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
template<typename FloatType>
FloatType VocalTractModel0<FloatType>::vocalTract(FloatType input, FloatType frication)
{
	FloatType delta;

	/*  INCREMENT CURRENT AND PREVIOUS POINTERS  */
	if (++currentPtr_ > 1) {
		currentPtr_ = 0;
	}
	if (++prevPtr_ > 1) {
		prevPtr_ = 0;
	}

	/*  UPDATE OROPHARYNX  */
	/*  INPUT TO TOP OF TUBE  */
	oropharynx_[S1][TOP][currentPtr_] =
			(oropharynx_[S1][BOTTOM][prevPtr_] * dampingFactor_) + input;

	/*  CALCULATE THE SCATTERING JUNCTIONS FOR S1-S2  */
	delta = oropharynxCoeff_[C1] *
			(oropharynx_[S1][TOP][prevPtr_] - oropharynx_[S2][BOTTOM][prevPtr_]);
	oropharynx_[S2][TOP][currentPtr_] =
			(oropharynx_[S1][TOP][prevPtr_] + delta) * dampingFactor_;
	oropharynx_[S1][BOTTOM][currentPtr_] =
			(oropharynx_[S2][BOTTOM][prevPtr_] + delta) * dampingFactor_;

	/*  CALCULATE THE SCATTERING JUNCTIONS FOR S2-S3 AND S3-S4  */
	for (int i = S2, j = C2, k = FC1; i < S4; i++, j++, k++) {
		delta = oropharynxCoeff_[j] *
				(oropharynx_[i][TOP][prevPtr_] - oropharynx_[i + 1][BOTTOM][prevPtr_]);
		oropharynx_[i + 1][TOP][currentPtr_] =
				((oropharynx_[i][TOP][prevPtr_] + delta) * dampingFactor_) +
				(fricationTap_[k] * frication);
		oropharynx_[i][BOTTOM][currentPtr_] =
				(oropharynx_[i + 1][BOTTOM][prevPtr_] + delta) * dampingFactor_;
	}

	/*  UPDATE 3-WAY JUNCTION BETWEEN THE MIDDLE OF R4 AND NASAL CAVITY  */
	const FloatType junctionPressure = (alpha_[LEFT] * oropharynx_[S4][TOP][prevPtr_])+
			(alpha_[RIGHT] * oropharynx_[S5][BOTTOM][prevPtr_]) +
			(alpha_[UPPER] * nasal_[VELUM][BOTTOM][prevPtr_]);
	oropharynx_[S4][BOTTOM][currentPtr_] =
			(junctionPressure - oropharynx_[S4][TOP][prevPtr_]) * dampingFactor_;
	oropharynx_[S5][TOP][currentPtr_] =
			((junctionPressure - oropharynx_[S5][BOTTOM][prevPtr_]) * dampingFactor_)
			+ (fricationTap_[FC3] * frication);
	nasal_[VELUM][TOP][currentPtr_] =
			(junctionPressure - nasal_[VELUM][BOTTOM][prevPtr_]) * dampingFactor_;

	/*  CALCULATE JUNCTION BETWEEN R4 AND R5 (S5-S6)  */
	delta = oropharynxCoeff_[C4] *
			(oropharynx_[S5][TOP][prevPtr_] - oropharynx_[S6][BOTTOM][prevPtr_]);
	oropharynx_[S6][TOP][currentPtr_] =
			((oropharynx_[S5][TOP][prevPtr_] + delta) * dampingFactor_) +
			(fricationTap_[FC4] * frication);
	oropharynx_[S5][BOTTOM][currentPtr_] =
			(oropharynx_[S6][BOTTOM][prevPtr_] + delta) * dampingFactor_;

	/*  CALCULATE JUNCTION INSIDE R5 (S6-S7) (PURE DELAY WITH DAMPING)  */
	oropharynx_[S7][TOP][currentPtr_] =
			(oropharynx_[S6][TOP][prevPtr_] * dampingFactor_) +
			(fricationTap_[FC5] * frication);
	oropharynx_[S6][BOTTOM][currentPtr_] =
			oropharynx_[S7][BOTTOM][prevPtr_] * dampingFactor_;

	/*  CALCULATE LAST 3 INTERNAL JUNCTIONS (S7-S8, S8-S9, S9-S10)  */
	for (int i = S7, j = C5, k = FC6; i < S10; i++, j++, k++) {
		delta = oropharynxCoeff_[j] *
				(oropharynx_[i][TOP][prevPtr_] - oropharynx_[i + 1][BOTTOM][prevPtr_]);
		oropharynx_[i + 1][TOP][currentPtr_] =
				((oropharynx_[i][TOP][prevPtr_] + delta) * dampingFactor_) +
				(fricationTap_[k] * frication);
		oropharynx_[i][BOTTOM][currentPtr_] =
				(oropharynx_[i + 1][BOTTOM][prevPtr_] + delta) * dampingFactor_;
	}

	/*  REFLECTED SIGNAL AT MOUTH GOES THROUGH A LOWPASS FILTER  */
	oropharynx_[S10][BOTTOM][currentPtr_] =  dampingFactor_ *
			mouthReflectionFilter_->filter(oropharynxCoeff_[C8] *
							oropharynx_[S10][TOP][prevPtr_]);

	/*  OUTPUT FROM MOUTH GOES THROUGH A HIGHPASS FILTER  */
	FloatType output = mouthRadiationFilter_->filter((1.0f + oropharynxCoeff_[C8]) *
						oropharynx_[S10][TOP][prevPtr_]);

	/*  UPDATE NASAL CAVITY  */
	for (int i = VELUM, j = NC1; i < N6; i++, j++) {
		delta = nasalCoeff_[j] *
				(nasal_[i][TOP][prevPtr_] - nasal_[i + 1][BOTTOM][prevPtr_]);
		nasal_[i+1][TOP][currentPtr_] =
				(nasal_[i][TOP][prevPtr_] + delta) * dampingFactor_;
		nasal_[i][BOTTOM][currentPtr_] =
				(nasal_[i + 1][BOTTOM][prevPtr_] + delta) * dampingFactor_;
	}

	/*  REFLECTED SIGNAL AT NOSE GOES THROUGH A LOWPASS FILTER  */
	nasal_[N6][BOTTOM][currentPtr_] = dampingFactor_ *
			nasalReflectionFilter_->filter(nasalCoeff_[NC6] * nasal_[N6][TOP][prevPtr_]);

	/*  OUTPUT FROM NOSE GOES THROUGH A HIGHPASS FILTER  */
	output += nasalRadiationFilter_->filter((1.0f + nasalCoeff_[NC6]) *
						nasal_[N6][TOP][prevPtr_]);
	/*  RETURN SUMMED OUTPUT FROM MOUTH AND NOSE  */
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
template<typename FloatType>
void
VocalTractModel0<FloatType>::writeOutputToFile(const char* outputFile)
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

template<typename FloatType>
void
VocalTractModel0<FloatType>::writeOutputToBuffer(std::vector<float>& outputBuffer)
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

template<typename FloatType>
float
VocalTractModel0<FloatType>::calculateOutputScale()
{
	const float scale = GS_VTM_VOCAL_TRACT_MODEL_0_OUTPUT_SCALE / srConv_->maximumSampleValue();
	LOG_DEBUG("\nScale: " << scale << '\n');
	return scale;
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::loadSingleInput(const VocalTractModelParameterValue pv)
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
					FloatType{GS_VTM_VOCAL_TRACT_MODEL_0_MIN_RADIUS});
		break;
	default:
		THROW_EXCEPTION(VTMException, "Invalid parameter index: " << pv.index << '.');
	}
}

template<typename FloatType>
std::size_t
VocalTractModel0<FloatType>::getOutputSamples(std::size_t n, float* buffer)
{
	if (outputData_.empty()) return 0;

	const std::size_t size = outputData_.size();
	const std::size_t initialPos = outputDataPos_;
	for (std::size_t j = 0; j < n, outputDataPos_ < size; ++j, ++outputDataPos_) {
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

#endif /* VTM_VOCAL_TRACT_MODEL0_H_ */