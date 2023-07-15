/***************************************************************************
 *  Copyright 2017 Marcelo Y. Matuda                                       *
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

#include "ParameterModificationSynthesis.h"

#include <chrono>
#include <cmath> /* rint */
#include <iostream>
#include <thread>

#include "ConfigurationData.h"
#include "Exception.h"
#include "JackConfig.h"
#include "Log.h"
#include "VocalTractModel.h"
#include "VTMUtil.h"

#define PARAMETER_FILTER_PERIOD_SEC (20.0e-3)



namespace {

using namespace GS;

extern "C" {

/*******************************************************************************
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 */
int
param_modif_jack_process_callback(jack_nframes_t nframes, void* arg)
{
	try {
		ParameterModificationSynthesis::Processor* p = static_cast<ParameterModificationSynthesis::Processor*>(arg);
		return p->process(nframes);
	} catch (std::exception& exc) {
		std::cerr << "[ParameterModificationSynthesis/jack_process_callback] Caught exception: " << exc.what() << '.' << std::endl;
		return 1;
	}
}

/*******************************************************************************
 * JACK calls this function if the server ever shuts down or
 * decides to disconnect the client.
 */
void
param_modif_jack_shutdown_callback(void* arg)
{
	if (Log::debugEnabled) std::cout << "[ParameterModificationSynthesis] jack_shutdown_callback()" << std::endl;

	ParameterModificationSynthesis::Processor* p = static_cast<ParameterModificationSynthesis::Processor*>(arg);
	p->stop();
}

} /* extern "C" */

} /* namespace */

//==============================================================================

namespace GS {

/*******************************************************************************
 * Constructor.
 */
ParameterModificationSynthesis::Processor::Processor(
			unsigned int numberOfParameters,
			JackRingbuffer* parameterRingbuffer,
			const ConfigurationData& vtmConfigData,
			double controlRate)
		: numParameters_(numberOfParameters)
		, outputPort_()
		, vtmBufferPos_()
		, parameterRingbuffer_(parameterRingbuffer)
		, vocalTractModel_(VTM::VocalTractModel::getInstance(vtmConfigData, false))
		, currentParam_(numParameters_)
		, delta_(numParameters_)
		, gain_()
		, stepIndex_()
		, paramSetIndex_(1)
		, controlSteps_(static_cast<unsigned int>(std::rint(vocalTractModel_->internalSampleRate() / controlRate)))
		, modifFilter_(static_cast<float>(vocalTractModel_->internalSampleRate()), PARAMETER_FILTER_PERIOD_SEC)
{
	if (!parameterRingbuffer_) {
		THROW_EXCEPTION(MissingValueException, "Missing parameter ringbuffer.");
	}

	modif_.clear();
}

/*******************************************************************************
 * Destructor.
 */
ParameterModificationSynthesis::Processor::~Processor()
{
}

/*******************************************************************************
 *
 */
int
ParameterModificationSynthesis::Processor::process(jack_nframes_t nframes)
{
	if (!outputPort_) {
		// Using this flag because with Pipewire 0.3.65 the "return 1" does not deactivate the client.
		playback_finished_.store(true, std::memory_order_release);

		return 1;
	}
	jack_default_audio_sample_t* out = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(outputPort_, nframes));
	std::vector<float>& vtmOutputBuffer = vocalTractModel_->outputBuffer();

	const std::size_t n = VTM::Util::getSamples(vtmOutputBuffer, vtmBufferPos_, out,
							nframes, gain_);

	if (n == nframes) return 0; // JACK does not need more samples

	// JACK needs more samples.

	const std::size_t targetBufferSize = nframes - n;
	while (vtmOutputBuffer.size() < targetBufferSize) { // while there is not enough data available
		if (paramSetIndex_ >= modifiedParamList_.size()) {
			// Using this flag because with Pipewire 0.3.65 the "return 1" does not deactivate the client.
			playback_finished_.store(true, std::memory_order_release);

			return 1; // the port may be disconnected
		}

		// Get modification data.
		if (parameterRingbuffer_->readSpace() >= sizeof(Modification)) {
#ifndef NDEBUG
			size_t bytesRead =
#endif
			parameterRingbuffer_->read(reinterpret_cast<char*>(&modif_), sizeof(Modification));
			assert(bytesRead == sizeof(Modification));
			assert(modif_.parameter < numParameters_);
		}

		const float filteredModif = (modif_.operation != OPER_NONE) ? modifFilter_.filter(modif_.value) : 0.0;

		// Calculate the parameters for the control step.
		if (stepIndex_ == 0) {
			// Apply the modification.
			const float origValue = paramList_[paramSetIndex_][modif_.parameter];
			if (modif_.operation == OPER_ADD) {
				modifiedParamList_[paramSetIndex_][modif_.parameter] = origValue + filteredModif;
			} else if (modif_.operation == OPER_MULTIPLY) {
				modifiedParamList_[paramSetIndex_][modif_.parameter] = origValue * filteredModif;
			}

			const float coef = 1.0f / controlSteps_;
			for (unsigned int i = 0; i < numParameters_; ++i) {
				currentParam_[i] = modifiedParamList_[paramSetIndex_ - 1][i];
				delta_[i] = (modifiedParamList_[paramSetIndex_][i] - modifiedParamList_[paramSetIndex_ - 1][i]) * coef;
			}
		} else {
			// Do linear interpolation.
			for (unsigned int i = 0; i < numParameters_; ++i) {
				currentParam_[i] += delta_[i];
			}
		}

		if (++stepIndex_ >= controlSteps_) {
			stepIndex_ = 0;
			++paramSetIndex_;
		}

		// Synthesize using the VTM.
		vocalTractModel_->setAllParameters(currentParam_);
		vocalTractModel_->execSynthesisStep();
	}

	[[maybe_unused]] const std::size_t n2 = VTM::Util::getSamples(vtmOutputBuffer, vtmBufferPos_, out + n,
									nframes - n, gain_);
	assert(n2 == nframes - n);

	return 0;
}

/*******************************************************************************
 *
 */
void
ParameterModificationSynthesis::Processor::stop()
{
	outputPort_ = nullptr;
}

/*******************************************************************************
 *
 */
void
ParameterModificationSynthesis::Processor::resetData(const std::vector<std::vector<float>>& paramList) {
	paramList_ = paramList;
	modifiedParamList_ = paramList_;
}

/*******************************************************************************
 *
 */
bool
ParameterModificationSynthesis::Processor::validData() const
{
	return modifiedParamList_.size() >= 2;
}

/*******************************************************************************
 *
 */
void
ParameterModificationSynthesis::Processor::prepareSynthesis(jack_port_t* jackOutputPort, float gain) {
	if (!jackOutputPort) {
		THROW_EXCEPTION(MissingValueException, "Missing JACK output port.");
	}

	outputPort_ = jackOutputPort;
	vtmBufferPos_ = 0;
	gain_ = gain;
	stepIndex_ = 0;
	paramSetIndex_ = 1;
	modif_.clear();
	modifFilter_.reset();
	playback_finished_ = false;
}

/*******************************************************************************
 *
 */
void
ParameterModificationSynthesis::Processor::getModifiedParameterList(std::vector<std::vector<float>>& paramList) const
{
	paramList = modifiedParamList_;
}

/*******************************************************************************
 *
 */
void
ParameterModificationSynthesis::Processor::resetParameter(unsigned int parameter)
{
	if (parameter >= numParameters_) {
		THROW_EXCEPTION(InvalidParameterException, "Invalid parameter index:" << parameter << '.');
	}

	for (std::size_t i = 0, size = paramList_.size(); i < size; ++i) {
		modifiedParamList_[i][parameter] = paramList_[i][parameter];
	}
}

/*******************************************************************************
 *
 */
bool
ParameterModificationSynthesis::Processor::running() const
{
	return outputPort_ && jack_port_connected(outputPort_) > 0
			&& !playback_finished_.load(std::memory_order_acquire);
}

/*******************************************************************************
 * Constructor.
 */
ParameterModificationSynthesis::ParameterModificationSynthesis(
			unsigned int numberOfParameters,
			double controlRate,
			const ConfigurationData& vtmConfigData)
		: parameterRingbuffer_(std::make_unique<JackRingbuffer>(PARAMETER_RINGBUFFER_SIZE * sizeof(Modification)))
		, processor_(std::make_unique<Processor>(
					numberOfParameters,
					parameterRingbuffer_.get(),
					vtmConfigData,
					controlRate))
		, jackClient_()
{
}

/*******************************************************************************
 * Starts the synthesis and the connection to the JACK server.
 */
void
ParameterModificationSynthesis::startSynthesis(float gain)
{
	if (Log::debugEnabled) std::cout << "ParameterModificationSynthesis::startSynthesis" << std::endl;

	if (jackClient_) return;

	auto newJackClient = std::make_unique<JackClient>(JackConfig::clientNameParamModif().c_str());

	jack_port_t* outputPort = newJackClient->registerPort("output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	if (Log::debugEnabled) {
		jack_nframes_t jackSampleRate = newJackClient->getSampleRate();
		std::cout << "Output sample rate: " << jackSampleRate << std::endl;
	}

	// Prepare the audio processor.
	if (!processor_->validData()) {
		THROW_EXCEPTION(InvalidValueException, "Not enough data in the parameter modification synthesis processor.");
	}
	processor_->prepareSynthesis(outputPort, gain);

	newJackClient->setProcessCallback(param_modif_jack_process_callback, processor_.get());
	newJackClient->setShutdownCallback(param_modif_jack_shutdown_callback, processor_.get());

	newJackClient->activate();

	// Connect the ports. You can't do this before the client is
	// activated, because we can't make connections to clients
	// that aren't running. Note the confusing (but necessary)
	// orientation of the driver backend ports: playback ports are
	// "input" to the backend, and capture ports are "output" from it.
	JackPorts ports;
	newJackClient->getPorts(JackConfig::destinationPortNameRegexp().c_str(), NULL, JackPortIsInput, ports);
	if (ports.list == NULL) {
		THROW_EXCEPTION(AudioException, "No playback ports.");
	}
	for (size_t i = 0; i < 2 && ports.list[i]; ++i) {
		newJackClient->connect(JackClient::portName(outputPort), ports.list[i]);
	}

	jackClient_ = std::move(newJackClient);

	if (Log::debugEnabled) std::cout << "Audio started." << std::endl;
}

/*******************************************************************************
 * Stops the connection to the JACK server.
 */
void
ParameterModificationSynthesis::stop()
{
	jackClient_.reset();
	parameterRingbuffer_->reset();

	if (Log::debugEnabled) std::cout << "Audio stopped." << std::endl;
	return;
}

/*******************************************************************************
 *
 */
bool
ParameterModificationSynthesis::modifyParameter(
		unsigned int parameter,
		Operation operation,
		float value)
{
	if (!processor_->running()) {
		stop();
		return false;
	}

	if (parameterRingbuffer_->writeSpace() >= sizeof(Modification)) {
		Modification modif;
		modif.parameter = parameter;
		modif.operation = operation;
		modif.value = value;
		parameterRingbuffer_->write(reinterpret_cast<const char*>(&modif), sizeof(Modification));
	}

	return true;
}

/*******************************************************************************
 *
 */
bool
ParameterModificationSynthesis::checkSynthesis()
{
	if (!processor_->running()) {
		stop();
		return false;
	}
	return true;
}

} // namespace GS
