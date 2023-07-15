/***************************************************************************
 *  Copyright 2008, 2014 Marcelo Y. Matuda                                 *
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

#ifndef INTERACTIVE_AUDIO_H_
#define INTERACTIVE_AUDIO_H_

#include <cstddef> /* std::size_t */
#include <memory>
#include <vector>

#include "JackClient.h"
#include "JackRingbuffer.h"
#include "MovingAverageFilter.h"
#include "VocalTractModel.h"



namespace GS {

struct InteractiveVTMConfiguration;

class InteractiveAudio {
public:
	enum {
		PARAMETER_RINGBUFFER_SIZE = 32,
		MAX_NUM_SAMPLES_FOR_ANALYSIS = 65536
	};

	class Processor {
	public:
		explicit Processor(std::size_t numberOfParameters);
		~Processor() = default;

		// Called only by the JACK thread.
		int process(jack_nframes_t nframes);

		// Can be called by the main thread only when the JACK thread is not running.
		void reset(jack_port_t* outputPort, InteractiveVTMConfiguration& configuration,
				JackRingbuffer& parameterRingbuffer, JackRingbuffer& analysisRingbuffer);
	private:
		Processor(const Processor&) = delete;
		Processor& operator=(const Processor&) = delete;
		Processor(Processor&&) = delete;
		Processor& operator=(Processor&&) = delete;

		float calcScale(const std::vector<float>& buffer);

		jack_port_t* outputPort_;
		std::size_t vtmBufferPos_;
		float maxAbsSampleValue_;
		std::unique_ptr<VTM::VocalTractModel> vocalTractModel_;
		JackRingbuffer* parameterRingbuffer_;
		JackRingbuffer* analysisRingbuffer_;
		std::vector<float> paramValues_;
		std::vector<VTM::MovingAverageFilter<float>> paramFilters_;
	};

	explicit InteractiveAudio(InteractiveVTMConfiguration& configuration);
	~InteractiveAudio() = default;

	void start();
	void stop();

	JackRingbuffer& parameterRingbuffer() { return *parameterRingbuffer_; }
	JackRingbuffer& analysisRingbuffer() { return *analysisRingbuffer_; }
	unsigned int sampleRate() const { return sampleRate_; }
private:
	enum class State {
		started,
		stopped
	};

	InteractiveAudio(const InteractiveAudio&) = delete;
	InteractiveAudio& operator=(const InteractiveAudio&) = delete;
	InteractiveAudio(InteractiveAudio&&) = delete;
	InteractiveAudio& operator=(InteractiveAudio&&) = delete;

	State state_;
	InteractiveVTMConfiguration& configuration_;
	Processor processor_; // must be accessed only by the JACK thread
	std::unique_ptr<JackRingbuffer> parameterRingbuffer_;
	std::unique_ptr<JackRingbuffer> analysisRingbuffer_;
	std::unique_ptr<JackClient> jackClient_;
	unsigned int sampleRate_;
};

} /* namespace GS */

#endif /* INTERACTIVE_AUDIO_H_ */
