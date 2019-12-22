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

#ifndef VTM_CONTROL_MODEL_PHONETIC_STRING_PARSER_H_
#define VTM_CONTROL_MODEL_PHONETIC_STRING_PARSER_H_

#include <cstddef> /* std::size_t */
#include <memory>
#include <string>
#include <vector>



namespace GS {
namespace VTMControlModel {

class Category;
class EventList;
class Model;
class Posture;

class PhoneticStringParser {
public:
	PhoneticStringParser(const char* configDirPath, const Model& model, EventList& eventList);
	~PhoneticStringParser() = default;

	void parse(const char* string /* ASCII */, std::size_t size);
private:
	PhoneticStringParser(const PhoneticStringParser&) = delete;
	PhoneticStringParser& operator=(const PhoneticStringParser&) = delete;
	PhoneticStringParser(PhoneticStringParser&&) = delete;
	PhoneticStringParser& operator=(PhoneticStringParser&&) = delete;

	enum class RewriterCommandType {
		nop,
		insert,
		insertIfWordStart,
		replaceFirst
	};

	struct RewriterCommand {
		const Category* category1;
		RewriterCommandType type;
		const Posture* posture;

		RewriterCommand() : category1(), type(RewriterCommandType::nop), posture() {}
	};

	struct RewriterData {
		const Category* category2;
		std::vector<RewriterCommand> commandList;

		RewriterData() : category2() {}
	};

	struct RewriterState {
		const Posture* lastPosture;

		RewriterState() : lastPosture() {}
	};

	void loadRewriterConfiguration(const std::string& filePath);
	void rewrite(const Posture& nextPosture, bool wordMarker, bool marked, RewriterState& state);
	std::shared_ptr<Category> getCategory(const char* name);
	const Posture* getPosture(const char* name);
	static double getTempo(const std::string& s, std::size_t index);

	const Model& model_;
	EventList& eventList_;
	std::vector<RewriterData> rewriterData_;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_PHONETIC_STRING_PARSER_H_ */
