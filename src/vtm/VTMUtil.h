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

#include <cmath> /* abs, log2, pow */
#include <cstddef> /* std::size_t */
#include <vector>



namespace GS {
namespace VTM {
namespace Util {

// Copies at most n samples from inBuffer (starting from inBufferPos) to outBuffer.
// inBufferPos is updated to the position of the next available sample in inBuffer.
// If all samples from inBuffer were copied, inBuffer is cleared and inBufferPos is set to zero.
// Returns the number of copied samples.
std::size_t getSamples(std::vector<float>& inBuffer, std::size_t& inBufferPos, float* outBuffer, std::size_t n, float scale = 1.0);

float calculateOutputScale(const std::vector<float>& buffer);
float calculateOutputScale(float maximumAbsoluteValue);



//******************************************************************************
// Converts dB level (0 - 60) to amplitude (0 - 1).
//******************************************************************************
template<typename TFloat>
TFloat
amplitude60dB(TFloat decibelLevel)
{
	constexpr TFloat p = 10.0;
	constexpr TFloat k = 1.0 / 20.0;
	constexpr TFloat volMax = 60.0;

	if (decibelLevel <= 0.0) {
		return 0.0;
	}
	if (decibelLevel == volMax) {
		return 1.0;
	}

	/*  CONVERT 0 - 60 RANGE TO -60 - 0 RANGE  */
	decibelLevel -= volMax;

	return std::pow(p, decibelLevel * k);
}

//******************************************************************************
// Converts a pitch to the corresponding frequency (Hz).
//
// pitch in semitones (0 = middle C)
//******************************************************************************
template<typename TFloat>
TFloat
frequency(TFloat pitch)
{
	constexpr TFloat refFreq = 220.0;
	constexpr TFloat pitchOffset = 3.0; /*  MIDDLE C = 0  */
	constexpr TFloat p = 2.0;
	constexpr TFloat k = 1.0 / 12.0;

	return refFreq * std::pow(p, (pitch + pitchOffset) * k);
}

//******************************************************************************
// Converts a frequency (Hz) to the corresponding pitch.
//
// pitch in semitones (0 = middle C)
//******************************************************************************
template<typename TFloat>
TFloat
pitch(TFloat frequency)
{
	constexpr TFloat refFreq = 220.0;
	constexpr TFloat pitchOffset = 3.0; /*  MIDDLE C = 0  */
	constexpr TFloat k = 12.0;

	return k * std::log2(frequency / refFreq) - pitchOffset;
}

//******************************************************************************
// Returns the speed of sound (m/s) at a given temperature (degrees Celsius).
//******************************************************************************
template<typename TFloat>
TFloat
speedOfSound(TFloat temperature)
{
	constexpr TFloat c0 = 331.4;
	constexpr TFloat c1 = 0.6;

	return c0 + (c1 * temperature);
}

template<typename TFloat>
TFloat
maximumAbsoluteValue(const std::vector<TFloat>& v)
{
	TFloat maxValue = 0.0;
	for (TFloat value : v) {
		const TFloat absValue = std::abs(value);
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
