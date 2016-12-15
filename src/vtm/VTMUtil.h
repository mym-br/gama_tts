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

#include <cmath> /* pow */



namespace GS {
namespace VTM {
namespace Util {

//******************************************************************************
// Converts dB value to amplitude value.
//
// 0 <= decibelLevel <= 60 dB
//******************************************************************************
template<typename FloatType>
FloatType
amplitude60dB(FloatType decibelLevel)
{
	/*  RANGE OF ALL VOLUME CONTROLS  */
	const FloatType volMax60dB = 60.0;

	/*  CONVERT 0-60 RANGE TO -60-0 RANGE  */
	decibelLevel -= volMax60dB;

	/*  IF -60 OR LESS, RETURN AMPLITUDE OF 0  */
	if (decibelLevel <= -volMax60dB) {
		return 0.0;
	}

	/*  IF 0 OR GREATER, RETURN AMPLITUDE OF 1  */
	if (decibelLevel >= 0.0) {
		return 1.0;
	}

	/*  ELSE RETURN INVERSE LOG VALUE  */
	return std::pow(static_cast<FloatType>(10.0), decibelLevel / 20.0f);
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
	const FloatType pitchBase = 220.0;
	const FloatType pitchOffset = 3.0; /*  MIDDLE C = 0  */

	return pitchBase * std::pow(static_cast<FloatType>(2.0), (pitch + pitchOffset) / 12.0f);
}

//******************************************************************************
// Returns the speed of sound according to the value of
// the temperature (in Celsius degrees).
//******************************************************************************
template<typename FloatType>
FloatType
speedOfSound(FloatType temperature)
{
	return 331.4f + (0.6f * temperature);
}

} /* namespace Util */
} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_UTIL_H_ */
