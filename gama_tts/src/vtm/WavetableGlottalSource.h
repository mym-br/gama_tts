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

#ifndef VTM_WAVETABLE_GLOTTAL_SOURCE_H_
#define VTM_WAVETABLE_GLOTTAL_SOURCE_H_

#include <algorithm> /* max */
#include <cmath>
#include <memory>
#include <vector>

#include "Exception.h"
#include "WavetableGlottalSourceFIRFilter.h"

/*  COMPILE WITH OVERSAMPLING OR PLAIN OSCILLATOR  */
#define VTM_WAVETABLE_GLOTTAL_SOURCE_OVERSAMPLING_OSCILLATOR   1



namespace GS {
namespace VTM {

template<typename TFloat>
class WavetableGlottalSource {
public:
	/*  WAVEFORM TYPES  */
	enum class Type {
		pulse,
		sine
	};

	WavetableGlottalSource(
			Type type, TFloat sampleRate,
			TFloat tp = 0.0, TFloat tnMin = 0.0, TFloat tnMax = 0.0);
	~WavetableGlottalSource() = default;

	void reset();
	TFloat getSample(TFloat frequency);
	void setup(TFloat amplitude);
private:
	WavetableGlottalSource(const WavetableGlottalSource&) = delete;
	WavetableGlottalSource& operator=(const WavetableGlottalSource&) = delete;
	WavetableGlottalSource(WavetableGlottalSource&&) = delete;
	WavetableGlottalSource& operator=(WavetableGlottalSource&&) = delete;

	void incrementTablePosition(TFloat frequency);

	TFloat mod0(TFloat value);

	/*  GLOTTAL SOURCE OSCILLATOR TABLE VARIABLES  */
	const unsigned int tableLength_;
	const unsigned int tableModulus_;

	/*  OVERSAMPLING FIR FILTER CHARACTERISTICS  */
	const TFloat firBeta_;
	const TFloat firGamma_;
	const TFloat firCutoff_;

	unsigned int tableDiv1_;
	unsigned int tableDiv2_;
	TFloat tnLength_;
	TFloat tnDelta_;
	TFloat basicIncrement_;
	TFloat currentPosition_;
	std::vector<TFloat> wavetable_;
	std::unique_ptr<WavetableGlottalSourceFIRFilter<TFloat>> firFilter_;
	TFloat prevAmplitude_;
};



template<typename TFloat>
WavetableGlottalSource<TFloat>::WavetableGlottalSource(Type type, TFloat sampleRate,
			TFloat tp, TFloat tnMin, TFloat tnMax)
		: tableLength_(512)
		, tableModulus_(tableLength_ - 1)
		, firBeta_(0.2)
		, firGamma_(0.1)
		, firCutoff_(0.00000001)
		, wavetable_(tableLength_)
		, prevAmplitude_(-1.0)
{
	// Calculates the initial glottal pulse and stores it
	// in the wavetable, for use in the oscillator.

	/*  CALCULATE WAVE TABLE PARAMETERS  */
	tableDiv1_ = static_cast<unsigned int>(std::rint(tableLength_ * (tp / 100.0f)));
	tableDiv2_ = static_cast<unsigned int>(std::rint(tableLength_ * ((tp + tnMax) / 100.0f)));
	tnLength_ = tableDiv2_ - tableDiv1_;
	tnDelta_ = std::rint(tableLength_ * ((tnMax - tnMin) / 100.0f));
	basicIncrement_ = tableLength_ / sampleRate;
	currentPosition_ = 0.0;

	/*  INITIALIZE THE WAVETABLE WITH EITHER A GLOTTAL PULSE OR SINE TONE  */
	if (type == Type::pulse) {
		/*  CALCULATE RISE PORTION OF WAVE TABLE  */
		for (unsigned int i = 0; i < tableDiv1_; i++) {
			const TFloat x = static_cast<TFloat>(i) / tableDiv1_;
			const TFloat x2 = x * x;
			const TFloat x3 = x2 * x;
			wavetable_[i] = (3.0f * x2) - (2.0f * x3);
		}

		/*  CALCULATE FALL PORTION OF WAVE TABLE  */
		for (unsigned int i = tableDiv1_, j = 0; i < tableDiv2_; i++, j++) {
			const TFloat x = static_cast<TFloat>(j) / tnLength_;
			wavetable_[i] = 1.0f - (x * x);
		}

		/*  SET CLOSED PORTION OF WAVE TABLE  */
		for (unsigned int i = tableDiv2_; i < tableLength_; i++) {
			wavetable_[i] = 0.0;
		}
	} else {
		/*  SINE WAVE  */
		for (unsigned int i = 0; i < tableLength_; i++) {
			wavetable_[i] = std::sin((static_cast<TFloat>(i) / tableLength_) * 2.0f * static_cast<TFloat>(M_PI));
		}
	}

#if VTM_WAVETABLE_GLOTTAL_SOURCE_OVERSAMPLING_OSCILLATOR
	firFilter_ = std::make_unique<WavetableGlottalSourceFIRFilter<TFloat>>(firBeta_, firGamma_, firCutoff_);
#endif
}

template<typename TFloat>
void
WavetableGlottalSource<TFloat>::reset()
{
	currentPosition_ = 0;
	firFilter_->reset();
	prevAmplitude_ = -1.0;
}

/******************************************************************************
*
*  function:  updateWavetable
*
*  purpose:   Rewrites the changeable part of the glottal pulse
*             according to the amplitude.
*
******************************************************************************/
template<typename TFloat>
void
WavetableGlottalSource<TFloat>::setup(TFloat amplitude)
{
	if (tnDelta_ == 0.0 || amplitude == prevAmplitude_) {
		return;
	} else {
		prevAmplitude_ = amplitude;
	}

	/*  CALCULATE NEW CLOSURE POINT, BASED ON AMPLITUDE  */
	const TFloat newDiv2 = std::max(tableDiv2_ - std::rint(amplitude * tnDelta_), TFloat{0.0});
	const TFloat invNewTnLength = 1.0f / (newDiv2 - tableDiv1_);

	/*  RECALCULATE THE FALLING PORTION OF THE GLOTTAL PULSE  */
	TFloat x = 0.0;
	for (unsigned int i = tableDiv1_, end = static_cast<unsigned int>(newDiv2); i < end; ++i, x += invNewTnLength) {
		wavetable_[i] = 1.0f - (x * x);
	}

	/*  FILL IN WITH CLOSED PORTION OF GLOTTAL PULSE  */
	for (unsigned int i = static_cast<unsigned int>(newDiv2); i < tableDiv2_; i++) {
		wavetable_[i] = 0.0;
	}
}

/******************************************************************************
*
*  function:  incrementTablePosition
*
*  purpose:   Increments the position in the wavetable according to
*             the desired frequency.
*
******************************************************************************/
template<typename TFloat>
void
WavetableGlottalSource<TFloat>::incrementTablePosition(TFloat frequency)
{
	currentPosition_ = mod0(currentPosition_ + (frequency * basicIncrement_));
}

/******************************************************************************
*
*  function:  oscillator
*
*  purpose:   Is a 2X oversampling interpolating wavetable
*             oscillator.
*
******************************************************************************/
#if VTM_WAVETABLE_GLOTTAL_SOURCE_OVERSAMPLING_OSCILLATOR
template<typename TFloat>
TFloat
WavetableGlottalSource<TFloat>::getSample(TFloat frequency)  /*  2X OVERSAMPLING OSCILLATOR  */
{
	TFloat output{};

	for (int i = 0; i < 2; i++) {
		/*  FIRST INCREMENT THE TABLE POSITION, DEPENDING ON FREQUENCY  */
		incrementTablePosition(frequency / 2.0f);

		/*  FIND SURROUNDING INTEGER TABLE POSITIONS  */
		const unsigned int lowerPosition = static_cast<unsigned int>(currentPosition_);
		const unsigned int upperPosition = static_cast<unsigned int>(mod0(lowerPosition + 1));
		if (lowerPosition >= wavetable_.size()) {
			THROW_EXCEPTION(InvalidValueException, "[WavetableGlottalSource] lowerPosition >= wavetable_.size().");
		}
		if (upperPosition >= wavetable_.size()) {
			THROW_EXCEPTION(InvalidValueException, "[WavetableGlottalSource] upperPosition >= wavetable_.size().");
		}

		/*  CALCULATE INTERPOLATED TABLE VALUE  */
		const TFloat interpolatedValue = wavetable_[lowerPosition] +
					((currentPosition_ - lowerPosition) *
					(wavetable_[upperPosition] - wavetable_[lowerPosition]));

		/*  PUT VALUE THROUGH FIR FILTER  */
		output = firFilter_->filter(interpolatedValue, i);
	}

	/*  SINCE WE DECIMATE, TAKE ONLY THE SECOND OUTPUT VALUE  */
	return output;
}
#else
template<typename TFloat>
TFloat
WavetableGlottalSource<TFloat>::getSample(TFloat frequency)  /*  PLAIN OSCILLATOR  */
{
	/*  FIRST INCREMENT THE TABLE POSITION, DEPENDING ON FREQUENCY  */
	incrementTablePosition(frequency);

	/*  FIND SURROUNDING INTEGER TABLE POSITIONS  */
	const unsigned int lowerPosition = static_cast<unsigned int>(currentPosition_);
	const unsigned int upperPosition = static_cast<unsigned int>(mod0(lowerPosition + 1));

	/*  RETURN INTERPOLATED TABLE VALUE  */
	return wavetable_[lowerPosition] +
		((currentPosition_ - lowerPosition) *
		(wavetable_[upperPosition] - wavetable_[lowerPosition]));
}
#endif

/******************************************************************************
*
*  function:  mod0
*
*  purpose:   Returns the modulus of 'value', keeping it in the
*             range 0 -> TABLE_MODULUS.
*
******************************************************************************/
template<typename TFloat>
TFloat
WavetableGlottalSource<TFloat>::mod0(TFloat value)
{
	if (value > tableModulus_) {
		value -= tableLength_;
	}

	return value;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_WAVETABLE_GLOTTAL_SOURCE_H_ */
