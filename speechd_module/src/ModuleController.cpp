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

#include "ModuleController.h"

#include <sstream>
#include <string_view>

#include "SynthesizerController.h"
#include "Util.h"



namespace {

constexpr std::string_view     audioCmdStr{"AUDIO"};
constexpr std::string_view      charCmdStr{"CHAR"};
constexpr std::string_view      initCmdStr{"INIT"};
constexpr std::string_view       keyCmdStr{"KEY"};
constexpr std::string_view  logLevelCmdStr{"LOGLEVEL"};
constexpr std::string_view      quitCmdStr{"QUIT"};
constexpr std::string_view       setCmdStr{"SET"};
constexpr std::string_view soundIconCmdStr{"SOUND_ICON"};
constexpr std::string_view     speakCmdStr{"SPEAK"};
constexpr std::string_view      stopCmdStr{"STOP"};

constexpr std::string_view respAudioInitStr{      "207 OK RECEIVING AUDIO SETTINGS"};
constexpr std::string_view respAudioDoneStr{      "203 OK AUDIO INITIALIZED"};
constexpr std::string_view respBeginEventStr{     "701 BEGIN"};
constexpr std::string_view respEndEventStr{       "702 END"};
constexpr std::string_view respExecFailStr{       "300 ERROR"};
constexpr std::string_view respLogLevelInitStr{   "207 OK RECEIVING LOGLEVEL SETTINGS"};
constexpr std::string_view respLogLevelDoneStr{   "203 OK LOG LEVEL SET"};
constexpr std::string_view respQuitDoneStr{       "210 OK QUIT"};
constexpr std::string_view respSetInitStr{        "203 OK RECEIVING SETTINGS"};
constexpr std::string_view respSetDoneStr{        "203 OK SETTINGS RECEIVED"};
constexpr std::string_view respSpeakInitStr{      "202 OK RECEIVING MESSAGE"};
constexpr std::string_view respStopEventStr{      "703 STOP"};
constexpr std::string_view respSynthInitDoneStr{  "299-GamaTTS: Initialized successfully.\n"
                                         "299 OK LOADED SUCCESSFULLY"};
constexpr std::string_view respSynthInitFailStr_1{"399-GamaTTS: "};
constexpr std::string_view respSynthInitFailStr_2{"399 ERR CANT INIT MODULE"};
constexpr std::string_view respSynthSpeakFailStr{ "301 ERROR CANT SPEAK"};
constexpr std::string_view respSynthSpeakDoneStr{ "200 OK SPEAKING"};

constexpr std::string_view    capLetRecognStr{"cap_let_recogn"};
constexpr std::string_view             dotStr{"."};
constexpr std::string_view        languageStr{"language"};
constexpr std::string_view        logLevelStr{"log_level"};
constexpr std::string_view           pitchStr{"pitch"};
constexpr std::string_view punctuationModeStr{"punctuation_mode"};
constexpr std::string_view            rateStr{"rate"};
constexpr std::string_view    spellingModeStr{"spelling_mode"};
constexpr std::string_view  synthesisVoiceStr{"synthesis_voice"};
constexpr std::string_view           voiceStr{"voice"};
constexpr std::string_view          volumeStr{"volume"};

} // namespace

ModuleController::ModuleController(std::istream& in, std::ostream& out, const char* configFilePath)
		: state_(State::idle)
		, in_(in)
		, out_(out)
		, configFilePath_(configFilePath)
		, newCommand_()
		, commandType_(CommandType::none)
		, synthController_(std::make_unique<SynthesizerController>(*this))
{
}

ModuleController::~ModuleController()
{
}

void
ModuleController::exec()
{
	std::string line;
	while (std::getline(in_, line)) {
		if (line == speakCmdStr || line == charCmdStr || line == keyCmdStr || line == soundIconCmdStr) {
			handleSpeakCommand();
		} else if (line == initCmdStr) {
			handleInitCommand();
		} else if (line == audioCmdStr) {
			handleAudioCommand();
		} else if (line == logLevelCmdStr) {
			handleLogLevelCommand();
		} else if (line == setCmdStr) {
			handleSetCommand();
		} else if (line == stopCmdStr) {
			handleStopCommand();
		} else if (line == quitCmdStr) {
			handleQuitCommand();
			return;
		} else {
			sendResponse(respExecFailStr);
		}
	}
}

void
ModuleController::handleInitCommand()
{
	setSynthCommand(CommandType::init, configFilePath_);
}

void
ModuleController::handleAudioCommand()
{
	sendResponse(respAudioInitStr);

//	audio_output_method=alsa
//	audio_oss_device=/dev/dsp
//	audio_alsa_device=default
//	audio_nas_server=tcp/localhost:5450
//	audio_pulse_server=default
//	audio_pulse_min_length=100

	std::string line;
	while (std::getline(in_, line)) {
		if (line == dotStr) break;

		// Ignore data.
	}

	sendResponse(respAudioDoneStr);
}

void
ModuleController::handleLogLevelCommand()
{
	sendResponse(respLogLevelInitStr);

	std::string line;
	while (std::getline(in_, line)) {
		if (line == dotStr) break;

		auto p = Util::getNameAndValue(line);
		if (p.first == logLevelStr) {
			std::lock_guard<std::mutex> lock(configMutex_);
			config_.setLogLevel(p.second);
		}
	}

	sendResponse(respLogLevelDoneStr);
}

void
ModuleController::handleSetCommand()
{
	sendResponse(respSetInitStr);

	{
		std::lock_guard<std::mutex> lock(configMutex_);

		std::string line;
		while (std::getline(in_, line)) {
			if (line == dotStr) break;

			auto p = Util::getNameAndValue(line);
			if (p.first == pitchStr) {
				config_.setPitch(p.second);
			} else if (p.first == rateStr) {
				config_.setRate(p.second);
			} else if (p.first == volumeStr) {
				config_.setVolume(p.second);
			} else if (p.first == punctuationModeStr) {
				config_.setPunctuationMode(p.second);
			} else if (p.first == spellingModeStr) {
				config_.setSpellingMode(p.second);
			} else if (p.first == capLetRecognStr) {
				config_.setCapitalLetterRecognition(p.second);
			} else if (p.first == voiceStr) {
				config_.setVoice(p.second);
			} else if (p.first == languageStr) {
				config_.setLanguage(p.second);
			} else if (p.first == synthesisVoiceStr) {
				config_.setSynthesisVoice(p.second);
			}
		}
	}
	setSynthCommand(CommandType::set, std::string{});

	sendResponse(respSetDoneStr);
}

void
ModuleController::handleSpeakCommand()
{
	sendResponse(respSpeakInitStr);

	std::ostringstream out;
	std::string line;
	while (std::getline(in_, line)) {
		if (line == dotStr) break;
		if (out.tellp() != 0) {
			out << ' ';
		}
		out << line;
	}
	std::string msg = out.str();

	Util::stripSSML(msg);

	state_ = State::speaking;
	setSynthCommand(CommandType::speak, msg);
}

void
ModuleController::handleStopCommand()
{
	state_ = State::stopRequested;
}

void
ModuleController::handleQuitCommand()
{
	setSynthCommand(CommandType::quit, std::string());

	synthController_->wait();

	sendResponse(respQuitDoneStr);
}

void
ModuleController::setSynthCommand(CommandType type, const std::string& message)
{
	{
		std::unique_lock<std::mutex> lock(commandMutex_);

		while (newCommand_) {
			commandReceivedCondition_.wait(lock);
		}

		commandType_ = type;
		commandMessage_ = message;

		newCommand_ = true;
	}
	commandSentCondition_.notify_one();
}

void
ModuleController::getSynthCommand(ModuleController::CommandType& type, std::string& message, bool wait)
{
	{
		std::unique_lock<std::mutex> lock(commandMutex_);

		if (wait) {
			while (!newCommand_) {
				commandSentCondition_.wait(lock);
			}
		} else {
			if (!newCommand_) {
				type = CommandType::none;
				message.clear();
				return;
			}
		}

		type = commandType_;
		message = commandMessage_;

		newCommand_ = false;
	}
	commandReceivedCondition_.notify_one();
}

void
ModuleController::setSynthCommandResult(CommandType type, bool failed, const std::string& msg)
{
	switch (type) {
	case CommandType::init:
		if (failed) {
			std::ostringstream stream;
			stream << respSynthInitFailStr_1 << msg << '\n' << respSynthInitFailStr_2;
			sendResponse(stream.str());
		} else {
			sendResponse(respSynthInitDoneStr);
		}
		break;
	case CommandType::speak:
		if (failed) {
			sendResponse(respSynthSpeakFailStr);
			state_ = State::idle;
		} else {
			sendResponse(respSynthSpeakDoneStr);
		}
		break;
	default:
		break;
	}
}

void
ModuleController::sendResponse(const std::string_view& msg)
{
	std::lock_guard<std::mutex> lock(responseMutex_);
	out_ << msg << std::endl;
}

void
ModuleController::sendBeginEvent()
{
	std::lock_guard<std::mutex> lock(responseMutex_);
	out_ << respBeginEventStr << std::endl;
}

void
ModuleController::sendEndEvent()
{
	responseMutex_.lock();
	out_ << respEndEventStr << std::endl;
	responseMutex_.unlock();

	state_ = State::idle;
}

void
ModuleController::sendStopEvent()
{
	responseMutex_.lock();
	out_ << respStopEventStr << std::endl;
	responseMutex_.unlock();

	state_ = State::idle;
}

void
ModuleController::getConfigCopy(ModuleConfiguration& config) {
	std::lock_guard<std::mutex> lock(configMutex_);
	config = config_;
}
