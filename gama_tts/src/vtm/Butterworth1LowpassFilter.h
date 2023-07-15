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
#ifndef VTM_BUTTERWORTH_1_LOWPASS_FILTER_H_
#define VTM_BUTTERWORTH_1_LOWPASS_FILTER_H_

#include <cmath>

#include "Exception.h"



namespace GS {
namespace VTM {

template<typename TFloat>
class Butterworth1LowPassFilter {
public:
	Butterworth1LowPassFilter();
	~Butterworth1LowPassFilter() = default;

	void reset();
	void update(TFloat sampleRate, TFloat cutoffFreq);
	TFloat filter(TFloat x);
private:
	Butterworth1LowPassFilter(const Butterworth1LowPassFilter&) = delete;
	Butterworth1LowPassFilter& operator=(const Butterworth1LowPassFilter&) = delete;
	Butterworth1LowPassFilter(Butterworth1LowPassFilter&&) = delete;
	Butterworth1LowPassFilter& operator=(Butterworth1LowPassFilter&&) = delete;

	TFloat b0_;
	TFloat a1_;
	TFloat x1_;
	TFloat y1_;
};



template<typename TFloat>
Butterworth1LowPassFilter<TFloat>::Butterworth1LowPassFilter()
		: b0_()
		, a1_()
		, x1_()
		, y1_()
{
}

template<typename TFloat>
void
Butterworth1LowPassFilter<TFloat>::reset()
{
	x1_ = 0.0;
	y1_ = 0.0;
}

template<typename TFloat>
void
Butterworth1LowPassFilter<TFloat>::update(TFloat sampleRate, TFloat cutoffFreq)
{
	constexpr TFloat MIN_FREQ = 1.0;
	constexpr TFloat MAX_FREQ_COEF = 0.48;
	if (cutoffFreq < MIN_FREQ || cutoffFreq > sampleRate * MAX_FREQ_COEF) {
		THROW_EXCEPTION(InvalidParameterException, "[Butterworth1LowPassFilter] The cutoff frequency must have a value between "
				<< MIN_FREQ << " and " << sampleRate * MAX_FREQ_COEF << '.');
	}

	constexpr TFloat pi = M_PI;
	const TFloat wcT = 2.0f * std::tan(pi * cutoffFreq / sampleRate);
	const TFloat c1 = 1.0f / (wcT + 2.0f);
	b0_ = c1 * wcT;
	// b1_ = b0_
	a1_ = c1 * (wcT - 2.0f);
}

template<typename TFloat>
TFloat
Butterworth1LowPassFilter<TFloat>::filter(TFloat x)
{
	const TFloat y = b0_ * (x + x1_) - a1_ * y1_;
	x1_ = x;
	y1_ = y;
	return y;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_BUTTERWORTH_1_LOWPASS_FILTER_H_ */
