/***************************************************************************
 *  Copyright 1991, 1992, 1993, 1994, 1995, 1996, 2001, 2002               *
 *    David R. Hill, Leonard Manzara, Craig Schock                         *
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
// 2014-09
// This file was copied from Gnuspeech and modified by Marcelo Y. Matuda.

#include "en/phonetic_string_parser/PhoneticStringParser.h"

#include <cctype> /* isalpha, isdigit, isspace */
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "Exception.h"
#include "Log.h"



#define CONFIG_DIR "/phonetic_string_parser/"
#define VOWEL_TRANSITIONS_CONFIG_FILE_NAME "vowel_transitions.txt"



namespace GS {
namespace En {

PhoneticStringParser::PhoneticStringParser(const char* configDirPath, VTMControlModel::Controller& controller)
		: model_(controller.model())
		, eventList_(controller.eventList())
{
	category_[0] = getCategory("stopped");
	category_[1] = getCategory("affricate");
	category_[2] = getCategory("hlike");
	category_[3] = getCategory("vocoid");

	const std::vector<const char*> postureList = {"h", "h'", "hv", "hv'", "ll", "ll'", "s", "s'", "z", "z'"};
	for (unsigned int i = 0; i < postureList.size(); ++i) {
		const char* posture = postureList[i];
		const VTMControlModel::Posture* tempPosture = getPosture(posture);
		category_[i + 4U] = tempPosture->findCategory(posture);
		if (!category_[i + 4U]) {
			THROW_EXCEPTION(UnavailableResourceException, "Could not find the category \"" << posture << "\".");
		}
	}

	category_[14] = getCategory("whistlehack");
	category_[15] = getCategory("lhack");
	category_[16] = getCategory("whistlehack");
	category_[17] = getCategory("whistlehack");

	returnPhone_[0] = getPosture("qc");
	returnPhone_[1] = getPosture("qt");
	returnPhone_[2] = getPosture("qp");
	returnPhone_[3] = getPosture("qk");
	returnPhone_[4] = getPosture("gs");
	returnPhone_[5] = getPosture("qs");
	returnPhone_[6] = getPosture("qz");

	initVowelTransitions(configDirPath);
}

PhoneticStringParser::~PhoneticStringParser()
{
}

std::shared_ptr<VTMControlModel::Category>
PhoneticStringParser::getCategory(const char* name)
{
	const std::shared_ptr<VTMControlModel::Category> category = model_.findCategory(name);
	if (!category) {
		THROW_EXCEPTION(UnavailableResourceException, "Could not find the category \"" << name << "\".");
	}
	return category;
}

const VTMControlModel::Posture*
PhoneticStringParser::getPosture(const char* name)
{
	const VTMControlModel::Posture* posture = model_.postureList().find(name);
	if (!posture) {
		THROW_EXCEPTION(UnavailableResourceException, "Could not find the posture \"" << name << "\".");
	}
	return posture;
}

void
PhoneticStringParser::initVowelTransitions(const char* configDirPath)
{
	char dummy[24], line[256];
	int i = 0;

	memset(vowelTransitions_, 0, 13 * 13 * sizeof(int));
	std::ostringstream path;
	path << configDirPath << CONFIG_DIR VOWEL_TRANSITIONS_CONFIG_FILE_NAME;
	FILE* fp = fopen(path.str().c_str(), "rb");
	if (fp == NULL) {
		THROW_EXCEPTION(IOException, "Could not open the file " << path.str().c_str() << '.');
	}
	while (fgets(line, 256, fp)) {
		if (i == 13) break;

		if ((line[0] == '#') || (line[0] == ' ')) {
			// Skip.
		} else {
			sscanf(line, "%s %d %d %d %d %d %d %d %d %d %d %d %d %d", dummy,
				&vowelTransitions_[i][0], &vowelTransitions_[i][1],  &vowelTransitions_[i][2],
				&vowelTransitions_[i][3], &vowelTransitions_[i][4],  &vowelTransitions_[i][5],
				&vowelTransitions_[i][6], &vowelTransitions_[i][7],  &vowelTransitions_[i][8],
				&vowelTransitions_[i][9], &vowelTransitions_[i][10], &vowelTransitions_[i][11],
				&vowelTransitions_[i][12]);
			i++;

//			int temp = (int) dummy[0];
//			if (dummy[1] != '\'') {
//				temp += (int) dummy[1];
//			}
//			printf("%s %d %d\n", dummy, i, temp);
		}
	}
	fclose(fp);

	if (Log::debugEnabled) {
		printVowelTransitions();
	}
}

void
PhoneticStringParser::printVowelTransitions()
{
	printf("===== Transitions configuration:\n");
	for (int i = 0; i < 13; i++) {
		printf("Transition %d: %d %d %d %d %d %d %d %d %d %d %d %d %d\n", i,
				vowelTransitions_[i][0], vowelTransitions_[i][1], vowelTransitions_[i][2],
				vowelTransitions_[i][3], vowelTransitions_[i][4], vowelTransitions_[i][5],
				vowelTransitions_[i][6], vowelTransitions_[i][7], vowelTransitions_[i][8],
				vowelTransitions_[i][9], vowelTransitions_[i][10], vowelTransitions_[i][11],
				vowelTransitions_[i][12]);
	}
}

const VTMControlModel::Posture*
PhoneticStringParser::rewrite(const VTMControlModel::Posture& nextPosture, int wordMarker, RewriterData& data)
{
	const VTMControlModel::Posture* tempPosture;
	int transitionMade = 0;
	const char* temp;
	const VTMControlModel::Posture* returnValue = nullptr;

	static const int stateTable[19][18] = {
		{ 1,  9,  0,  7,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 0 */
		{ 3,  9,  0,  7,  2,  2,  2,  2,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 1 */
		{ 1,  9,  0,  7,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 2 */
		{ 4,  9,  0,  7,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 3 */
		{ 1,  9,  0,  7,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 4 */
		{ 1,  9,  0,  6,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 5 */
		{ 1,  9,  0,  8,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 6 */
		{ 1,  9,  0,  8,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 7 */
		{ 1,  9,  0,  8,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 8 */
		{10, 12, 12,  0,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 9 */
		{11, 11, 11,  0,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 10 */
		{ 1,  9,  0,  0,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 11 */
		{ 1,  9,  0,  0,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 12 */
		{ 1,  9,  0,  0,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15, 14,  0,  0, 17},		/* State 13 */
		{ 1,  9,  0,  0,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 14 */
		{ 1,  9,  0,  0,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15, 16,  0,  0, 17},		/* State 15 */
		{ 1,  9,  0,  0,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 16 */
		{ 1,  9,  0,  0,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0, 18, 17},		/* State 17 */
		{ 1,  9,  0,  0,  0,  0,  0,  0,  5,  5, 13, 13, 15, 15,  0,  0,  0, 17},		/* State 18 */
	};

	for (int i = 0; i < 18; i++) {
		if (nextPosture.isMemberOfCategory(*category_[i])) {
			//printf("Found %s %s state %d -> %d\n", nextPhone.name().c_str(), category_[i]->name.c_str(),
			//	data.currentState, stateTable[data.currentState][i]);
			data.currentState = stateTable[data.currentState][i];
			transitionMade = 1;
			break;
		}
	}
	if (transitionMade) {
		switch (data.currentState) {
			default:
			case 0:
			case 1:
			case 3:
			case 5:
			case 7:
			case 9:
				//printf("No rewrite\n");
				break;
			case 2:
			case 4:
			case 11:
				temp = data.lastPosture->name().c_str();
				switch (temp[0]) {
					case 'd':
					case 't': returnValue = returnPhone_[1];
						break;
					case 'p':
					case 'b': returnValue = returnPhone_[2];
						break;
					case 'k':
					case 'g': returnValue = returnPhone_[3];
						break;
				}
				break;
			case 6:
				if (strchr(nextPosture.name().c_str(), '\'')) {
					tempPosture = model_.postureList().find("l'");
				} else {
					tempPosture = model_.postureList().find("l");
				}

				eventList_.replaceCurrentPostureWith(*tempPosture);

				break;
			case 8:
				if (wordMarker) {
					returnValue = calcVowelTransition(nextPosture, data);
				}

				break;
			case 10:
				returnValue = returnPhone_[0];
				break;
			case 12:
				returnValue = returnPhone_[0];
				break;
			case 14:
				returnValue = returnPhone_[5];
				break;
			case 16:
				returnValue = returnPhone_[6];
				break;
			case 18:
				//printf("Case 18\n");
				if (!wordMarker) {
					break;
				}

				if (strchr(nextPosture.name().c_str(), '\'')) {
					tempPosture = model_.postureList().find("ll'");
				} else {
					tempPosture = model_.postureList().find("ll");
				}

				//printf("Replacing with ll\n");
				eventList_.replaceCurrentPostureWith(*tempPosture);

				break;
		}
		data.lastPosture = &nextPosture;
	} else {
		data.currentState = 0;
		data.lastPosture = nullptr;
	}
	return returnValue;
}

void
PhoneticStringParser::parse(const char* string, std::size_t size)
{
	std::size_t index = 0;
	std::size_t baseIndex = 0;
	std::string buffer;
	int lastFoot = 0;
	int markedFoot = 0;
	int wordMarker = 0;
	double ruleTempo = 1.0;
	double postureTempo = 1.0;
	RewriterData rewriterData;

	auto skipSeparators = [&]() {
		while ((index < size) && (std::isspace(string[index]) || (string[index] == '_'))) {
			++index;
		}
	};

	auto getNumber = [&]() {
		buffer.clear();
		while ((index < size) && (std::isdigit(string[index]) || (string[index] == '.'))) {
			buffer.push_back(string[index]);
			++index;
		}
	};

	const auto* startEndPosture = model_.postureList().find("^"); // hardcoded
	if (!startEndPosture) {
		THROW_EXCEPTION(MissingValueException, "Posture \"^\" not found.");
	}
	eventList_.newPostureWithObject(*startEndPosture);

	while (index < size) {
		skipSeparators();
		if (index >= size) break;

		switch (string[index]) {
		case '/': /* Handle "/" escape sequences */
			index++;
			switch(string[index]) {
			case '0': /* Tone group 0. Statement */
				index++;
				eventList_.setCurrentToneGroupType(TONE_GROUP_TYPE_STATEMENT);
				break;
			case '1': /* Tone group 1. Exclamation */
				index++;
				eventList_.setCurrentToneGroupType(TONE_GROUP_TYPE_EXCLAMATION);
				break;
			case '2': /* Tone group 2. Question */
				index++;
				eventList_.setCurrentToneGroupType(TONE_GROUP_TYPE_QUESTION);
				break;
			case '3': /* Tone group 3. Continuation */
				index++;
				eventList_.setCurrentToneGroupType(TONE_GROUP_TYPE_CONTINUATION);
				break;
			case '4': /* Tone group 4. Semi-colon */
				index++;
				eventList_.setCurrentToneGroupType(TONE_GROUP_TYPE_SEMICOLON);
				break;
			case '_': /* New foot */
				eventList_.newFoot();
				if (lastFoot) {
					eventList_.setCurrentFootLast();
				}
				lastFoot = 0;
				markedFoot = 0;
				index++;
				break;
			case '*': /* New Marked foot */
				eventList_.newFoot();
				eventList_.setCurrentFootMarked();
				if (lastFoot) {
					eventList_.setCurrentFootLast();
				}
				lastFoot = 0;
				markedFoot = 1;
				index++;
				break;
			case '/': /* New Tone Group */
				index++;
				eventList_.newToneGroup();
				break;
			case 'c': /* New Chunk */
				// Ignore.
				index++;
				break;
			case 'l': /* Last Foot in tone group marker */
				index++;
				lastFoot = 1;
				break;
			case 'w': /* word marker */
				index++;
				wordMarker = 1;
				break;
			case 'f': /* Foot tempo indicator */
				index++;
				baseIndex = index;
				skipSeparators();
				if (index >= size) {
					THROW_EXCEPTION(MissingValueException, "Missing foot tempo value in the phonetic string at index=" << baseIndex << '.');
				}

				baseIndex = index;
				getNumber();
				if (buffer.empty()) {
					THROW_EXCEPTION(MissingValueException, "Missing foot tempo value in the phonetic string at index=" << baseIndex << '.');
				}
				eventList_.setCurrentFootTempo(std::stod(buffer));
				break;
			case 'r': /* Rule tempo indicator */
				index++;
				baseIndex = index;
				skipSeparators();
				if (index >= size) {
					THROW_EXCEPTION(MissingValueException, "Missing rule tempo value in the phonetic string at index=" << baseIndex << '.');
				}

				baseIndex = index;
				getNumber();
				if (buffer.empty()) {
					THROW_EXCEPTION(MissingValueException, "Missing rule tempo value in the phonetic string at index=" << baseIndex << '.');
				}
				ruleTempo = std::stod(buffer);
				break;
			case '"': /* Secondary stress */
				// Ignore.
				index++;
				break;
			default:
				THROW_EXCEPTION(InvalidValueException, "Unknown escape sequence \"/" << string[index] << "\" in the phonetic string at index=" << index - 1 << '.');
			}
			break;
		case '.': /* Syllable Marker */
			eventList_.setCurrentPostureSyllable();
			index++;
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			getNumber();
			postureTempo = std::stod(buffer);
			break;

		default:
			baseIndex = index;
			buffer.clear();
			while ((index < size) &&
					(std::isalpha(string[index]) ||
						(string[index] == '^') || (string[index] == '#'))) { // hardcoded
				buffer.push_back(string[index]);
				++index;
			}
			if (buffer.empty()) {
				THROW_EXCEPTION(MissingValueException, "Missing posture in the phonetic string at index=" << baseIndex << '.');
			}
			if (markedFoot) {
				buffer.push_back('\'');
			}

			const auto* posture = model_.postureList().find(buffer);
			if (!posture) {
				THROW_EXCEPTION(MissingValueException, "Posture \"" << buffer << "\" not found.");
			}

			const auto* posture1 = rewrite(*posture, wordMarker, rewriterData);
			if (posture1) {
				eventList_.newPostureWithObject(*posture1);
			}

			eventList_.newPostureWithObject(*posture);
			eventList_.setCurrentPostureTempo(postureTempo);
			eventList_.setCurrentPostureRuleTempo(ruleTempo);

			postureTempo = 1.0;
			ruleTempo = 1.0;
			wordMarker = 0;

			break;
		}
	}

	const auto* posture = model_.postureList().find("#"); // hardcoded
	if (!posture) {
		THROW_EXCEPTION(MissingValueException, "Posture \"#\" not found.");
	}
	eventList_.newPostureWithObject(*posture);

	eventList_.newPostureWithObject(*startEndPosture);
}

const VTMControlModel::Posture*
PhoneticStringParser::calcVowelTransition(const VTMControlModel::Posture& nextPosture, RewriterData& data)
{
	int vowelHash[13] = { 194, 201, 97, 101, 105, 111, 221, 117, 211, 216, 202, 215, 234 };
	int lastValue, nextValue, i, action;
	const char* temp;

	temp = data.lastPosture->name().c_str();
	lastValue = (int) temp[0];
	if (temp[1] != '\'') {
		lastValue += (int) temp[1];
	}

	for (i = 0; i < 13; i++) {
		if (lastValue == vowelHash[i]) {
			lastValue = i;
			break;
		}
	}
	if (i == 13) {
		return nullptr;
	}

	temp = nextPosture.name().c_str();
	nextValue = (int) temp[0];
	if (temp[1] != '\'') {
		nextValue += (int) temp[1];
	}

	for (i = 0; i < 13; i++) {
		if (nextValue == vowelHash[i]) {
			nextValue = i;
			break;
		}
	}
	if (i == 13) {
		return nullptr;
	}

	action = vowelTransitions_[lastValue][nextValue];

	switch (action) {
	default:
	case 0:
		return nullptr;
	case 1: return model_.postureList().find("gs");
	case 2: return model_.postureList().find("r");
	}
}

} /* namespace En */
} /* namespace GS */
