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

#ifndef TEXT_PARSER_DICTIONARY_SEARCH_H_
#define TEXT_PARSER_DICTIONARY_SEARCH_H_

#include <string>
#include <vector>

#include "Dictionary.h"



namespace GS {
namespace TextParser {

class DictionarySearch {
public:
	DictionarySearch();
	~DictionarySearch() = default;

	void load(const char* dictionaryPath, const char* suffixListPath);

	// The returned string is invalidated if the dictionary is changed.
	const char* getEntry(const char* word);

	// The returned string is invalidated if the dictionary is changed.
	const char* version();
private:
	enum {
		BUF_LEN = 32,
		MAX_LEN = 1024
	};

	struct SuffixInfo {
		std::string suffix;
		std::string replacement;
		std::string pronunciation;
	};

	DictionarySearch(const DictionarySearch&) = delete;
	DictionarySearch& operator=(const DictionarySearch&) = delete;
	DictionarySearch(DictionarySearch&&) = delete;
	DictionarySearch& operator=(DictionarySearch&&) = delete;

	void clearBuffers();
	const char* augmentedSearch(const char* orthography);

	Dictionary dict_;
	std::vector<SuffixInfo> suffixInfoList_;
	std::vector<char> wordTypeBuffer_;
	std::vector<char> buffer_;
};

} /* namespace TextParser */
} /* namespace GS */

#endif /* TEXT_PARSER_DICTIONARY_SEARCH_H_ */
