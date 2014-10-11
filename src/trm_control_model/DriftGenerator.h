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

#ifndef TRM_CONTROL_MODEL_DRIFT_GENERATOR_H_
#define TRM_CONTROL_MODEL_DRIFT_GENERATOR_H_



namespace TRMControlModel {

class DriftGenerator {
public:
	DriftGenerator();
	~DriftGenerator();

	void setUp(float deviation, float sampleRate, float lowpassCutoff);
	float drift();
private:
	DriftGenerator(const DriftGenerator&);
	DriftGenerator& operator=(const DriftGenerator&);

	float pitchDeviation_;
	float pitchOffset_;
	float seed_;
	float a0_;
	float b1_;
	float previousSample_;
};

} /* namespace TRMControlModel */

#endif /* TRM_CONTROL_MODEL_DRIFT_GENERATOR_H_ */
