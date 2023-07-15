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

#include "AudioPlayer.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "Exception.h"
#include "JackClient.h"
#include "JackConfig.h"
#include "Log.h"



namespace {

using namespace GS;

extern "C" {

/*******************************************************************************
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 */
int
player_jack_process_callback(jack_nframes_t nframes, void* arg)
{
	return static_cast<AudioPlayer*>(arg)->callback(nframes);
}

/*******************************************************************************
 * JACK calls this function if the server ever shuts down or
 * decides to disconnect the client.
 */
void
player_jack_shutdown_callback(void* arg)
{
	if (Log::debugEnabled) std::cout << "[AudioPlayer] player_jack_shutdown_callback()" << std::endl;

	static_cast<AudioPlayer*>(arg)->stop();
}

} /* extern "C" */

} /* namespace */

//==============================================================================

namespace GS {

AudioPlayer::AudioPlayer()
		: bufferIndex_()
		, jackOutputPort_()
{
}

void
AudioPlayer::play(double sampleRate)
{
	std::lock_guard<std::mutex> lock(bufferMutex_);

	bufferIndex_ = 0;
	playback_finished_ = false;

	auto jackClient = std::make_unique<JackClient>(JackConfig::clientNamePlayer().c_str());

	jackClient->setProcessCallback(player_jack_process_callback, this);
	jackClient->setShutdownCallback(player_jack_shutdown_callback, this);

	jackOutputPort_ = jackClient->registerPort("output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	jack_nframes_t jackSampleRate = jackClient->getSampleRate();
	if (jackSampleRate != static_cast<jack_nframes_t>(sampleRate + 0.5)) {
		THROW_EXCEPTION(JackClientException, "Sampling rate mismatch (JACK: " << jackSampleRate
				<< " GamaTTS: " << sampleRate << ").");
	}

	jackClient->activate();

	// Connect the ports. You can't do this before the client is
	// activated, because we can't make connections to clients
	// that aren't running. Note the confusing (but necessary)
	// orientation of the driver backend ports: playback ports are
	// "input" to the backend, and capture ports are "output" from it.
	JackPorts ports;
	jackClient->getPorts(JackConfig::destinationPortNameRegexp().c_str(), NULL, JackPortIsInput, ports);
	if (ports.list == NULL) {
		THROW_EXCEPTION(JackClientException, "No playback ports.");
	}
	for (std::size_t i = 0; i < 2 && ports.list[i]; ++i) {
		jackClient->connect(JackClient::portName(jackOutputPort_), ports.list[i]);
	}

	do {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	} while (jackOutputPort_ && jack_port_connected(jackOutputPort_) > 0
			&& !playback_finished_.load(std::memory_order_acquire));
}

void
AudioPlayer::copyBuffer(std::vector<float>& out)
{
	std::lock_guard<std::mutex> lock(bufferMutex_);

	out = buffer_;
}

int
AudioPlayer::callback(jack_nframes_t nframes)
{
	if (!jackOutputPort_) return 1; // end
	jack_default_audio_sample_t* out =
		static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(jackOutputPort_, nframes));

	std::size_t outIndex = 0;
	const std::size_t bufferSize = buffer_.size();
	while (bufferIndex_ < bufferSize && outIndex < nframes) {
		out[outIndex] = buffer_[bufferIndex_];
		++bufferIndex_;
		++outIndex;
	}
	while (outIndex < nframes) {
		out[outIndex] = 0.0;
		++outIndex;
	}
	if (bufferIndex_ == bufferSize) {
		// Using this flag because with Pipewire 0.3.65 the "return 1" does not deactivate the client.
		playback_finished_.store(true, std::memory_order_release);

		return 1; // the port may be disconnected
	}

	return 0;
}

void
AudioPlayer::stop()
{
	jackOutputPort_ = nullptr;
}

} // namespace GS
