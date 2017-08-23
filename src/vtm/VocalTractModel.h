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



namespace GS {

class ConfigurationData;

namespace VTM {

class VocalTractModel {
public:
	virtual ~VocalTractModel() {}

	virtual void reset() = 0;

	virtual double internalSampleRate() const = 0;
	virtual double outputSampleRate() const = 0;

	virtual void setParameter(int parameter, float value) = 0;
	virtual void setAllParameters(const std::vector<float>& parameters) = 0;

	virtual void execSynthesisStep() = 0;
	virtual void finishSynthesis() = 0;

	virtual std::vector<float>& outputBuffer() = 0;

	static std::unique_ptr<VocalTractModel> getInstance(const ConfigurationData& data, bool interactive = false);
protected:
	enum {
		OUTPUT_BUFFER_RESERVE = 1024
	};
};

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_VOCAL_TRACT_MODEL_H_ */
