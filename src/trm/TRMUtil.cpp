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

#include "TRMUtil.h"

#include <cmath> /* pow */



/*  PITCH VARIABLES  */
#define PITCH_BASE                220.0
#define PITCH_OFFSET              3           /*  MIDDLE C = 0  */

/*  RANGE OF ALL VOLUME CONTROLS  */
#define VOL_MAX_60_DB                   60.0



namespace GS {
namespace TRM {
namespace Util {

double
amplitude60dB(double decibelLevel)
{
	/*  CONVERT 0-60 RANGE TO -60-0 RANGE  */
	decibelLevel -= VOL_MAX_60_DB;

	/*  IF -60 OR LESS, RETURN AMPLITUDE OF 0  */
	if (decibelLevel <= (-VOL_MAX_60_DB)) {
		return 0.0;
	}

	/*  IF 0 OR GREATER, RETURN AMPLITUDE OF 1  */
	if (decibelLevel >= 0.0) {
		return 1.0;
	}

	/*  ELSE RETURN INVERSE LOG VALUE  */
	return std::pow(10.0, decibelLevel / 20.0);
}

double
frequency(double pitch)
{
	return PITCH_BASE * std::pow(2.0, (pitch + PITCH_OFFSET) / 12.0);
}

double
speedOfSound(double temperature)
{
	return 331.4 + (0.6 * temperature);
}

} /* namespace Util */
} /* namespace TRM */
} /* namespace GS */
