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
// 2014-09
// This file was copied from Gnuspeech and modified by Marcelo Y. Matuda.

#include "DriftGenerator.h"

#define INITIAL_SEED     0.7892347
#define FACTOR           377.0



namespace GS {
namespace VTMControlModel {

DriftGenerator::DriftGenerator()
	: pitchDeviation_()
	, pitchOffset_()
	, seed_(INITIAL_SEED)
{
}

/******************************************************************************
*
*	function:	setDriftGenerator
*
*	purpose:	Sets the parameters of the drift generator.
*			
*       arguments:      deviation - the amount of drift in semitones above
*                            and below the median.  A value around 1 or
*                            so should give good results.
*                       sampleRate - the rate of the system in Hz---this
*                            should be the same as the control rate (250 Hz).
*                       lowpassCutoff - the cutoff in Hz of the lowpass filter
*                            applied to the noise generator.  This value must
*                            range from 0 Hz to nyquist.  A low value around
*                            1 - 4 Hz should give good results.
*
******************************************************************************/
void
DriftGenerator::setUp(double deviation, double sampleRate, double lowpassCutoff)
{
	/*  SET PITCH DEVIATION AND OFFSET VARIABLES  */
	pitchDeviation_ = deviation * 2.0;
	pitchOffset_ = deviation;

	filter_.update(sampleRate, lowpassCutoff);
}

/******************************************************************************
*
*	function:	drift
*
*	purpose:	Returns one sample of the drift signal.
*			
******************************************************************************/
double
DriftGenerator::drift()
{
	/*  CREATE RANDOM NUMBER BETWEEN 0 AND 1  */
	const double temp = seed_ * FACTOR;
	seed_ = temp - static_cast<int>(temp);  /* SEED IS SAVED FOR NEXT INVOCATION  */

	/*  CREATE RANDOM SIGNAL WITH RANGE -DEVIATION TO +DEVIATION  */
	const double pitchNoise = (seed_ * pitchDeviation_) - pitchOffset_;

	/*  LOWPASS FILTER THE RANDOM SIGNAL  */
	return filter_.filter(pitchNoise);
}

} /* namespace VTMControlModel */
} /* namespace GS */
