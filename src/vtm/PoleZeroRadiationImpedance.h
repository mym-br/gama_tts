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

#ifndef VTM_POLE_ZERO_RADIATION_IMPEDANCE_H_
#define VTM_POLE_ZERO_RADIATION_IMPEDANCE_H_

#include <cmath>

#include "Exception.h"



namespace GS {
namespace VTM {

/*******************************************************************************
 * The radiation impedance is calculated by the formula:
 *
 * Zr = (d*c/A)*a*(1-z^-1)/(1-b*z^-1)
 *
 * d: air density
 * c: speed of sound
 * A: area
 *
 * described in:
 *
 * - Liljencrants - "Speech synthesis with a reflection-type line analog", 1985.
 * - Laine - "Modelling of lip radiation impedance in z-domain", 1992.
 *
 * The constants a and b were calculated using as reference the radiation
 * impedance of a circular piston in a spherical baffle
 * (sphere radius: 9 cm, c: 353.6 m/s (37°C)), described in:
 *
 * - Chalker, Mackerras - "Models for Representing the Acoustic Radiation
 *   Impedance of the Mouth", 1985.
 * - Morse, Ingard - "Theoretical Acoustics", 1968.
 *
 * Notes:
 * - Zr = 1.0 at fs/2.
 * - Real(Zr) = 0.5 at the transition frequency.
 *
 *
 *
 * in               outT (transmitted)
 * --------->|--------->
 * <---------|
 * outR (reflected)
 *
 * in: flow
 * outR: flow
 *
 * K = (Zr-d*c/A)/(Zr+d*c/A)
 *
 * outT = (1-K)*in
 * outR = K*in
 *
 * K = (a*(1-z^-1)/(1-b*z^-1) - 1) / (a*(1-z^-1)/(1-b*z^-1) + 1)
 *   = (a - a*z^-1 - 1 + b*z^-1) / (a - a*z^-1 + 1 - b*z^-1)
 *   = ((a - 1) + (b - a) * z^-1) / ((a + 1) - (b + a) * z^-1)
 * 1-K = ((a + 1) - (b + a) * z^-1 - (a - 1) - (b - a) * z^-1) / ((a + 1) - (b + a) * z^-1)
 *     = ((2 - 2 * b * z^-1) / ((a + 1) - (b + a) * z^-1)
 *
 * Transmitted:
 * outT[n] = { (a+b)*outT[n-1] +     2*in[n] -   2*b*in[n-1] } / (a+1)
 * outT[n] =     cT1*outT[n-1] +   cT2*in[n] +   cT3*in[n-1]
 *
 * Reflected:
 * outR[n] = { (a+b)*outR[n-1] + (a-1)*in[n] + (b-a)*in[n-1] } / (a+1)
 * outR[n] =     cR1*outR[n-1] +   cR2*in[n] +   cR3*in[n-1]
 */
template<typename FloatType>
class PoleZeroRadiationImpedance {
public:
	PoleZeroRadiationImpedance(FloatType sampleRate);

	void reset();
	void update(FloatType radius /* m */);
	void process(FloatType in /* flow */, FloatType& outT /* flow */, FloatType& outR /* flow */);
private:
	static FloatType transitionFrequency(FloatType radius /* m */);

	FloatType samplePeriod_;
	FloatType in1_; // the previous value
	FloatType outT1_; // the previous value
	FloatType outR1_; // the previous value
	FloatType cT1_, cT2_, cT3_; // transmission coefficients
	FloatType cR1_, cR2_, cR3_; // reflection coefficients
	FloatType oldRadius_;

	static constexpr FloatType MIN_RADIUS = 0.4e-2;
	//static constexpr FloatType MAX_RADIUS = 5.0e-2;
	static constexpr FloatType MIN_SAMPLE_RATE = 50000.0;
};



// sampleRate must be > 50 kHz, but not much higher than 100 kHz.
template<typename FloatType>
PoleZeroRadiationImpedance<FloatType>::PoleZeroRadiationImpedance(FloatType sampleRate)
{
	reset();

	if (sampleRate < MIN_SAMPLE_RATE) {
		THROW_EXCEPTION(InvalidValueException, "[PoleZeroRadiationImpedance] Invalid sample rate: " << sampleRate << " (minimum: " << MIN_SAMPLE_RATE << ").");
	} else {
		samplePeriod_ = 1.0f / sampleRate;
	}
}

template<typename FloatType>
void
PoleZeroRadiationImpedance<FloatType>::reset()
{
	in1_ = 0.0;
	outT1_ = 0.0;
	outR1_ = 0.0;
	cT1_ = 0.0;
	cT2_ = 0.0;
	cT3_ = 0.0;
	cR1_ = 0.0;
	cR2_ = 0.0;
	cR3_ = 0.0;
	oldRadius_ = -1.0;
}

template<typename FloatType>
FloatType
PoleZeroRadiationImpedance<FloatType>::transitionFrequency(FloatType radius) {
	if (radius < MIN_RADIUS) {
		radius = MIN_RADIUS;
	}
	return FloatType{62.547840296135675} / radius + FloatType{321.9571845220485};
}

template<typename FloatType>
void
PoleZeroRadiationImpedance<FloatType>::update(FloatType radius)
{
	if (radius == oldRadius_) {
		return;
	} else {
		oldRadius_ = radius;
	}

	const FloatType transFreq = transitionFrequency(radius);
	const FloatType cosWT = std::cos(FloatType{2.0 * M_PI} * transFreq * samplePeriod_);

	const FloatType qa = 2.0f * cosWT;
	const FloatType qb = -2.0f * (cosWT + 1.0f);
	const FloatType qc = cosWT + 1.0f;
	const FloatType delta = qb * qb - 4.0f * qa * qc;
	const FloatType a = (-qb - std::sqrt(delta)) / (2.0f * qa);
	const FloatType b = 2.0f * a - 1.0f;

	const FloatType coef = 1.0f / (a + 1.0f);
	const FloatType aPlusB = a + b;

	cT1_ =    aPlusB * coef;
	cT2_ =      2.0f * coef;
	cT3_ = -2.0f * b * coef;

	cR1_ =     aPlusB * coef;
	cR2_ = (a - 1.0f) * coef;
	cR3_ =    (b - a) * coef;
}

template<typename FloatType>
void
PoleZeroRadiationImpedance<FloatType>::process(FloatType in, FloatType& outT, FloatType& outR)
{
	outT = cT1_ * outT1_ + cT2_ * in + cT3_ * in1_;
	outR = cR1_ * outR1_ + cR2_ * in + cR3_ * in1_;

	in1_ = in;
	outT1_ = outT;
	outR1_ = outR;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_POLE_ZERO_RADIATION_IMPEDANCE_H_ */
