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

#ifndef VTM_VOCAL_TRACT_MODEL_0_H_
#define VTM_VOCAL_TRACT_MODEL_0_H_

#include <algorithm> /* max */
#include <array>
#include <cstddef> /* std::size_t */
#include <cstring> /* memset */
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

#define GS_VTM0_MIN_RADIUS (0.01)

/*  SCALING CONSTANT FOR INPUT TO VOCAL TRACT & THROAT (MATCHES DSP)  */
//#define GS_VTM0_VT_SCALE                  0.03125     /*  2^(-5)  */
// this is a temporary fix only, to try to match dsp synthesizer
#define GS_VTM0_VT_SCALE                  0.125     /*  2^(-3)  */



namespace GS {
namespace VTM {

template<typename TFloat>
class VocalTractModel0 : public VocalTractModel {
public:
	explicit VocalTractModel0(const ConfigurationData& data, bool interactive=false);
	virtual ~VocalTractModel0() noexcept = default;

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
		GLOTTAL_SOURCE_SINE  = 1
	};
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

	VocalTractModel0(const VocalTractModel0&) = delete;
	VocalTractModel0& operator=(const VocalTractModel0&) = delete;
	VocalTractModel0(VocalTractModel0&&) = delete;
	VocalTractModel0& operator=(VocalTractModel0&&) = delete;

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
	TFloat oropharynx_[TOTAL_SECTIONS][2][2];
	TFloat oropharynxCoeff_[TOTAL_COEFFICIENTS];

	TFloat nasal_[TOTAL_NASAL_SECTIONS][2][2];
	TFloat nasalCoeff_[TOTAL_NASAL_COEFFICIENTS];

	TFloat alpha_[TOTAL_ALPHA_COEFFICIENTS];
	int currentPtr_;
	int prevPtr_;

	/*  MEMORY FOR FRICATION TAPS  */
	TFloat fricationTap_[TOTAL_FRIC_COEFFICIENTS];

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



template<typename TFloat>
VocalTractModel0<TFloat>::VocalTractModel0(const ConfigurationData& data, bool interactive)
		: interactive_(interactive)
{
	loadConfiguration(data);
	VocalTractModel0::reset();
	initializeSynthesizer();
	outputBuffer_.reserve(OUTPUT_BUFFER_RESERVE);
}

template<typename TFloat>
void
VocalTractModel0<TFloat>::loadConfiguration(const ConfigurationData& data)
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

template<typename TFloat>
void
VocalTractModel0<TFloat>::reset() noexcept
{
	memset(&oropharynx_[0][0][0], 0, sizeof(TFloat) * TOTAL_SECTIONS * 2 * 2);
	memset(&nasal_[0][0][0],      0, sizeof(TFloat) * TOTAL_NASAL_SECTIONS * 2 * 2);
	currentPtr_ = 1;
	prevPtr_    = 0;
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
template<typename TFloat>
void
VocalTractModel0<TFloat>::initializeSynthesizer()
{
	TFloat nyquist;

	/*  CALCULATE THE SAMPLE RATE, BASED ON NOMINAL TUBE LENGTH AND SPEED OF SOUND  */
	const TFloat c = Util::speedOfSound(config_.temperature);
	sampleRate_ = static_cast<int>((c * TOTAL_SECTIONS * 100.0f) / config_.length);
	nyquist = sampleRate_ / 2.0f;
	if (!interactive_) LOG_DEBUG("[VocalTractModel0] Internal sample rate: " << sampleRate_);

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

template<typename TFloat>
void
VocalTractModel0<TFloat>::execSynthesisStep() noexcept
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
	signal = vocalTract(((pulse + (ah1 * signal)) * TFloat{GS_VTM0_VT_SCALE}),
				bandpassFilter_->filter(signal));

	/*  PUT PULSE THROUGH THROAT  */
	signal += throat_->process(pulse * TFloat{GS_VTM0_VT_SCALE});

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
template<typename TFloat>
void
VocalTractModel0<TFloat>::initializeNasalCavity()
{
	/*  CALCULATE COEFFICIENTS FOR INTERNAL FIXED SECTIONS OF NASAL CAVITY  */
	for (int i = N2, j = NC2; i < N6; i++, j++) {
		const TFloat radA2 = config_.nasalRadius[i]     * config_.nasalRadius[i];
		const TFloat radB2 = config_.nasalRadius[i + 1] * config_.nasalRadius[i + 1];
		nasalCoeff_[j] = (radA2 - radB2) / (radA2 + radB2);
	}

	/*  CALCULATE THE FIXED COEFFICIENT FOR THE NOSE APERTURE  */
	const TFloat radA2 = config_.nasalRadius[N6] * config_.nasalRadius[N6];
	const TFloat radB2 = config_.apertureRadius * config_.apertureRadius;
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
template<typename TFloat>
void
VocalTractModel0<TFloat>::calculateTubeCoefficients()
{
	/*  CALCULATE COEFFICIENTS FOR THE OROPHARYNX  */
	for (int i = 0; i < (TOTAL_REGIONS - 1); i++) {
		const TFloat radA2 = currentParameter_[PARAM_R1 + i]     * currentParameter_[PARAM_R1 + i];
		const TFloat radB2 = currentParameter_[PARAM_R1 + i + 1] * currentParameter_[PARAM_R1 + i + 1];
		oropharynxCoeff_[i] = (radA2 - radB2) / (radA2 + radB2);
	}

	/*  CALCULATE THE COEFFICIENT FOR THE MOUTH APERTURE  */
	TFloat radA2 = currentParameter_[PARAM_R8] * currentParameter_[PARAM_R8];
	TFloat radB2 = config_.apertureRadius * config_.apertureRadius;
	oropharynxCoeff_[C8] = (radA2 - radB2) / (radA2 + radB2);

	/*  CALCULATE ALPHA COEFFICIENTS FOR 3-WAY JUNCTION  */
	/*  NOTE:  SINCE JUNCTION IS IN MIDDLE OF REGION 4, r0_2 = r1_2  */
	const TFloat r1_2 = currentParameter_[PARAM_R4] * currentParameter_[PARAM_R4];
	const TFloat r0_2 = r1_2;
	const TFloat r2_2 = currentParameter_[PARAM_VELUM] * currentParameter_[PARAM_VELUM];
	const TFloat sum = 2.0f / (r0_2 + r1_2 + r2_2);
	alpha_[LEFT]  = sum * r0_2;
	alpha_[RIGHT] = sum * r1_2;
	alpha_[UPPER] = sum * r2_2;

	/*  AND 1ST NASAL PASSAGE COEFFICIENT  */
	radA2 = r2_2;
	radB2 = config_.nasalRadius[N2] * config_.nasalRadius[N2];
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
template<typename TFloat>
void
VocalTractModel0<TFloat>::setFricationTaps()
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
template<typename TFloat>
TFloat
VocalTractModel0<TFloat>::vocalTract(TFloat input, TFloat frication)
{
	TFloat delta;
	std::swap(prevPtr_, currentPtr_);

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
	const TFloat junctionPressure = (alpha_[LEFT] * oropharynx_[S4][TOP][prevPtr_])+
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
	TFloat output = mouthRadiationFilter_->filter((1.0f + oropharynxCoeff_[C8]) *
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

template<typename TFloat>
void
VocalTractModel0<TFloat>::setParameter(int parameter, float value) noexcept
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
						static_cast<TFloat>(GS_VTM0_MIN_RADIUS));
		break;
	default:
		// Invalid parameter index.
		return; // fail silently
	}
}

template<typename TFloat>
void
VocalTractModel0<TFloat>::setAllParameters(const std::vector<float>& parameters) noexcept
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
					static_cast<TFloat>(GS_VTM0_MIN_RADIUS));
	}

	currentParameter_[PARAM_VELUM] = parameters[PARAM_VELUM];
}

template<typename TFloat>
void
VocalTractModel0<TFloat>::finishSynthesis() noexcept
{
	srConv_->flushBuffer();
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_VOCAL_TRACT_MODEL_0_H_ */
