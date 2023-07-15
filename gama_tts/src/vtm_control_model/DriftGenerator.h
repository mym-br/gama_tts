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

#ifndef VTM_CONTROL_MODEL_DRIFT_GENERATOR_H_
#define VTM_CONTROL_MODEL_DRIFT_GENERATOR_H_

#include "Butterworth2LowpassFilter.h"



namespace GS {
namespace VTMControlModel {

class DriftGenerator {
public:
	DriftGenerator();
	~DriftGenerator() = default;

	void setUp(double deviation, double sampleRate, double lowpassCutoff);
	double drift();
private:
	DriftGenerator(const DriftGenerator&) = delete;
	DriftGenerator& operator=(const DriftGenerator&) = delete;
	DriftGenerator(DriftGenerator&&) = delete;
	DriftGenerator& operator=(DriftGenerator&&) = delete;

	VTM::Butterworth2LowPassFilter<double> filter_;
	double pitchDeviation_;
	double pitchOffset_;
	double seed_;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_DRIFT_GENERATOR_H_ */
