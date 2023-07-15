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

#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <atomic>
#include <cstddef> /* std::size_t */
#include <mutex>
#include <vector>

#include <jack/jack.h>



namespace GS {

class AudioPlayer {
public:
	AudioPlayer();
	~AudioPlayer() = default;

	// Called only by the JACK thread.
	int callback(jack_nframes_t nframes);
	void stop(); // must be called only by the shutdown callback

	// These functions can be called by the main thread.
	template<typename T> void fillBuffer(T f);
	void play(double sampleRate); // will block until the end of the playback
	void copyBuffer(std::vector<float>& out);
private:
	AudioPlayer(const AudioPlayer&) = delete;
	AudioPlayer& operator=(const AudioPlayer&) = delete;
	AudioPlayer(AudioPlayer&&) = delete;
	AudioPlayer& operator=(AudioPlayer&&) = delete;

	std::vector<float> buffer_;
	std::size_t bufferIndex_;
	std::mutex bufferMutex_;
	std::atomic<jack_port_t*> jackOutputPort_;
	std::atomic_bool playback_finished_;
};

template<typename T>
void
AudioPlayer::fillBuffer(T f)
{
	std::lock_guard<std::mutex> lock(bufferMutex_);

	f(buffer_);
}

} // namespace GS

#endif // AUDIO_PLAYER_H
