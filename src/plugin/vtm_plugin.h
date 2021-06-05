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

#ifndef VTM_PLUGIN_H
#define VTM_PLUGIN_H

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
	void* destructVTM_;
	VocalTractModel* vtm_;
};

} /* namespace VTM */
} /* namespace GS */

#endif // VTM_PLUGIN_H
