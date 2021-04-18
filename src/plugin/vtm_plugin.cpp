/*
 * Copyright 2021 Marcelo Y. Matuda
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
