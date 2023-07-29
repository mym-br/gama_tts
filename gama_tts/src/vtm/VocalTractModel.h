/*
 * Copyright 2016 Marcelo Y. Matuda
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

#ifndef VTM_VOCAL_TRACT_MODEL_H_
#define VTM_VOCAL_TRACT_MODEL_H_

#include <memory>
#include <vector>



namespace GS {

class ConfigurationData;

namespace VTM {

class VocalTractModel {
public:
	VocalTractModel() = default;
	virtual ~VocalTractModel() noexcept = default;

	virtual void reset() noexcept = 0;

	virtual double internalSampleRate() const noexcept = 0;
	virtual double outputSampleRate() const noexcept = 0;

	virtual void setParameter(int parameter, float value) noexcept = 0;
	virtual void setAllParameters(const std::vector<float>& parameters) noexcept = 0;

	virtual void execSynthesisStep() noexcept = 0;
	virtual void finishSynthesis() noexcept = 0;

	virtual std::vector<float>& outputBuffer() noexcept = 0;

	static std::unique_ptr<VocalTractModel> getInstance(const ConfigurationData& data, bool interactive = false);
protected:
	enum {
		OUTPUT_BUFFER_RESERVE = 1024
	};
private:
	VocalTractModel(const VocalTractModel&) = delete;
	VocalTractModel& operator=(const VocalTractModel&) = delete;
	VocalTractModel(VocalTractModel&&) = delete;
	VocalTractModel& operator=(VocalTractModel&&) = delete;
};

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_VOCAL_TRACT_MODEL_H_ */
