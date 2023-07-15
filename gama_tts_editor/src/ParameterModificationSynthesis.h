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

#ifndef PARAMETER_MODIFICATION_SYNTHESIS_H
#define PARAMETER_MODIFICATION_SYNTHESIS_H

#include <atomic>
#include <memory>
#include <vector>

#include "JackClient.h"
#include "JackRingbuffer.h"
#include "MovingAverageFilter.h"



namespace GS {

class ConfigurationData;
namespace VTM {
class VocalTractModel;
}

class ParameterModificationSynthesis {
public:
	enum Operation {
		OPER_ADD,
		OPER_MULTIPLY,
		OPER_NONE
	};

	struct Modification {
		unsigned int parameter;
		Operation operation;
		float value;

		void clear() {
			parameter = 0;
			operation = OPER_NONE;
			value = 0.0;
		}
	};

	class Processor {
	public:
		Processor(
			unsigned int numberOfParameters,
			JackRingbuffer* parameterRingbuffer,
			const ConfigurationData& vtmConfigData,
			double controlRate);
		~Processor();

		// Called only by the JACK thread.
		int process(jack_nframes_t nframes);
		void stop(); // must be called only by the shutdown callback

		// These functions can be called by the main thread only when the JACK thread is not running.
		void resetData(const std::vector<std::vector<float>>& paramList);
		bool validData() const;
		void prepareSynthesis(jack_port_t* jackOutputPort, float gain);
		template<typename T> void getModifiedParameter(unsigned int parameter, T& paramList) const;
		template<typename T> void getParameter(unsigned int parameter, T& paramList) const;
		void getModifiedParameterList(std::vector<std::vector<float>>& paramList) const;
		void resetParameter(unsigned int parameter);

		// Can be called by any thread.
		bool running() const;
	private:
		Processor(const Processor&) = delete;
		Processor& operator=(const Processor&) = delete;
		Processor(Processor&&) = delete;
		Processor& operator=(Processor&&) = delete;

		unsigned int numParameters_;
		std::atomic<jack_port_t*> outputPort_;
		std::size_t vtmBufferPos_;
		JackRingbuffer* parameterRingbuffer_;
		std::vector<std::vector<float>> paramList_;
		std::vector<std::vector<float>> modifiedParamList_;
		std::unique_ptr<VTM::VocalTractModel> vocalTractModel_;
		std::vector<float> currentParam_;
		std::vector<float> delta_;
		float gain_;
		unsigned int stepIndex_;
		unsigned int paramSetIndex_;
		unsigned int controlSteps_;
		Modification modif_;
		VTM::MovingAverageFilter<float> modifFilter_;
		std::atomic_bool playback_finished_;
	};

	ParameterModificationSynthesis(
		unsigned int numberOfParameters,
		double controlRate,
		const ConfigurationData& vtmConfigData);
	~ParameterModificationSynthesis() = default;

	void startSynthesis(float gain);

	// Returns false when there are no more data to process.
	bool modifyParameter(
			unsigned int parameter,
			Operation operation,
			float value);

	// Returns false when there are no more data to process.
	bool checkSynthesis();

	Processor& processor() { return *processor_; }
private:
	enum {
		PARAMETER_RINGBUFFER_SIZE = 8
	};

	ParameterModificationSynthesis(const ParameterModificationSynthesis&) = delete;
	ParameterModificationSynthesis& operator=(const ParameterModificationSynthesis&) = delete;
	ParameterModificationSynthesis(ParameterModificationSynthesis&&) = delete;
	ParameterModificationSynthesis& operator=(ParameterModificationSynthesis&&) = delete;

	void stop();

	std::unique_ptr<JackRingbuffer> parameterRingbuffer_;
	std::unique_ptr<Processor> processor_; // used by the JACK thread
	std::unique_ptr<JackClient> jackClient_;
};

/*******************************************************************************
 *
 */
template<typename T>
void
ParameterModificationSynthesis::Processor::getModifiedParameter(unsigned int parameter, T& paramList) const
{
	if (parameter >= numParameters_) {
		THROW_EXCEPTION(InvalidParameterException, "Invalid parameter index:" << parameter << '.');
	}

	paramList.resize(modifiedParamList_.size());
	for (std::size_t i = 0, size = modifiedParamList_.size(); i < size; ++i) {
		paramList[i] = modifiedParamList_[i][parameter];
	}
}

/*******************************************************************************
 *
 */
template<typename T>
void
ParameterModificationSynthesis::Processor::getParameter(unsigned int parameter, T& paramList) const
{
	if (parameter >= numParameters_) {
		THROW_EXCEPTION(InvalidParameterException, "Invalid parameter index:" << parameter << '.');
	}

	paramList.resize(paramList_.size());
	for (std::size_t i = 0, size = paramList_.size(); i < size; ++i) {
		paramList[i] = paramList_[i][parameter];
	}
}

} // namespace GS

#endif // PARAMETER_MODIFICATION_SYNTHESIS_H
