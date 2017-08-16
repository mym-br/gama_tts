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

#ifndef TEXT_PARSER_ENGLISH_TEXT_PARSER_H_
#define TEXT_PARSER_ENGLISH_TEXT_PARSER_H_

#include <memory>
#include <string>
#include <vector>

#include "DictionarySearch.h"
#include "english/NumberParser.h"
#include "StringMap.h"
#include "TextParser.h"

namespace GS {
namespace TextParser {
namespace English {

class EnglishTextParser : public TextParser {
public:
	EnglishTextParser(
		const std::string& textParserConfigDirPath,
		const TextParserConfiguration& config);
	~EnglishTextParser();

	virtual std::string parse(const char* text);
	virtual void setMode(Mode mode) { mode_ = mode; }
private:
	enum {
		DICTIONARY_ORDER_SIZE = 6
	};

	EnglishTextParser(const EnglishTextParser&) = delete;
	EnglishTextParser& operator=(const EnglishTextParser&) = delete;

	const char* lookupWord(const char* word);
	void expandWord(char* word, int is_tonic, std::stringstream& stream);
	void finalConversion(std::stringstream& stream1, std::size_t stream1Length,
				std::stringstream& stream2, std::size_t* stream2Length);
	int expandAbbreviation(char* buffer, std::size_t length, std::size_t i, std::stringstream& stream);
	void stripPunctuation(char* buffer, std::size_t length, std::stringstream& stream, std::size_t* streamLength);
	const char* isSpecialAcronym(const char* word);

	std::unique_ptr<DictionarySearch> dict1_;
	std::unique_ptr<DictionarySearch> dict2_;
	std::unique_ptr<DictionarySearch> dict3_;
	short dictionaryOrder_[DICTIONARY_ORDER_SIZE];
	std::vector<char> pronunciation_;
	NumberParser numberParser_;
	Mode mode_;
	StringMap abbrevMap_;
	StringMap abbrevWithNumberMap_;
	StringMap specialAcronymsMap_;
};

} /* namespace English */
} /* namespace TextParser */
} /* namespace GS */

#endif /* TEXT_PARSER_ENGLISH_TEXT_PARSER_H_ */
