/***************************************************************************
 *  Copyright 1991, 1992, 1993, 1994, 1995, 1996, 2001, 2002               *
 *    David R. Hill, Leonard Manzara, Craig Schock                         *
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
// 2014-09
// This file was copied from Gnuspeech and modified by Marcelo Y. Matuda.

#include "PhoneticStringParser.h"

#include <cctype> /* isalpha, isdigit, isspace */
#include <fstream>
#include <sstream>

#include "Category.h"
#include "EventList.h"
#include "Exception.h"
#include "Log.h"
#include "Model.h"
#include "Posture.h"



#define CONFIG_DIR "/phonetic_string_parser/"
#define REWRITE_CONFIG_FILE_NAME "rewrite.txt"

namespace {

const char SEPARATOR_CHAR = '>';
const char COMMENT_CHAR = '#';

void
throwException(const std::string& filePath, int lineNumber, const char* message)
{
	THROW_EXCEPTION(GS::ParsingException, "[PhoneticStringParser] Error in file " << filePath << " (line " << lineNumber << "): " << message << '.');
}

template<typename T>
void
throwException(const std::string& filePath, int lineNumber, const char* message, const T& complement)
{
	THROW_EXCEPTION(GS::ParsingException, "[PhoneticStringParser] Error in file " << filePath << " (line " << lineNumber << "): " << message << complement << '.');
}

} /* namespace */

namespace GS {
namespace VTMControlModel {

PhoneticStringParser::PhoneticStringParser(const char* configDirPath, const Model& model, EventList& eventList)
		: model_{model}
		, eventList_{eventList}
{
	std::ostringstream rewriteFilePath;
	rewriteFilePath << configDirPath << CONFIG_DIR REWRITE_CONFIG_FILE_NAME;
	loadRewriterConfiguration(rewriteFilePath.str());
}

PhoneticStringParser::~PhoneticStringParser()
{
}

std::shared_ptr<Category>
PhoneticStringParser::getCategory(const char* name)
{
	const Posture* posture = model_.postureList().find(name);
	if (posture) {
		std::shared_ptr<Category> category = posture->findCategory(name);
		if (!category) {
			THROW_EXCEPTION(UnavailableResourceException, "Could not find the category \"" << name << "\".");
		}
		return category;
	} else {
		std::shared_ptr<Category> category = model_.findCategory(name);
		if (!category) {
			THROW_EXCEPTION(UnavailableResourceException, "Could not find the category \"" << name << "\".");
		}
		return category;
	}
}

const Posture*
PhoneticStringParser::getPosture(const char* name)
{
	const Posture* posture = model_.postureList().find(name);
	if (!posture) {
		THROW_EXCEPTION(UnavailableResourceException, "Could not find the posture \"" << name << "\".");
	}
	return posture;
}

void
PhoneticStringParser::rewrite(const Posture& nextPosture, int wordMarker, RewriterState& state)
{
	if (state.lastPosture == nullptr) {
		state.lastPosture = &nextPosture;
		return;
	}

	for (RewriterData& data : rewriterData_) {
		if (nextPosture.isMemberOfCategory(*data.category2)) {
			// Next posture is in category 2.
			for (RewriterCommand& command : data.commandList) {
				if (state.lastPosture->isMemberOfCategory(*command.category1)) {
					// Last posture is in category 1.
					switch (command.type) {
					case REWRITER_COMMAND_INSERT:
						LOG_DEBUG("REWRITER_COMMAND_INSERT");
						eventList_.newPostureWithObject(*command.posture);
						break;
					case REWRITER_COMMAND_INSERT_IF_WORD_START:
						LOG_DEBUG("REWRITER_COMMAND_INSERT_IF_WORD_START");
						if (wordMarker) {
							eventList_.newPostureWithObject(*command.posture);
						}
						break;
					case REWRITER_COMMAND_REPLACE_FIRST:
						LOG_DEBUG("REWRITER_COMMAND_REPLACE_FIRST");
						eventList_.replaceCurrentPostureWith(*command.posture);
						break;
					case REWRITER_COMMAND_NOP:
						LOG_DEBUG("REWRITER_COMMAND_REPLACE_NOP");
						break;
					}
					state.lastPosture = &nextPosture;
					return;
				}
			}
		}
	}

	state.lastPosture = &nextPosture;
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
	RewriterState rewriterState;

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

			rewrite(*posture, wordMarker, rewriterState);

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

void
PhoneticStringParser::loadRewriterConfiguration(const std::string& filePath)
{
	std::ifstream in(filePath.c_str(), std::ios_base::binary);
	if (!in) THROW_EXCEPTION(IOException, "Could not open the file: " << filePath << '.');
	std::string::iterator iter;
	std::string line;

	auto skipSpaces = [&]() {
		while (iter != line.end() && std::isspace(*iter)) ++iter;
	};
	auto getText = [&]() {
		while (iter != line.end() && !std::isspace(*iter) && *iter != SEPARATOR_CHAR) {
			++iter;
		}
	};

	int lineNum = 0;
	while (getline(in, line)) {
		++lineNum;

		if (line.empty()) continue;

		iter = line.begin();

		// Comment (must start at the beginning of the line).
		if (*iter == COMMENT_CHAR) {
			continue;
		}

		// Read first category.
		skipSpaces();
		auto baseIter = iter;
		getText();
		if (iter == line.end()) throwException(filePath, lineNum, "First category not found");
		std::string firstCategory(baseIter, iter);
		if (firstCategory.empty()) throwException(filePath, lineNum, "Empty first category");

		// Read second category.
		skipSpaces();
		baseIter = iter;
		getText();
		if (iter == line.end()) throwException(filePath, lineNum, "Second category not found");
		std::string secondCategory(baseIter, iter);
		if (secondCategory.empty()) throwException(filePath, lineNum, "Empty second category");

		// Separator.
		skipSpaces();
		if (iter == line.end() || *iter != SEPARATOR_CHAR) throwException(filePath, lineNum, "Missing separator");
		++iter;

		// Read command.
		skipSpaces();
		baseIter = iter;
		getText();
		if (iter == line.end()) throwException(filePath, lineNum, "Command not found");
		std::string commandName(baseIter, iter);
		if (commandName.empty()) throwException(filePath, lineNum, "Empty command");

		// Read posture.
		skipSpaces();
		baseIter = iter;
		getText();
		std::string postureName(baseIter, iter);
		if (postureName.empty()) throwException(filePath, lineNum, "Empty posture");

		std::shared_ptr<Category> cat1 = getCategory(firstCategory.c_str());
		std::shared_ptr<Category> cat2 = getCategory(secondCategory.c_str());
		const Posture* posture = getPosture(postureName.c_str());

		RewriterData* data {};
		for (RewriterData& item : rewriterData_) {
			if (item.category2 == cat2.get()) {
				data = &item;
				break;
			}
		}
		if (data == nullptr) {
			rewriterData_.emplace_back();
			data = &rewriterData_.back();
			data->category2 = cat2.get();
		}

		for (RewriterCommand& item : data->commandList) {
			if (item.category1 == cat1.get()) {
				throwException(filePath, lineNum, "Duplicate category pair");
			}
		}
		data->commandList.emplace_back();
		RewriterCommand* command = &data->commandList.back();
		command->category1 = cat1.get();

		command->posture = posture;

		if (commandName == "insert") {
			command->type = REWRITER_COMMAND_INSERT;
		} else if (commandName == "insert_if_word_start") {
			command->type = REWRITER_COMMAND_INSERT_IF_WORD_START;
		} else if (commandName == "replace_first") {
			command->type = REWRITER_COMMAND_REPLACE_FIRST;
		} else if (commandName == "nop") {
			command->type = REWRITER_COMMAND_NOP;
		} else {
			throwException(filePath, lineNum, "Invalid command", commandName);
		}
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */
