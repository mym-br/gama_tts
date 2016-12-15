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

#ifndef VTM_VOCALTRACTMODEL0_H_
#define VTM_VOCALTRACTMODEL0_H_

#include <algorithm> /* max, min */
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

#define GS_VTM_TUBE_MIN_RADIUS (0.001)

#define INPUT_VECTOR_RESERVE 128
#define OUTPUT_VECTOR_RESERVE 1024

/*  SCALING CONSTANT FOR INPUT TO VOCAL TRACT & THROAT (MATCHES DSP)  */
//#define VT_SCALE                  0.03125     /*  2^(-5)  */
// this is a temporary fix only, to try to match dsp synthesizer
#define VT_SCALE                  0.125     /*  2^(-3)  */

/*  FINAL OUTPUT SCALING, SO THAT .SND FILES APPROX. MATCH DSP OUTPUT  */
#define OUTPUT_SCALE              0.95

/*  BI-DIRECTIONAL TRANSMISSION LINE POINTERS  */
#define TOP                       0
#define BOTTOM                    1



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
		PARAM_VELUM      = 15
	};
	enum Waveform {
		GLOTTAL_SOURCE_PULSE = 0,
		GLOTTAL_SOURCE_SINE = 1
	};

	struct Configuration {
		float  outputRate;                  /*  output sample rate (22.05, 44.1)  */
		float  controlRate;                 /*  1.0-1000.0 input tables/second (Hz)  */

		double volume;                      /*  master volume (0 - 60 dB)  */

		int    waveform;                    /*  GS waveform type (0=PULSE, 1=SINE  */
		double tp;                          /*  % glottal pulse rise time  */
		double tnMin;                       /*  % glottal pulse fall time minimum  */
		double tnMax;                       /*  % glottal pulse fall time maximum  */
		double breathiness;                 /*  % glottal source breathiness  */

		double length;                      /*  nominal tube length (10 - 20 cm)  */
		double temperature;                 /*  tube temperature (25 - 40 C)  */
		double lossFactor;                  /*  junction loss factor in (0 - 5 %)  */

		double apertureRadius;              /*  aperture scl. radius (3.05 - 12 cm)  */
		double mouthCoef;                   /*  mouth aperture coefficient  */
		double noseCoef;                    /*  nose aperture coefficient  */

		// Set noseRadius[N1] to 0.0, because it is not used.
		double noseRadius[TOTAL_NASAL_SECTIONS]; /*  fixed nose radii (0 - 3 cm)  */

		double throatCutoff;                /*  throat lp cutoff (50 - nyquist Hz)  */
		double throatVol;                   /*  throat volume (0 - 48 dB) */

		int    modulation;                  /*  pulse mod. of noise (0=OFF, 1=ON)  */
		double mixOffset;                   /*  noise crossmix offset (30 - 60 dB)  */
	};

	VocalTractModel0();
	~VocalTractModel0() {}

	void synthesizeToFile(std::istream& inputStream, const char* outputFile);
	void synthesizeToBuffer(std::istream& inputStream, std::vector<float>& outputBuffer);

	void loadConfiguration(const Configuration& config) {
		config_ = config;
	}

	void initializeSynthesizer();
	void initializeInputFilters(double period);
	void loadSingleInput(const VocalTractModelParameterValue pv);
	void synthesizeForInputSequence();
	void synthesizeForSingleInput(int numIterations);

	std::vector<float>& outputData() { return outputData_; }
	std::size_t outputDataPos() const { return outputDataPos_; }
	void setOutputDataPos(std::size_t pos) { outputDataPos_ = pos; }
	void resetOutputData() {
		outputData_.clear();
		outputDataPos_ = 0;
	}
	double maximumOutputSampleValue() const { return srConv_->maximumSampleValue(); }
	float outputRate() const { return config_.outputRate; }
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

	struct InputData {
		double glotPitch;
		double glotVol;
		double aspVol;
		double fricVol;
		double fricPos;
		double fricCF;
		double fricBW;
		double radius[TOTAL_REGIONS];
		double velum;
	};

	/*  VARIABLES FOR INTERPOLATION  */
	struct CurrentData {
		double glotPitch;
		double glotPitchDelta;
		double glotVol;
		double glotVolDelta;
		double aspVol;
		double aspVolDelta;
		double fricVol;
		double fricVolDelta;
		double fricPos;
		double fricPosDelta;
		double fricCF;
		double fricCFDelta;
		double fricBW;
		double fricBWDelta;
		double radius[TOTAL_REGIONS];
		double radiusDelta[TOTAL_REGIONS];
		double velum;
		double velumDelta;
	};

	struct InputFilters {
		MovingAverageFilter<FloatType> glotPitchFilter;
		MovingAverageFilter<FloatType> glotVolFilter;
		MovingAverageFilter<FloatType> aspVolFilter;
		MovingAverageFilter<FloatType> fricVolFilter;
		MovingAverageFilter<FloatType> fricPosFilter;
		MovingAverageFilter<FloatType> fricCFFilter;
		MovingAverageFilter<FloatType> fricBWFilter;
		MovingAverageFilter<FloatType> radius0Filter;
		MovingAverageFilter<FloatType> radius1Filter;
		MovingAverageFilter<FloatType> radius2Filter;
		MovingAverageFilter<FloatType> radius3Filter;
		MovingAverageFilter<FloatType> radius4Filter;
		MovingAverageFilter<FloatType> radius5Filter;
		MovingAverageFilter<FloatType> radius6Filter;
		MovingAverageFilter<FloatType> radius7Filter;
		MovingAverageFilter<FloatType> velumFilter;
		InputFilters(FloatType sampleRate, FloatType period)
			: glotPitchFilter(sampleRate, period)
			, glotVolFilter(sampleRate, period)
			, aspVolFilter(sampleRate, period)
			, fricVolFilter(sampleRate, period)
			, fricPosFilter(sampleRate, period)
			, fricCFFilter(sampleRate, period)
			, fricBWFilter(sampleRate, period)
			, radius0Filter(sampleRate, period)
			, radius1Filter(sampleRate, period)
			, radius2Filter(sampleRate, period)
			, radius3Filter(sampleRate, period)
			, radius4Filter(sampleRate, period)
			, radius5Filter(sampleRate, period)
			, radius6Filter(sampleRate, period)
			, radius7Filter(sampleRate, period)
			, velumFilter(sampleRate, period) {}
		void reset() {
			glotPitchFilter.reset();
			glotVolFilter.reset();
			aspVolFilter.reset();
			fricVolFilter.reset();
			fricPosFilter.reset();
			fricCFFilter.reset();
			fricBWFilter.reset();
			radius0Filter.reset();
			radius1Filter.reset();
			radius2Filter.reset();
			radius3Filter.reset();
			radius4Filter.reset();
			radius5Filter.reset();
			radius6Filter.reset();
			radius7Filter.reset();
			velumFilter.reset();
		}
	};

	VocalTractModel0(const VocalTractModel0&) = delete;
	VocalTractModel0& operator=(const VocalTractModel0&) = delete;

	void reset();
	void calculateTubeCoefficients();
	void initializeNasalCavity();
	void parseInputStream(std::istream& in);
	void sampleRateInterpolation();
	void setControlRateParameters(int pos);
	void setFricationTaps();
	double vocalTract(double input, double frication);
	void writeOutputToFile(const char* outputFile);
	void writeOutputToBuffer(std::vector<float>& outputBuffer);
	void synthesize();
	float calculateOutputScale();

	template<typename T> T readParameterFromInputStream(std::istream& in, std::string& line, const char* paramName);

	Configuration config_;

	/*  DERIVED VALUES  */
	int    controlPeriod_;
	int    sampleRate_;
	double actualTubeLength_;            /*  actual length in cm  */

	/*  MEMORY FOR TUBE AND TUBE COEFFICIENTS  */
	double oropharynx_[TOTAL_SECTIONS][2][2];
	double oropharynxCoeff_[TOTAL_COEFFICIENTS];

	double nasal_[TOTAL_NASAL_SECTIONS][2][2];
	double nasalCoeff_[TOTAL_NASAL_COEFFICIENTS];

	double alpha_[TOTAL_ALPHA_COEFFICIENTS];
	int currentPtr_;
	int prevPtr_;

	/*  MEMORY FOR FRICATION TAPS  */
	double fricationTap_[TOTAL_FRIC_COEFFICIENTS];

	double dampingFactor_;               /*  calculated damping factor  */
	double crossmixFactor_;              /*  calculated crossmix factor  */
	double breathinessFactor_;

	double prevGlotAmplitude_;

	std::vector<std::unique_ptr<InputData>> inputData_;
	CurrentData currentData_;
	InputData singleInput_;
	std::size_t outputDataPos_;
	std::vector<float> outputData_;
	std::unique_ptr<SampleRateConverter<FloatType>> srConv_;
	std::unique_ptr<RadiationFilter<FloatType>>  mouthRadiationFilter_;
	std::unique_ptr<ReflectionFilter<FloatType>> mouthReflectionFilter_;
	std::unique_ptr<RadiationFilter<FloatType>>  nasalRadiationFilter_;
	std::unique_ptr<ReflectionFilter<FloatType>> nasalReflectionFilter_;
	std::unique_ptr<Throat<FloatType>>                 throat_;
	std::unique_ptr<WavetableGlottalSource<FloatType>> glottalSource_;
	std::unique_ptr<BandpassFilter<FloatType>>         bandpassFilter_;
	std::unique_ptr<NoiseFilter<FloatType>>            noiseFilter_;
	std::unique_ptr<NoiseSource>                       noiseSource_;
	std::unique_ptr<InputFilters>                      inputFilters_;
};



template<typename FloatType>
VocalTractModel0<FloatType>::VocalTractModel0()
{
	reset();

	inputData_.reserve(INPUT_VECTOR_RESERVE);
	outputData_.reserve(OUTPUT_VECTOR_RESERVE);
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::reset()
{
	config_.outputRate       = 0.0;
	config_.controlRate      = 0.0;
	config_.volume           = 0.0;
	config_.waveform         = 0;
	config_.tp               = 0.0;
	config_.tnMin            = 0.0;
	config_.tnMax            = 0.0;
	config_.breathiness      = 0.0;
	config_.length           = 0.0;
	config_.temperature      = 0.0;
	config_.lossFactor       = 0.0;
	config_.apertureRadius   = 0.0;
	config_.mouthCoef        = 0.0;
	config_.noseCoef         = 0.0;
	memset(config_.noseRadius, 0, sizeof(double) * TOTAL_NASAL_SECTIONS);
	config_.throatCutoff     = 0.0;
	config_.throatVol        = 0.0;
	config_.modulation       = 0;
	config_.mixOffset        = 0.0;
	controlPeriod_    = 0;
	sampleRate_       = 0;
	actualTubeLength_ = 0.0;
	memset(&oropharynx_[0][0][0], 0, sizeof(double) * TOTAL_SECTIONS * 2 * 2);
	memset(oropharynxCoeff_,      0, sizeof(double) * TOTAL_COEFFICIENTS);
	memset(&nasal_[0][0][0],      0, sizeof(double) * TOTAL_NASAL_SECTIONS * 2 * 2);
	memset(nasalCoeff_,           0, sizeof(double) * TOTAL_NASAL_COEFFICIENTS);
	memset(alpha_,                0, sizeof(double) * TOTAL_ALPHA_COEFFICIENTS);
	currentPtr_ = 1;
	prevPtr_    = 0;
	memset(fricationTap_, 0, sizeof(double) * TOTAL_FRIC_COEFFICIENTS);
	dampingFactor_     = 0.0;
	crossmixFactor_    = 0.0;
	breathinessFactor_ = 0.0;
	prevGlotAmplitude_ = -1.0;
	inputData_.resize(0);
	memset(&currentData_, 0, sizeof(CurrentData));
	memset(&singleInput_, 0, sizeof(InputData));
	outputDataPos_ = 0;
	outputData_.resize(0);

	if (srConv_) srConv_->reset();
	if (mouthRadiationFilter_) mouthRadiationFilter_->reset();
	if (mouthReflectionFilter_) mouthReflectionFilter_->reset();
	if (nasalRadiationFilter_) nasalRadiationFilter_->reset();
	if (nasalReflectionFilter_) nasalReflectionFilter_->reset();
	if (throat_) throat_->reset();
	if (glottalSource_) glottalSource_->reset();
	if (bandpassFilter_) bandpassFilter_->reset();
	if (noiseFilter_) noiseFilter_->reset();
	if (noiseSource_) noiseSource_->reset();
	if (inputFilters_) inputFilters_->reset();
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::synthesizeToFile(std::istream& inputStream, const char* outputFile)
{
	if (!outputData_.empty()) {
		reset();
	}
	parseInputStream(inputStream);
	initializeSynthesizer();
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
	initializeSynthesizer();
	synthesizeForInputSequence();
	writeOutputToBuffer(outputBuffer);
}

template<typename FloatType>
template<typename T>
T
VocalTractModel0<FloatType>::readParameterFromInputStream(std::istream& in, std::string& line, const char* paramName)
{
	if (!std::getline(in, line)) {
		THROW_EXCEPTION(VTMException, "[VTM::VocalTractModel0] Error in input parameter parsing: Could not read " << paramName << '.');
	}
	return Text::parseString<T>(line);
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::parseInputStream(std::istream& in)
{
	std::string line;

	config_.outputRate     = readParameterFromInputStream<float>( in, line, "output sample rate");
	config_.controlRate    = readParameterFromInputStream<float>( in, line, "input control rate");
	config_.volume         = readParameterFromInputStream<double>(in, line, "master volume");
	config_.waveform       = readParameterFromInputStream<int>(   in, line, "glottal source waveform type");
	config_.tp             = readParameterFromInputStream<double>(in, line, "glottal pulse rise time (tp)");
	config_.tnMin          = readParameterFromInputStream<double>(in, line, "glottal pulse fall time minimum (tnMin)");
	config_.tnMax          = readParameterFromInputStream<double>(in, line, "glottal pulse fall time maximum (tnMax)");
	config_.breathiness    = readParameterFromInputStream<double>(in, line, "glottal source breathiness");
	config_.length         = readParameterFromInputStream<double>(in, line, "nominal tube length");
	config_.temperature    = readParameterFromInputStream<double>(in, line, "tube temperature");
	config_.lossFactor     = readParameterFromInputStream<double>(in, line, "junction loss factor");
	config_.apertureRadius = readParameterFromInputStream<double>(in, line, "aperture scaling radius");
	config_.mouthCoef      = readParameterFromInputStream<double>(in, line, "mouth aperture coefficient");
	config_.noseCoef       = readParameterFromInputStream<double>(in, line, "nose aperture coefficient");

	config_.noseRadius[N1] = 0.0;
	for (int i = N2; i < TOTAL_NASAL_SECTIONS; i++) {
		std::ostringstream out;
		out << "nose radius " << i;
		config_.noseRadius[i] = std::max(readParameterFromInputStream<double>(in, line, out.str().c_str()), GS_VTM_TUBE_MIN_RADIUS);
	}

	config_.throatCutoff   = readParameterFromInputStream<double>(in, line, "throat lowpass filter cutoff");
	config_.throatVol      = readParameterFromInputStream<double>(in, line, "throat volume");
	config_.modulation     = readParameterFromInputStream<int>(   in, line, "pulse modulation of noise flag");
	config_.mixOffset      = readParameterFromInputStream<double>(in, line, "noise crossmix offset");

	/*  GET THE INPUT TABLE VALUES  */
	unsigned int paramNumber = 0;
	while (std::getline(in, line)) {
		std::istringstream lineStream(line);
		auto data = std::make_unique<InputData>();

		/*  GET EACH PARAMETER  */
		lineStream >>
			data->glotPitch >>
			data->glotVol >>
			data->aspVol >>
			data->fricVol >>
			data->fricPos >>
			data->fricCF >>
			data->fricBW;
		for (int i = 0; i < TOTAL_REGIONS; i++) {
			double radius;
			lineStream >> radius;
			data->radius[i] = std::max(radius, GS_VTM_TUBE_MIN_RADIUS);
		}
		lineStream >> data->velum;
		if (!lineStream) {
			THROW_EXCEPTION(VTMException, "[VTM::VocalTractModel0] Error in input parameter parsing: Could not read parameters (number " << paramNumber << ").");
		}

		inputData_.push_back(std::move(data));
		++paramNumber;
	}

	/*  DOUBLE UP THE LAST INPUT TABLE, TO HELP INTERPOLATION CALCULATIONS  */
	if (!inputData_.empty()) {
		auto lastData = std::make_unique<InputData>();
		*lastData = *inputData_.back();
		inputData_.push_back(std::move(lastData));
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
	double nyquist;

	/*  CALCULATE THE SAMPLE RATE, BASED ON NOMINAL TUBE LENGTH AND SPEED OF SOUND  */
	if (config_.length > 0.0) {
		double c = Util::speedOfSound(config_.temperature);
		controlPeriod_ = static_cast<int>(rint((c * TOTAL_SECTIONS * 100.0) / (config_.length * config_.controlRate)));
		sampleRate_ = static_cast<int>(config_.controlRate * controlPeriod_);
		actualTubeLength_ = (c * TOTAL_SECTIONS * 100.0) / sampleRate_;
		nyquist = sampleRate_ / 2.0;
	} else {
		THROW_EXCEPTION(VTMException, "Illegal tube length.\n");
	}

	/*  CALCULATE THE BREATHINESS FACTOR  */
	breathinessFactor_ = config_.breathiness / 100.0;

	/*  CALCULATE CROSSMIX FACTOR  */
	crossmixFactor_ = 1.0 / Util::amplitude60dB(config_.mixOffset);

	/*  CALCULATE THE DAMPING FACTOR  */
	dampingFactor_ = (1.0 - (config_.lossFactor / 100.0));

	/*  INITIALIZE THE WAVE TABLE  */
	glottalSource_ = std::make_unique<WavetableGlottalSource<FloatType>>(
						config_.waveform == GLOTTAL_SOURCE_PULSE ?
							WavetableGlottalSource<FloatType>::TYPE_PULSE :
							WavetableGlottalSource<FloatType>::TYPE_SINE,
						sampleRate_,
						config_.tp, config_.tnMin, config_.tnMax);

	/*  INITIALIZE REFLECTION AND RADIATION FILTER COEFFICIENTS FOR MOUTH  */
	double mouthApertureCoeff = (nyquist - config_.mouthCoef) / nyquist;
	mouthRadiationFilter_  = std::make_unique<RadiationFilter<FloatType>>(mouthApertureCoeff);
	mouthReflectionFilter_ = std::make_unique<ReflectionFilter<FloatType>>(mouthApertureCoeff);

	/*  INITIALIZE REFLECTION AND RADIATION FILTER COEFFICIENTS FOR NOSE  */
	double nasalApertureCoeff = (nyquist - config_.noseCoef) / nyquist;
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
	noiseFilter_ = std::make_unique<NoiseFilter<FloatType>>();
	noiseSource_ = std::make_unique<NoiseSource>();
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::initializeInputFilters(double period)
{
	inputFilters_ = std::make_unique<InputFilters>(sampleRate_, period);
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
	/*  CONTROL RATE LOOP  */
	for (int i = 1, size = inputData_.size(); i < size; i++) {
		/*  SET CONTROL RATE PARAMETERS FROM INPUT TABLES  */
		setControlRateParameters(i);

		/*  SAMPLE RATE LOOP  */
		for (int j = 0; j < controlPeriod_; j++) {
			synthesize();

			/*  DO SAMPLE RATE INTERPOLATION OF CONTROL PARAMETERS  */
			sampleRateInterpolation();
		}
	}
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::synthesizeForSingleInput(int numIterations)
{
	if (!inputFilters_) {
		THROW_EXCEPTION(InvalidStateException, "Input filters have not been initialized.");
	}

	for (int i = 0; i < numIterations; ++i) {

		currentData_.glotPitch = inputFilters_->glotPitchFilter.filter(singleInput_.glotPitch);
		currentData_.glotVol   = inputFilters_->glotVolFilter.filter(  singleInput_.glotVol);
		currentData_.aspVol    = inputFilters_->aspVolFilter.filter(   singleInput_.aspVol);
		currentData_.fricVol   = inputFilters_->fricVolFilter.filter(  singleInput_.fricVol);
		currentData_.fricPos   = inputFilters_->fricPosFilter.filter(  singleInput_.fricPos);
		currentData_.fricCF    = inputFilters_->fricCFFilter.filter(   singleInput_.fricCF);
		currentData_.fricBW    = inputFilters_->fricBWFilter.filter(   singleInput_.fricBW);
		currentData_.radius[0] = inputFilters_->radius0Filter.filter(  singleInput_.radius[0]);
		currentData_.radius[1] = inputFilters_->radius1Filter.filter(  singleInput_.radius[1]);
		currentData_.radius[2] = inputFilters_->radius2Filter.filter(  singleInput_.radius[2]);
		currentData_.radius[3] = inputFilters_->radius3Filter.filter(  singleInput_.radius[3]);
		currentData_.radius[4] = inputFilters_->radius4Filter.filter(  singleInput_.radius[4]);
		currentData_.radius[5] = inputFilters_->radius5Filter.filter(  singleInput_.radius[5]);
		currentData_.radius[6] = inputFilters_->radius6Filter.filter(  singleInput_.radius[6]);
		currentData_.radius[7] = inputFilters_->radius7Filter.filter(  singleInput_.radius[7]);
		currentData_.velum     = inputFilters_->velumFilter.filter(    singleInput_.velum);

		synthesize();
	}
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::synthesize()
{
	/*  CONVERT PARAMETERS HERE  */
	double f0 = Util::frequency(currentData_.glotPitch);
	double ax = Util::amplitude60dB(currentData_.glotVol);
	double ah1 = Util::amplitude60dB(currentData_.aspVol);
	calculateTubeCoefficients();
	setFricationTaps();
	bandpassFilter_->update(sampleRate_, currentData_.fricBW, currentData_.fricCF);

	/*  DO SYNTHESIS HERE  */
	/*  CREATE LOW-PASS FILTERED NOISE  */
	double lpNoise = noiseFilter_->filter(noiseSource_->getSample());

	/*  UPDATE THE SHAPE OF THE GLOTTAL PULSE, IF NECESSARY  */
	if (config_.waveform == GLOTTAL_SOURCE_PULSE) {
		if (ax != prevGlotAmplitude_) {
			glottalSource_->updateWavetable(ax);
		}
	}

	/*  CREATE GLOTTAL PULSE (OR SINE TONE)  */
	double pulse = glottalSource_->getSample(f0);

	/*  CREATE PULSED NOISE  */
	double pulsedNoise = lpNoise * pulse;

	/*  CREATE NOISY GLOTTAL PULSE  */
	pulse = ax * ((pulse * (1.0 - breathinessFactor_)) +
			(pulsedNoise * breathinessFactor_));

	double signal;
	/*  CROSS-MIX PURE NOISE WITH PULSED NOISE  */
	if (config_.modulation) {
		double crossmix = ax * crossmixFactor_;
		crossmix = (crossmix < 1.0) ? crossmix : 1.0;
		signal = (pulsedNoise * crossmix) +
				(lpNoise * (1.0 - crossmix));
	} else {
		signal = lpNoise;
	}

	/*  PUT SIGNAL THROUGH VOCAL TRACT  */
	signal = vocalTract(((pulse + (ah1 * signal)) * VT_SCALE),
				bandpassFilter_->filter(signal));

	/*  PUT PULSE THROUGH THROAT  */
	signal += throat_->process(pulse * VT_SCALE);

	/*  OUTPUT SAMPLE HERE  */
	srConv_->dataFill(signal);

	prevGlotAmplitude_ = ax;
}

/******************************************************************************
*
*  function:  setControlRateParameters
*
*  purpose:   Calculates the current table values, and their
*             associated sample-to-sample delta values.
*
******************************************************************************/
template<typename FloatType>
void
VocalTractModel0<FloatType>::setControlRateParameters(int pos)
{
	double controlFreq = 1.0 / controlPeriod_;

	currentData_.glotPitch = inputData_[pos - 1]->glotPitch;
	currentData_.glotPitchDelta = (inputData_[pos]->glotPitch - currentData_.glotPitch) * controlFreq;

	currentData_.glotVol = inputData_[pos - 1]->glotVol;
	currentData_.glotVolDelta = (inputData_[pos]->glotVol - currentData_.glotVol) * controlFreq;

	currentData_.aspVol = inputData_[pos - 1]->aspVol;
	currentData_.aspVolDelta = (inputData_[pos]->aspVol - currentData_.aspVol) * controlFreq;

	currentData_.fricVol = inputData_[pos - 1]->fricVol;
	currentData_.fricVolDelta = (inputData_[pos]->fricVol - currentData_.fricVol) * controlFreq;

	currentData_.fricPos = inputData_[pos - 1]->fricPos;
	currentData_.fricPosDelta = (inputData_[pos]->fricPos - currentData_.fricPos) * controlFreq;

	currentData_.fricCF = inputData_[pos - 1]->fricCF;
	currentData_.fricCFDelta = (inputData_[pos]->fricCF - currentData_.fricCF) * controlFreq;

	currentData_.fricBW = inputData_[pos - 1]->fricBW;
	currentData_.fricBWDelta = (inputData_[pos]->fricBW - currentData_.fricBW) * controlFreq;

	for (int i = 0; i < TOTAL_REGIONS; i++) {
		currentData_.radius[i] = inputData_[pos - 1]->radius[i];
		currentData_.radiusDelta[i] = (inputData_[pos]->radius[i] - currentData_.radius[i]) * controlFreq;
	}

	currentData_.velum = inputData_[pos - 1]->velum;
	currentData_.velumDelta = (inputData_[pos]->velum - currentData_.velum) * controlFreq;
}

/******************************************************************************
*
*  function:  sampleRateInterpolation
*
*  purpose:   Interpolates table values at the sample rate.
*
******************************************************************************/
template<typename FloatType>
void
VocalTractModel0<FloatType>::sampleRateInterpolation()
{
	currentData_.glotPitch += currentData_.glotPitchDelta;
	currentData_.glotVol   += currentData_.glotVolDelta;
	currentData_.aspVol    += currentData_.aspVolDelta;
	currentData_.fricVol   += currentData_.fricVolDelta;
	currentData_.fricPos   += currentData_.fricPosDelta;
	currentData_.fricCF    += currentData_.fricCFDelta;
	currentData_.fricBW    += currentData_.fricBWDelta;
	for (int i = 0; i < TOTAL_REGIONS; i++) {
		currentData_.radius[i] += currentData_.radiusDelta[i];
	}
	currentData_.velum     += currentData_.velumDelta;
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
	double radA2, radB2;

	/*  CALCULATE COEFFICIENTS FOR INTERNAL FIXED SECTIONS OF NASAL CAVITY  */
	for (int i = N2, j = NC2; i < N6; i++, j++) {
		radA2 = config_.noseRadius[i]     * config_.noseRadius[i];
		radB2 = config_.noseRadius[i + 1] * config_.noseRadius[i + 1];
		nasalCoeff_[j] = (radA2 - radB2) / (radA2 + radB2);
	}

	/*  CALCULATE THE FIXED COEFFICIENT FOR THE NOSE APERTURE  */
	radA2 = config_.noseRadius[N6] * config_.noseRadius[N6];
	radB2 = config_.apertureRadius * config_.apertureRadius;
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
	double radA2, radB2, r0_2, r1_2, r2_2, sum;

	/*  CALCULATE COEFFICIENTS FOR THE OROPHARYNX  */
	for (int i = 0; i < (TOTAL_REGIONS - 1); i++) {
		radA2 = currentData_.radius[i]     * currentData_.radius[i];
		radB2 = currentData_.radius[i + 1] * currentData_.radius[i + 1];
		oropharynxCoeff_[i] = (radA2 - radB2) / (radA2 + radB2);
	}

	/*  CALCULATE THE COEFFICIENT FOR THE MOUTH APERTURE  */
	radA2 = currentData_.radius[R8] * currentData_.radius[R8];
	radB2 = config_.apertureRadius * config_.apertureRadius;
	oropharynxCoeff_[C8] = (radA2 - radB2) / (radA2 + radB2);

	/*  CALCULATE ALPHA COEFFICIENTS FOR 3-WAY JUNCTION  */
	/*  NOTE:  SINCE JUNCTION IS IN MIDDLE OF REGION 4, r0_2 = r1_2  */
	r0_2 = r1_2 = currentData_.radius[R4] * currentData_.radius[R4];
	r2_2 = currentData_.velum * currentData_.velum;
	sum = 2.0 / (r0_2 + r1_2 + r2_2);
	alpha_[LEFT]  = sum * r0_2;
	alpha_[RIGHT] = sum * r1_2;
	alpha_[UPPER] = sum * r2_2;

	/*  AND 1ST NASAL PASSAGE COEFFICIENT  */
	radA2 = currentData_.velum * currentData_.velum;
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
	int integerPart;
	double complement, remainder;
	double fricationAmplitude = Util::amplitude60dB(currentData_.fricVol);

	/*  CALCULATE POSITION REMAINDER AND COMPLEMENT  */
	integerPart = (int) currentData_.fricPos;
	complement = currentData_.fricPos - (double) integerPart;
	remainder = 1.0 - complement;

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
	for (i = FC1; i < TOTAL_FRIC_COEFFICIENTS; i++)
		printf("%.6f  ", fricationTap[i]);
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
double
VocalTractModel0<FloatType>::vocalTract(double input, double frication)
{
	int i, j, k;
	double delta, output, junctionPressure;

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
	for (i = S2, j = C2, k = FC1; i < S4; i++, j++, k++) {
		delta = oropharynxCoeff_[j] *
				(oropharynx_[i][TOP][prevPtr_] - oropharynx_[i + 1][BOTTOM][prevPtr_]);
		oropharynx_[i + 1][TOP][currentPtr_] =
				((oropharynx_[i][TOP][prevPtr_] + delta) * dampingFactor_) +
				(fricationTap_[k] * frication);
		oropharynx_[i][BOTTOM][currentPtr_] =
				(oropharynx_[i + 1][BOTTOM][prevPtr_] + delta) * dampingFactor_;
	}

	/*  UPDATE 3-WAY JUNCTION BETWEEN THE MIDDLE OF R4 AND NASAL CAVITY  */
	junctionPressure = (alpha_[LEFT] * oropharynx_[S4][TOP][prevPtr_])+
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
	for (i = S7, j = C5, k = FC6; i < S10; i++, j++, k++) {
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
	output = mouthRadiationFilter_->filter((1.0 + oropharynxCoeff_[C8]) *
						oropharynx_[S10][TOP][prevPtr_]);

	/*  UPDATE NASAL CAVITY  */
	for (i = VELUM, j = NC1; i < N6; i++, j++) {
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
	output += nasalRadiationFilter_->filter((1.0 + nasalCoeff_[NC6]) *
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

	float scale = calculateOutputScale();
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

	float scale = calculateOutputScale();
	for (unsigned int i = 0, end = srConv_->numberSamples(); i < end; ++i) {
		outputBuffer[i] = outputData_[i] * scale;
	}
}

template<typename FloatType>
float
VocalTractModel0<FloatType>::calculateOutputScale()
{
	float scale = static_cast<float>((OUTPUT_SCALE / srConv_->maximumSampleValue()) * Util::amplitude60dB(config_.volume));
	LOG_DEBUG("\nScale: " << scale << '\n');
	return scale;
}

template<typename FloatType>
void
VocalTractModel0<FloatType>::loadSingleInput(const VocalTractModelParameterValue pv)
{
	switch (pv.index) {
	case PARAM_GLOT_PITCH:
		singleInput_.glotPitch = pv.value;
		break;
	case PARAM_GLOT_VOL:
		singleInput_.glotVol   = pv.value;
		break;
	case PARAM_ASP_VOL:
		singleInput_.aspVol    = pv.value;
		break;
	case PARAM_FRIC_VOL:
		singleInput_.fricVol   = pv.value;
		break;
	case PARAM_FRIC_POS:
		singleInput_.fricPos   = pv.value;
		break;
	case PARAM_FRIC_CF:
		singleInput_.fricCF    = pv.value;
		break;
	case PARAM_FRIC_BW:
		singleInput_.fricBW    = pv.value;
		break;
	case PARAM_R1:
		singleInput_.radius[0] = std::max(pv.value, GS_VTM_TUBE_MIN_RADIUS);
		break;
	case PARAM_R2:
		singleInput_.radius[1] = std::max(pv.value, GS_VTM_TUBE_MIN_RADIUS);
		break;
	case PARAM_R3:
		singleInput_.radius[2] = std::max(pv.value, GS_VTM_TUBE_MIN_RADIUS);
		break;
	case PARAM_R4:
		singleInput_.radius[3] = std::max(pv.value, GS_VTM_TUBE_MIN_RADIUS);
		break;
	case PARAM_R5:
		singleInput_.radius[4] = std::max(pv.value, GS_VTM_TUBE_MIN_RADIUS);
		break;
	case PARAM_R6:
		singleInput_.radius[5] = std::max(pv.value, GS_VTM_TUBE_MIN_RADIUS);
		break;
	case PARAM_R7:
		singleInput_.radius[6] = std::max(pv.value, GS_VTM_TUBE_MIN_RADIUS);
		break;
	case PARAM_R8:
		singleInput_.radius[7] = std::max(pv.value, GS_VTM_TUBE_MIN_RADIUS);
		break;
	case PARAM_VELUM:
		singleInput_.velum     = pv.value;
		break;
	default:
		THROW_EXCEPTION(VTMException, "Invalid parameter index: " << pv.index << '.');
	}
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_VOCALTRACTMODEL0_H_ */
