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

#ifndef TRM_UTIL_H_
#define TRM_UTIL_H_

namespace GS {
namespace TRM {
namespace Util {

//******************************************************************************
// Converts dB value to amplitude value.
//
// 0 <= decibelLevel <= 60 dB
//******************************************************************************
double amplitude60dB(double decibelLevel);

//******************************************************************************
// Converts a given pitch to the corresponding frequency.
//
// pitch in semitones, 0 = middle C
//******************************************************************************
double frequency(double pitch);

//******************************************************************************
// Returns the speed of sound according to the value of
// the temperature (in Celsius degrees).
//******************************************************************************
double speedOfSound(double temperature);

} /* namespace Util */
} /* namespace TRM */
} /* namespace GS */

#endif /* TRM_UTIL_H_ */
