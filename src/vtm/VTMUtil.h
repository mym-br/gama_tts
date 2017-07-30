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
// 2016-06
// This file was copied from Gnuspeech and modified by Marcelo Y. Matuda.

#ifndef VTM_UTIL_H_
#define VTM_UTIL_H_

#include <cmath> /* abs, pow */
#include <cstddef> /* std::size_t */
#include <vector>



namespace GS {
namespace VTM {
namespace Util {

// Copies at most n samples from inBuffer (starting from inBufferPos) to outBuffer.
// inBufferPos is updated to the position of the next available sample in inBuffer.
// If all samples from inBuffer were copied, inBuffer is cleared and inBufferPos is set to zero.
// Returns the number of copied samples.
std::size_t getSamples(std::vector<float>& inBuffer, std::size_t& inBufferPos, float* outBuffer, std::size_t n);

float calculateOutputScale(const std::vector<float>& buffer);



//******************************************************************************
// Converts dB value to amplitude value.
//
// 0 <= decibelLevel <= 60 dB
//******************************************************************************
template<typename FloatType>
FloatType
amplitude60dB(FloatType decibelLevel)
{
	constexpr FloatType p = 10.0;
	constexpr FloatType k = 1.0 / 20.0;

	/*  RANGE OF ALL VOLUME CONTROLS  */
	constexpr FloatType volMax = 60.0;

	/*  CONVERT 0-60 RANGE TO -60-0 RANGE  */
	decibelLevel -= volMax;

	/*  IF -60 OR LESS, RETURN AMPLITUDE OF 0  */
	if (decibelLevel <= -volMax) {
		return 0.0;
	}

	/*  IF 0 OR GREATER, RETURN AMPLITUDE OF 1  */
	if (decibelLevel >= 0.0) {
		return 1.0;
	}

	/*  ELSE RETURN INVERSE LOG VALUE  */
	return std::pow(p, decibelLevel * k);
}

//******************************************************************************
// Converts a given pitch to the corresponding frequency.
//
// pitch in semitones, 0 = middle C
//******************************************************************************
template<typename FloatType>
FloatType
frequency(FloatType pitch)
{
	constexpr FloatType pitchBase = 220.0;
	constexpr FloatType pitchOffset = 3.0; /*  MIDDLE C = 0  */
	constexpr FloatType p = 2.0;
	constexpr FloatType k = 1.0 / 12.0;

	return pitchBase * std::pow(p, (pitch + pitchOffset) * k);
}

//******************************************************************************
// Returns the speed of sound according to the value of
// the temperature (in Celsius degrees).
//******************************************************************************
template<typename FloatType>
FloatType
speedOfSound(FloatType temperature)
{
	constexpr FloatType c0 = 331.4;
	constexpr FloatType c1 = 0.6;

	return c0 + (c1 * temperature);
}

template<typename FloatType>
FloatType
maximumAbsoluteValue(const std::vector<FloatType>& v)
{
	FloatType maxValue = 0.0;
	for (auto& value : v) {
		const FloatType absValue = std::abs(value);
		if (absValue > maxValue) {
			maxValue = absValue;
		}
	}
	return maxValue;
}

} /* namespace Util */
} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_UTIL_H_ */
