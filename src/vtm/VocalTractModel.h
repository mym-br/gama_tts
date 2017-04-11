/***************************************************************************
 *  Copyright 2016 Marcelo Y. Matuda                                       *
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

#ifndef VTM_VOCAL_TRACT_MODEL_H_
#define VTM_VOCAL_TRACT_MODEL_H_

#include <memory>
#include <vector>

#include "VocalTractModelParameterValue.h"



namespace GS {

class ConfigurationData;

namespace VTM {

class VocalTractModel {
public:
	virtual ~VocalTractModel() {}

	virtual double outputRate() const = 0;

	// ----- Batch mode.
	virtual void synthesizeToFile(std::istream& inputStream, const char* outputFile) = 0;
	virtual void synthesizeToBuffer(std::istream& inputStream, std::vector<float>& outputBuffer) = 0;

	// ----- Interactive mode.
	virtual void loadSingleInput(const VocalTractModelParameterValue pv) = 0;
	// Synthesizes at least numberOfSamples samples (may synthesize more samples).
	virtual void synthesizeSamples(std::size_t numberOfSamples) = 0;
	// Returns the number of samples read.
	virtual std::size_t getOutputSamples(std::size_t n, float* buffer) = 0;

	static std::unique_ptr<VocalTractModel> getInstance(const ConfigurationData& data, bool interactive = false);
};

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_VOCAL_TRACT_MODEL_H_ */
