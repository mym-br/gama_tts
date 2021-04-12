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

#include "vtm_plugin.h"

// POSIX.1-2001
#include <dlfcn.h> /* dlclose, dlerror, dlopen, dlsym */

#include <iostream>
#include <string>

#include "ConfigurationData.h"
#include "Exception.h"

#define GET_VTM_SYMBOL "GAMA_TTS_get_vocal_tract_model"



extern "C" {

typedef void* (*GetVTM)(void *config_data, int is_interactive);

} /* extern "C" */



namespace GS {
namespace VTM {

VocalTractModelPlugin::VocalTractModelPlugin(ConfigurationData& data, bool interactive)
		: dll_(nullptr)
{
	const std::string dllPath = data.value<std::string>("dll_path");

	dll_ = dlopen(dllPath.c_str(), RTLD_NOW);
	if (dll_ == NULL) {
		THROW_EXCEPTION(UnavailableResourceException, "Error: " << dlerror());
	}

	GetVTM getVTM = reinterpret_cast<GetVTM>(dlsym(dll_, GET_VTM_SYMBOL));
	if (getVTM == NULL) {
		THROW_EXCEPTION(UnavailableResourceException, "Error: " << dlerror());
	}

	vtm_.reset(reinterpret_cast<VocalTractModel*>(getVTM(&data, interactive)));
	if (!vtm_) {
		THROW_EXCEPTION(UnavailableResourceException, "Could not construct the vocal tract model.");
	}
}

VocalTractModelPlugin::~VocalTractModelPlugin()
{
	vtm_.reset(); // must be called before dlclose

	int retVal = dlclose(dll_);
	if (retVal != 0) {
		std::cerr << "Error in dlclose: " << dlerror() << std::endl;
	}
}

void
VocalTractModelPlugin::reset()
{
	return vtm_->reset();
}

double
VocalTractModelPlugin::internalSampleRate() const
{
	return vtm_->internalSampleRate();
}

double
VocalTractModelPlugin::outputSampleRate() const
{
	return vtm_->outputSampleRate();
}

void
VocalTractModelPlugin::setParameter(int parameter, float value)
{
	return vtm_->setParameter(parameter, value);
}

void
VocalTractModelPlugin::setAllParameters(const std::vector<float>& parameters)
{
	return vtm_->setAllParameters(parameters);
}

void
VocalTractModelPlugin::execSynthesisStep()
{
	return vtm_->execSynthesisStep();
}

void
VocalTractModelPlugin::finishSynthesis()
{
	return vtm_->finishSynthesis();
}

std::vector<float>&
VocalTractModelPlugin::outputBuffer()
{
	return vtm_->outputBuffer();
}

} /* namespace VTM */
} /* namespace GS */
