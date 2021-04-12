/***************************************************************************
 *  Copyright 2021 Marcelo Y. Matuda                                       *
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

#ifndef VTM_PLUGIN_H
#define VTM_PLUGIN_H

#include <memory>
#include <vector>

#include "VocalTractModel.h"



namespace GS {

class ConfigurationData;

namespace VTM {

class VocalTractModelPlugin : public VocalTractModel {
public:
	explicit VocalTractModelPlugin(ConfigurationData& data, bool interactive=false);
	virtual ~VocalTractModelPlugin();

	virtual void reset();

	virtual double internalSampleRate() const;
	virtual double outputSampleRate() const;

	virtual void setParameter(int parameter, float value);
	virtual void setAllParameters(const std::vector<float>& parameters);

	virtual void execSynthesisStep();
	virtual void finishSynthesis();

	virtual std::vector<float>& outputBuffer();
private:
	void* dll_;
	std::unique_ptr<VocalTractModel> vtm_;
};

} /* namespace VTM */
} /* namespace GS */

#endif // VTM_PLUGIN_H
