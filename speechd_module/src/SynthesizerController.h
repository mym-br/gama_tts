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

#ifndef SYNTHESIZER_CONTROLLER_H_
#define SYNTHESIZER_CONTROLLER_H_

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "ModuleController.h"
#include "RtAudio.h"



namespace GS {
	class Index;
	namespace TextParser {
		class TextParser;
	}
	namespace VTMControlModel {
		class Controller;
		class Model;
	}
}

class SynthesizerController {
public:
	explicit SynthesizerController(ModuleController& moduleController);
	~SynthesizerController();

	// Main thread function.
	void exec();

	void wait();
	int audioCallback(float* outputBuffer, unsigned int nBufferFrames);
private:
	enum class State {
		stopped,
		playing,
		stopping
	};

	SynthesizerController(const SynthesizerController&) = delete;
	SynthesizerController& operator=(const SynthesizerController&) = delete;
	SynthesizerController(SynthesizerController&&) = delete;
	SynthesizerController& operator=(SynthesizerController&&) = delete;

	void init();
	void set();
	void speak();

	ModuleController& moduleController_;
	std::thread synthThread_;

	std::unique_ptr<GS::Index> index_;
	std::unique_ptr<GS::TextParser::TextParser> textParser_;
	std::unique_ptr<GS::VTMControlModel::Model> model_;
	std::unique_ptr<GS::VTMControlModel::Controller> modelController_;

	ModuleController::CommandType commandType_;
	std::string commandMessage_;

	std::vector<float> audioBuffer_;

	ModuleConfiguration moduleConfig_;
	double defaultPitchOffset_;

	unsigned int audioBufferIndex_;

	std::atomic<State> state_;
	float fadeOutAmplitude_;
	float fadeOutDelta_;

	RtAudio audio_;
};

#endif /* SYNTHESIZER_CONTROLLER_H_ */
