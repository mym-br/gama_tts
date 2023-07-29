/*
 * Copyright 2021, 2023 Marcelo Y. Matuda
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

#include "VocalTractModelPlugin.h"

// POSIX.1-2001
#include <dlfcn.h> /* dlclose, dlerror, dlopen, dlsym */

#include <iostream>
#include <string>

#include "ConfigurationData.h"
#include "Exception.h"

#define CONSTRUCT_VTM_SYMBOL "GAMA_TTS_construct_vocal_tract_model"
#define  DESTRUCT_VTM_SYMBOL "GAMA_TTS_destruct_vocal_tract_model"



extern "C" {

typedef void* (*ConstructVTM)(const void *config_data, int is_interactive);
typedef void (*DestructVTM)(void *vtm);

} /* extern "C" */



namespace GS {
namespace VTM {

VocalTractModelPlugin::VocalTractModelPlugin(const ConfigurationData& data, bool interactive)
		: dll_(nullptr)
{
	const std::string dllPath = data.value<std::string>("dll_path");
	dlerror(); // reset error

	dll_ = dlopen(dllPath.c_str(), RTLD_NOW);
	if (dll_ == NULL) {
		if (dllPath.find('/') == std::string::npos) {
			// Try again with relative path.
			const auto relDllPath = "./" + dllPath;
			dll_ = dlopen(relDllPath.c_str(), RTLD_NOW);
			if (dll_ == NULL) {
				THROW_EXCEPTION(UnavailableResourceException, "Error: " << dlerror());
			}
		} else {
			THROW_EXCEPTION(UnavailableResourceException, "Error: " << dlerror());
		}
	}

	ConstructVTM constructVTM = reinterpret_cast<ConstructVTM>(dlsym(dll_, CONSTRUCT_VTM_SYMBOL));
	if (constructVTM == NULL) {
		THROW_EXCEPTION(UnavailableResourceException, "Error: " << dlerror());
	}

	destructVTM_ = dlsym(dll_, DESTRUCT_VTM_SYMBOL);
	if (destructVTM_ == NULL) {
		THROW_EXCEPTION(UnavailableResourceException, "Error: " << dlerror());
	}

	vtm_ = reinterpret_cast<VocalTractModel*>(constructVTM(&data, interactive));
	if (!vtm_) {
		THROW_EXCEPTION(UnavailableResourceException, "Could not construct the vocal tract model.");
	}
}

VocalTractModelPlugin::~VocalTractModelPlugin() noexcept
{
	(*reinterpret_cast<DestructVTM>(destructVTM_))(vtm_); // must be called before dlclose

	dlerror(); // reset error
	int retVal = dlclose(dll_);
	if (retVal != 0) {
		std::cerr << "Error in dlclose: " << dlerror() << std::endl;
	}
}

void
VocalTractModelPlugin::reset() noexcept
{
	return vtm_->reset();
}

double
VocalTractModelPlugin::internalSampleRate() const noexcept
{
	return vtm_->internalSampleRate();
}

double
VocalTractModelPlugin::outputSampleRate() const noexcept
{
	return vtm_->outputSampleRate();
}

void
VocalTractModelPlugin::setParameter(int parameter, float value) noexcept
{
	return vtm_->setParameter(parameter, value);
}

void
VocalTractModelPlugin::setAllParameters(const std::vector<float>& parameters) noexcept
{
	return vtm_->setAllParameters(parameters);
}

void
VocalTractModelPlugin::execSynthesisStep() noexcept
{
	return vtm_->execSynthesisStep();
}

void
VocalTractModelPlugin::finishSynthesis() noexcept
{
	return vtm_->finishSynthesis();
}

std::vector<float>&
VocalTractModelPlugin::outputBuffer() noexcept
{
	return vtm_->outputBuffer();
}

} /* namespace VTM */
} /* namespace GS */
