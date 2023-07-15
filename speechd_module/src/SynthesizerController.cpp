/***************************************************************************
 *  Copyright 2015, 2017 Marcelo Y. Matuda                                 *
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

#include "SynthesizerController.h"

#include <cmath> /* pow */
#include <chrono>
#include <exception>
#include <iostream>

#include "ConfigurationData.h"
#include "Controller.h"
#include "Index.h"
#include "Model.h"
#include "TextParser.h"
#include "Util.h"

#define FADE_OUT_TIME_MS 30.0
#define DEFAULT_AUDIO_FRAMES_PER_BUFFER 256



namespace {

int
rtAudioCallback(void* outputBuffer, void* /*inputBuffer*/, unsigned int nBufferFrames,
		double /*streamTime*/, RtAudioStreamStatus status, void* userData)
{
	float* buffer = static_cast<float*>(outputBuffer);
	if (status) {
		std::cerr << "[RtAudio] Stream underflow detected!" << std::endl;
	}
	SynthesizerController* sc = static_cast<SynthesizerController*>(userData);
	return sc->audioCallback(buffer, nBufferFrames);
}

} // namespace



SynthesizerController::SynthesizerController(ModuleController& moduleController)
		: moduleController_(moduleController)
		, synthThread_(&SynthesizerController::exec, std::ref(*this))
		, commandType_(ModuleController::CommandType::none)
		, defaultPitchOffset_()
		, audioBufferIndex_()
		, state_(State::stopped)
		, fadeOutAmplitude_(1.0)
		, fadeOutDelta_()
{
}

SynthesizerController::~SynthesizerController()
{
}

void
SynthesizerController::exec()
{
	for (;;) {
		moduleController_.getSynthCommand(commandType_, commandMessage_, true);

		switch (commandType_) {
		case ModuleController::CommandType::init:
			init();
			break;
		case ModuleController::CommandType::set:
			set();
			break;
		case ModuleController::CommandType::speak:
			speak();
			break;
		case ModuleController::CommandType::quit:
			return;
		default:
			break;
		}
	}
}

void
SynthesizerController::wait()
{
	if (synthThread_.joinable()) {
		synthThread_.join();
	}
}

int
SynthesizerController::audioCallback(float* outputBuffer, unsigned int nBufferFrames)
{
	const State st = state_;

	const unsigned int framesAvailable = audioBuffer_.size() - audioBufferIndex_;
	const unsigned int numFrames = (framesAvailable > nBufferFrames) ? nBufferFrames : framesAvailable;
	if (st == State::stopping) {
		for (unsigned int i = 0; i < numFrames; ++i) {
			fadeOutAmplitude_ -= fadeOutDelta_;
			if (fadeOutAmplitude_ < 0.0) fadeOutAmplitude_ = 0.0;
			const unsigned int baseIndex = i * 2;
			const float value = audioBuffer_[audioBufferIndex_ + i] * fadeOutAmplitude_;
			outputBuffer[baseIndex]     = value;
			outputBuffer[baseIndex + 1] = value;
		}
	} else {
		for (unsigned int i = 0; i < numFrames; ++i) {
			const unsigned int baseIndex = i * 2;
			const float value = audioBuffer_[audioBufferIndex_ + i];
			outputBuffer[baseIndex]     = value;
			outputBuffer[baseIndex + 1] = value;
		}
	}
	audioBufferIndex_ += numFrames;

	for (unsigned int i = numFrames; i < nBufferFrames; ++i) {
		const unsigned int baseIndex = i * 2;
		outputBuffer[baseIndex]     = 0;
		outputBuffer[baseIndex + 1] = 0;
	}
	if (st == State::stopping && fadeOutAmplitude_ == 0.0) {
		state_ = State::stopped;
		return 1;
	}
	if (audioBufferIndex_ == audioBuffer_.size()) {
		state_ = State::stopped;
		return 1;
	} else {
		return 0;
	}
}

void
SynthesizerController::init()
{
	try {
		GS::ConfigurationData data(commandMessage_);
		const std::string configDirPath = data.value<std::string>("config_dir_path");
		int audioOutputDeviceIndex = data.value<int>("audio_output_device_index");

		//----------------------------
		// Initialize the synthesizer.

		index_ = std::make_unique<GS::Index>(configDirPath);

		textParser_ = GS::TextParser::TextParser::getInstance(*index_, GS::VTMControlModel::PhoneticStringFormat::gnuspeech);

		model_ = std::make_unique<GS::VTMControlModel::Model>();
		model_->load(*index_);

		modelController_ = std::make_unique<GS::VTMControlModel::Controller>(*index_, *model_);
		const GS::VTMControlModel::Configuration& vtmControlConfig = modelController_->vtmControlModelConfiguration();
		defaultPitchOffset_ = vtmControlConfig.pitchOffset;

		//-----------------------------
		// Initialize the audio device.

		const double sampleRate = modelController_->outputSampleRate();
		fadeOutDelta_ = 1.0 / (FADE_OUT_TIME_MS * 1.0e-3 * sampleRate);

		RtAudio::StreamParameters audioStreamParameters;
		if (audioOutputDeviceIndex == -1) {
			audioStreamParameters.deviceId = audio_.getDefaultOutputDevice();
		} else {
			audioStreamParameters.deviceId = audioOutputDeviceIndex;
		}
		audioStreamParameters.nChannels = 2;
		audioStreamParameters.firstChannel = 0;
		unsigned int bufferFrames = DEFAULT_AUDIO_FRAMES_PER_BUFFER;

		audio_.openStream(&audioStreamParameters, nullptr, RTAUDIO_FLOAT32,
					static_cast<unsigned int>(sampleRate + 0.5), &bufferFrames, rtAudioCallback, this);

	} catch (const std::exception& exc) {
		std::ostringstream msg;
		msg << "[SynthesizerController::init] Could not initialize the synthesis controller. Reason: " << exc.what() << '.';
		std::string s = msg.str();
		std::cerr << s << std::endl;
		Util::removeLF(s);
		moduleController_.setSynthCommandResult(commandType_, true, s);
		return;
	}

	moduleController_.setSynthCommandResult(commandType_, false, std::string{});
}

void
SynthesizerController::set()
{
	moduleController_.getConfigCopy(moduleConfig_);

	GS::VTMControlModel::Configuration& vtmControlConfig = modelController_->vtmControlModelConfiguration();
	vtmControlConfig.pitchOffset = defaultPitchOffset_ + moduleConfig_.pitch / 10.0;
	vtmControlConfig.tempo = std::pow(10.0, moduleConfig_.rate / 100.0);

	if (moduleConfig_.spellingMode) {
		textParser_->setMode(GS::TextParser::TextParser::Mode::letter);
	} else {
		textParser_->setMode(GS::TextParser::TextParser::Mode::normal);
	}
}

void
SynthesizerController::speak()
{
	if (!modelController_) {
		moduleController_.setSynthCommandResult(commandType_, true, "The synthesizer has not been initialized.");
		return;
	}
	if (!audio_.isStreamOpen()) {
		moduleController_.setSynthCommandResult(commandType_, true, "The audio device has not been initialized.");
		return;
	}

	//--------------------
	// Generate the audio.

	try {
		const std::string phoneticString = textParser_->parse(commandMessage_.c_str());
		modelController_->synthesizePhoneticStringToBuffer(phoneticString, nullptr, audioBuffer_);

	} catch (const std::exception& exc) {
		std::ostringstream msg;
		msg << "[SynthesizerController::speak] Could not synthesize the text. Reason: " << exc.what() << '.';
		std::string s = msg.str();
		std::cerr << s << std::endl;
		Util::removeLF(s);
		moduleController_.setSynthCommandResult(commandType_, true, s);
		return;
	}
	moduleController_.setSynthCommandResult(commandType_, false, std::string{});

	//----------------
	// Play the audio.

	moduleController_.sendBeginEvent();

	audioBufferIndex_ = 0;
	fadeOutAmplitude_ = 1.0;

	try {
		state_ = State::playing;
		bool stopping = false;
		audio_.startStream();
		while (audio_.isStreamRunning()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if (!stopping) {
				if (moduleController_.state() == ModuleController::State::stopRequested) {
					state_ = State::stopping;
					stopping = true;
				}
			}
		}
		if (stopping) {
			moduleController_.sendStopEvent();
			return;
		}
	} catch (const std::exception& exc) {
		std::cerr << "[SynthesizerController::speak] Caught exception: " << exc.what() << '.' << std::endl;
	}

	moduleController_.sendEndEvent();
}
