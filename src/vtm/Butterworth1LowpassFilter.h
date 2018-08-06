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

template<typename FloatType>
class Butterworth1LowPassFilter {
public:
	Butterworth1LowPassFilter();
	~Butterworth1LowPassFilter() {}

	void reset();
	void update(FloatType sampleRate, FloatType cutoffFreq);
	FloatType filter(FloatType x);
private:
	Butterworth1LowPassFilter(const Butterworth1LowPassFilter&) = delete;
	Butterworth1LowPassFilter& operator=(const Butterworth1LowPassFilter&) = delete;

	FloatType b0_;
	FloatType a1_;
	FloatType x1_;
	FloatType y1_;
};



template<typename FloatType>
Butterworth1LowPassFilter<FloatType>::Butterworth1LowPassFilter()
		: b0_()
		, a1_()
		, x1_()
		, y1_()
{
}

template<typename FloatType>
void
Butterworth1LowPassFilter<FloatType>::reset()
{
	x1_ = 0.0;
	y1_ = 0.0;
}

template<typename FloatType>
void
Butterworth1LowPassFilter<FloatType>::update(FloatType sampleRate, FloatType cutoffFreq)
{
	constexpr FloatType MIN_FREQ = 1.0;
	constexpr FloatType MAX_FREQ_COEF = 0.48;
	if (cutoffFreq < MIN_FREQ || cutoffFreq > sampleRate * MAX_FREQ_COEF) {
		THROW_EXCEPTION(InvalidParameterException, "[Butterworth1LowPassFilter] The cutoff frequency must have a value between "
				<< MIN_FREQ << " and " << sampleRate * MAX_FREQ_COEF << '.');
	}

	constexpr FloatType pi = M_PI;
	const FloatType wcT = 2.0f * std::tan(pi * cutoffFreq / sampleRate);
	const FloatType c1 = 1.0f / (wcT + 2.0f);
	b0_ = c1 * wcT;
	// b1_ = b0_
	a1_ = c1 * (wcT - 2.0f);
}

template<typename FloatType>
FloatType
Butterworth1LowPassFilter<FloatType>::filter(FloatType x)
{
	const FloatType y = b0_ * (x + x1_) - a1_ * y1_;
	x1_ = x;
	y1_ = y;
	return y;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_BUTTERWORTH_1_LOWPASS_FILTER_H_ */
