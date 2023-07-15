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

#define GS_VTM_POLE_ZERO_RAD_IMPED_TRANSITION_RADIUS (0.5e-2)
#define GS_VTM_POLE_ZERO_RAD_IMPED_MIN_SAMPLE_RATE (50000.0)



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
 * (sphere radius: 9 cm, c: 352.4 m/s (35°C)), described in:
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
template<typename TFloat>
class PoleZeroRadiationImpedance {
public:
	explicit PoleZeroRadiationImpedance(TFloat sampleRate);

	void reset();
	void update(TFloat radius /* m */);
	void process(TFloat in /* flow */, TFloat& outT /* flow */, TFloat& outR /* flow */);
private:
	static TFloat transitionFrequency(TFloat radius /* m */);

	TFloat samplePeriod_;
	TFloat in1_; // the previous value
	TFloat outT1_; // the previous value
	TFloat outR1_; // the previous value
	TFloat cT1_, cT2_, cT3_; // transmission coefficients
	TFloat cR1_, cR2_, cR3_; // reflection coefficients
	TFloat prevRadius_;
};



// sampleRate must be > 50 kHz, but not much higher than 100 kHz.
template<typename TFloat>
PoleZeroRadiationImpedance<TFloat>::PoleZeroRadiationImpedance(TFloat sampleRate)
{
	reset();

	if (sampleRate < TFloat{GS_VTM_POLE_ZERO_RAD_IMPED_MIN_SAMPLE_RATE}) {
		THROW_EXCEPTION(InvalidValueException, "[PoleZeroRadiationImpedance] Invalid sample rate: " << sampleRate <<
				" (minimum: " << GS_VTM_POLE_ZERO_RAD_IMPED_MIN_SAMPLE_RATE << ").");
	} else {
		samplePeriod_ = 1.0f / sampleRate;
	}
}

template<typename TFloat>
void
PoleZeroRadiationImpedance<TFloat>::reset()
{
	in1_ = 0.0;
	outT1_ = 0.0;
	outR1_ = 0.0;
	prevRadius_ = -1.0;
}

template<typename TFloat>
TFloat
PoleZeroRadiationImpedance<TFloat>::transitionFrequency(TFloat radius) {
	if (radius < TFloat{GS_VTM_POLE_ZERO_RAD_IMPED_TRANSITION_RADIUS}) {
		radius = GS_VTM_POLE_ZERO_RAD_IMPED_TRANSITION_RADIUS;
	}
	return TFloat{62.3371} / radius + TFloat{320.204};
}

template<typename TFloat>
void
PoleZeroRadiationImpedance<TFloat>::update(TFloat radius)
{
	if (radius == prevRadius_) {
		return;
	} else {
		prevRadius_ = radius;
	}

	const TFloat transFreq = transitionFrequency(radius);
	const TFloat cosWT = std::cos(TFloat{2.0 * M_PI} * transFreq * samplePeriod_);

	const TFloat qa = 2.0f * cosWT;
	const TFloat qb = -2.0f * (cosWT + 1.0f);
	const TFloat qc = cosWT + 1.0f;
	const TFloat delta = qb * qb - 4.0f * qa * qc;
	TFloat a = (-qb - std::sqrt(delta)) / (2.0f * qa);
	const TFloat b = 2.0f * a - 1.0f;

	if (radius < TFloat{GS_VTM_POLE_ZERO_RAD_IMPED_TRANSITION_RADIUS}) {
		a *= TFloat{40391.2} * (radius * radius);
	}

	const TFloat coef = 1.0f / (a + 1.0f);
	const TFloat aPlusB = a + b;

	cT1_ =    aPlusB * coef;
	cT2_ =      2.0f * coef;
	cT3_ = -2.0f * b * coef;

	cR1_ =     aPlusB * coef;
	cR2_ = (a - 1.0f) * coef;
	cR3_ =    (b - a) * coef;
}

template<typename TFloat>
void
PoleZeroRadiationImpedance<TFloat>::process(TFloat in, TFloat& outT, TFloat& outR)
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
