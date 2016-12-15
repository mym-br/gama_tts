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

#ifndef VTM_BANDPASS_FILTER_H_
#define VTM_BANDPASS_FILTER_H_

#include <cmath>



namespace GS {
namespace VTM {

template<typename FloatType>
class BandpassFilter {
public:
	BandpassFilter();
	~BandpassFilter() {}

	void reset();
	void update(FloatType sampleRate, FloatType bandwidth, FloatType centerFreq);
	FloatType filter(FloatType input);
private:
	BandpassFilter(const BandpassFilter&) = delete;
	BandpassFilter& operator=(const BandpassFilter&) = delete;

	FloatType bpAlpha_;
	FloatType bpBeta_;
	FloatType bpGamma_;
	FloatType xn1_;
	FloatType xn2_;
	FloatType yn1_;
	FloatType yn2_;
};



template<typename FloatType>
BandpassFilter<FloatType>::BandpassFilter()
		: bpAlpha_ {}
		, bpBeta_ {}
		, bpGamma_ {}
		, xn1_ {}
		, xn2_ {}
		, yn1_ {}
		, yn2_ {}
{
}

template<typename FloatType>
void
BandpassFilter<FloatType>::reset()
{
	xn1_ = 0.0;
	xn2_ = 0.0;
	yn1_ = 0.0;
	yn2_ = 0.0;
}

template<typename FloatType>
void
BandpassFilter<FloatType>::update(FloatType sampleRate, FloatType bandwidth, FloatType centerFreq)
{
	const FloatType tanValue = std::tan((static_cast<FloatType>(M_PI) * bandwidth) / sampleRate);
	const FloatType cosValue = std::cos((2.0f * static_cast<FloatType>(M_PI) * centerFreq) / sampleRate);
	bpBeta_ = (1.0f - tanValue) / (2.0f * (1.0f + tanValue));
	bpGamma_ = (0.5f + bpBeta_) * cosValue;
	bpAlpha_ = (0.5f - bpBeta_) / 2.0f;
}

/******************************************************************************
*
*  function:  bandpassFilter
*
*  purpose:   Frication bandpass filter, with variable center
*             frequency and bandwidth.
*
******************************************************************************/
template<typename FloatType>
FloatType
BandpassFilter<FloatType>::filter(FloatType input)
{
	const FloatType output = 2.0f * ((bpAlpha_ * (input - xn2_)) + (bpGamma_ * yn1_) - (bpBeta_ * yn2_));

	xn2_ = xn1_;
	xn1_ = input;
	yn2_ = yn1_;
	yn1_ = output;

	return output;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_BANDPASS_FILTER_H_ */
