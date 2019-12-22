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
#ifndef VTM_BUTTERWORTH_2_LOWPASS_FILTER_H_
#define VTM_BUTTERWORTH_2_LOWPASS_FILTER_H_

#include <cmath>

#include "Exception.h"



namespace GS {
namespace VTM {

template<typename TFloat>
class Butterworth2LowPassFilter {
public:
	Butterworth2LowPassFilter();
	~Butterworth2LowPassFilter() = default;

	void reset();
	void update(TFloat sampleRate, TFloat cutoffFreq);
	TFloat filter(TFloat x);
private:
	Butterworth2LowPassFilter(const Butterworth2LowPassFilter&) = delete;
	Butterworth2LowPassFilter& operator=(const Butterworth2LowPassFilter&) = delete;
	Butterworth2LowPassFilter(Butterworth2LowPassFilter&&) = delete;
	Butterworth2LowPassFilter& operator=(Butterworth2LowPassFilter&&) = delete;

	TFloat b0_;
	TFloat b1_;
	TFloat a1_;
	TFloat a2_;
	TFloat x1_;
	TFloat x2_;
	TFloat y1_;
	TFloat y2_;
};



template<typename TFloat>
Butterworth2LowPassFilter<TFloat>::Butterworth2LowPassFilter()
		: b0_()
		, b1_()
		, a1_()
		, a2_()
		, x1_()
		, x2_()
		, y1_()
		, y2_()
{
}

template<typename TFloat>
void
Butterworth2LowPassFilter<TFloat>::reset()
{
	x1_ = 0.0;
	x2_ = 0.0;
	y1_ = 0.0;
	y2_ = 0.0;
}

template<typename TFloat>
void
Butterworth2LowPassFilter<TFloat>::update(TFloat sampleRate, TFloat cutoffFreq)
{
	constexpr TFloat MIN_FREQ = 1.0;
	constexpr TFloat MAX_FREQ_COEF = 0.48;
	if (cutoffFreq < MIN_FREQ || cutoffFreq > sampleRate * MAX_FREQ_COEF) {
		THROW_EXCEPTION(InvalidParameterException, "[Butterworth2LowPassFilter] The cutoff frequency must have a value between "
				<< MIN_FREQ << " and " << sampleRate * MAX_FREQ_COEF << '.');
	}

	constexpr TFloat pi = M_PI;
	const TFloat wcT = 2.0f * std::tan(pi * cutoffFreq / sampleRate);
	const TFloat wc2T2 = wcT * wcT;
	const TFloat c1 = 2.0f * std::sqrt(TFloat(2.0)) * wcT;
	const TFloat c2 = 1.0f / (wc2T2 + c1 + 4.0f);
	b0_ = c2 * wc2T2;
	b1_ = 2.0f * b0_;
	// b2_ = b0
	a1_ = c2 * (2.0f * wc2T2 - 8.0f);
	a2_ = c2 * (wc2T2 - c1 + 4.0f);
}

template<typename TFloat>
TFloat
Butterworth2LowPassFilter<TFloat>::filter(TFloat x)
{
	const TFloat y = b0_ * (x + x2_) + b1_ * x1_ - a1_ * y1_ - a2_ * y2_;
	x2_ = x1_;
	x1_ = x;
	y2_ = y1_;
	y1_ = y;
	return y;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_BUTTERWORTH_2_LOWPASS_FILTER_H_ */
