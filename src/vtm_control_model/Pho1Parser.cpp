/***************************************************************************
 *  Copyright 2017, 2021 Marcelo Y. Matuda                                 *
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

#include "Pho1Parser.h"

#include <iostream>
#include <sstream>

#include "ConfigurationData.h"
#include "EventList.h"
#include "Exception.h"
#include "Index.h"
#include "Log.h"
#include "Model.h"
#include "Posture.h"
#include "VTMUtil.h"

#define SEPARATORS " \t"
#define COMMENT_CHAR ';'
#define PHONEME_SEPARATOR '_'
#define CONFIG_FILE "pho1.txt"



namespace {

[[noreturn]] void
throwException(int lineNumber, const char* message)
{
	THROW_EXCEPTION(GS::ParsingException, "[Pho1Parser] Error in phonetic string (line " << lineNumber << "): " << message << '.');
}

} /* namespace */

namespace GS {
namespace VTMControlModel {

Pho1Parser::Pho1Parser(const Index& index, const Model& model, EventList& eventList)
		: model_(model)
		, eventList_(eventList)
{
	const std::string dir = index.entry("pho1_parser_dir");
	ConfigurationData config(dir + CONFIG_FILE);

	std::string phonemeMapFile = config.value<std::string>("phoneme_map_file");
	phonemeMap_.load((dir + phonemeMapFile).c_str());
}

void
Pho1Parser::parse(const std::string& pho)
{
	loadInputData(pho);
	if (Log::debugEnabled) {
		std::cout << "\n[Pho1Parser] Input data:" << std::endl;
		printInputData();
	}
	replacePhonemes();
	if (Log::debugEnabled) {
		std::cout << "\n[Pho1Parser] Input data with replaced phonemes:" << std::endl;
		printInputData();
	}
	fillPostureList();
	eventList_.generateEventList();
	addIntonation();
	eventList_.prepareMacroIntonationInterpolation();
}

void
Pho1Parser::printInputData()
{
	for (auto& item : inputData_) {
		std::cout << item.phoneme << "\n  duration:" << item.duration << std::endl;
		for (auto& point : item.intonationPoints) {
			std::cout << "  intonation point:" << point.position << ',' << point.frequency << '\n';
		}
	}
	std::cout << std::endl;
}

void
Pho1Parser::loadInputData(const std::string& pho)
{
	inputData_.clear();
	std::istringstream in{pho};
	std::string line;
	std::size_t lineNumber = 0;
	while (std::getline(in, line)) {
		++lineNumber;

		std::size_t pos1 = line.find_first_not_of(SEPARATORS);
		if (pos1 == std::string::npos) {
			continue; // empty line
		}
		if (line[pos1] == COMMENT_CHAR) {
			continue; // comment
		}

		Pho1Data item;

		std::istringstream lineStream{line};
		if (!(lineStream >> item.phoneme)) {
			throwException(lineNumber, "Could not get the phoneme.");
		}

		if (!(lineStream >> item.duration)) {
			throwException(lineNumber, "Could not get the duration.");
		}

		float intonPos, intonFreq;
		while (lineStream >> intonPos) {
			if (!(lineStream >> intonFreq)) {
				throwException(lineNumber, "Could not get the intonation frequency.");
			}
			item.intonationPoints.emplace_back(intonPos, intonFreq);
		}

		inputData_.push_back(std::move(item));
	}
}

void
Pho1Parser::replacePhonemes()
{
	for (auto it = inputData_.begin(); it != inputData_.end(); ++it) {
		const char* newPhoneme = phonemeMap_.getEntry(it->phoneme.c_str());
		if (newPhoneme == nullptr) continue; // no mapping

		std::string newPhonemeString{newPhoneme};
		std::size_t pos = newPhonemeString.find_first_of(PHONEME_SEPARATOR);
		if (pos == std::string::npos) { // replacement is single phoneme
			it->phoneme = newPhonemeString;
			continue;
		}

		// Replacement is two phonemes.
		std::string phoneme1 = newPhonemeString.substr(0, pos);
		std::string phoneme2 = newPhonemeString.substr(pos + 1);

		it->phoneme = phoneme2;
		it->duration *= 0.5f;

		Pho1Data newData;
		newData.phoneme = phoneme1;
		newData.duration = it->duration;
		auto newIt = inputData_.insert(it, std::move(newData)); // insert before the current phoneme

		std::vector<Pho1IntonationPoint> pointList;
		pointList.swap(it->intonationPoints);

		for (auto& point : pointList) {
			if (point.position <= 50.0f) {
				point.position *= 2.0f;
				newIt->intonationPoints.push_back(point);
			} else {
				point.position = (point.position - 50.0f) * 2.0f;
				it->intonationPoints.push_back(point);
			}
		}
	}
}

void
Pho1Parser::fillPostureList()
{
	for (auto& item : inputData_) {
		const auto* posture = model_.postureList().find(item.phoneme);
		if (!posture) {
			THROW_EXCEPTION(MissingValueException, "Posture \"" << item.phoneme << "\" not found.");
		}

		// Don't use Posture::SYMB_DURATION, because it is not used in the calculation of times.
		const float postureDuration =
				posture->getSymbolTarget(Posture::SYMB_QSSA) +
				posture->getSymbolTarget(Posture::SYMB_QSSB) +
				posture->getSymbolTarget(Posture::SYMB_TRANSITION);

		item.postureIndex = eventList_.newPostureWithObject(*posture, false);
		eventList_.setCurrentPostureTempo(postureDuration / item.duration);
		eventList_.setCurrentPostureRuleTempo(eventList_.globalTempo());
	}
}

void
Pho1Parser::addIntonation()
{
	for (auto& item : inputData_) {
		for (auto& point : item.intonationPoints) {
			eventList_.addPostureIntonationPoint(
					item.postureIndex, point.position / 100.0f,
					VTM::Util::pitch(point.frequency) - eventList_.meanPitch());
		}
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */
