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

#include "DictionarySearch.h"

#include <cstddef> /* std::size_t */
#include <cstring> /* strcat, strcmp, stdlen, strcpy */
#include <fstream>

#include "Exception.h"
#include "Log.h"



namespace {

const char SUFFIX_SEPARATOR = '|';
const char SUFFIX_COMMENT = '#';
const char* SUFFIX_END_CHARS = " \t#";

/**************************************************************************
*
*       function:   word_has_suffix
*
*       purpose:    Returns position of suffix if word has suffix which
*                   matches, else returns NULL.
*
**************************************************************************/
const char*
wordHasSuffix(const char* word, const char* suffix)
{
	int word_length, suffix_length;
	const char* suffix_position;

	/*  GET LENGTH OF WORD AND SUFFIX  */
	word_length = std::strlen(word);
	suffix_length = std::strlen(suffix);

	/*  DON'T ALLOW SUFFIX TO BE LONGER THAN THE WORD, OR THE WHOLE WORD  */
	if (suffix_length >= word_length) {
		return nullptr;
	}

	/*  FIND POSITION OF SUFFIX IN WORD  */
	suffix_position = word + word_length - suffix_length;

	/*  RETURN SUFFIX POSITION IF THE SUFFIX MATCHES, ELSE RETURN NULL  */
	if (!std::strcmp(suffix_position, suffix)) {
		return suffix_position;
	} else {
		return nullptr;
	}
}

} /* namespace */

//==============================================================================

namespace GS {
namespace TextParser {

DictionarySearch::DictionarySearch()
		: wordTypeBuffer_(BUF_LEN)
		, buffer_(MAX_LEN)
{
	clearBuffers();
}

DictionarySearch::~DictionarySearch()
{
}

void
DictionarySearch::clearBuffers()
{
	wordTypeBuffer_.assign(wordTypeBuffer_.size(), '\0');
	buffer_.assign(buffer_.size(), '\0');
}

void
DictionarySearch::load(const char* dictionaryPath, const char* suffixListPath)
{
	// Load the dictionary.
	dict_.load(dictionaryPath);

	// Load the suffix list.
	std::ifstream in(suffixListPath, std::ios_base::binary);
	if (!in) {
		THROW_EXCEPTION(IOException, "Could not open the file " << suffixListPath << '.');
	}
	std::string line;
	std::size_t lineNumber = 0;
	while (std::getline(in, line)) {
		++lineNumber;
		if (line.empty() || line[0] == SUFFIX_COMMENT) continue;

		std::size_t div1Pos = line.find_first_of(SUFFIX_SEPARATOR);
		if (div1Pos == std::string::npos) {
			THROW_EXCEPTION(IOException, "Could not find a separator (file: " << suffixListPath << " line: " << lineNumber << ").");
		}
		std::size_t div2Pos = line.find_first_of(SUFFIX_SEPARATOR, div1Pos + 1);
		if (div2Pos == std::string::npos) {
			THROW_EXCEPTION(IOException, "Could not find a separator (file: " << suffixListPath << " line: " << lineNumber << ").");
		}

		std::size_t endPos = line.find_first_of(SUFFIX_END_CHARS, div2Pos + 1);
		if (endPos == std::string::npos) {
			endPos = line.size();
		}

		SuffixInfo info;
		info.suffix        = std::string(line.begin()              , line.begin() + div1Pos);
		info.replacement   = std::string(line.begin() + div1Pos + 1, line.begin() + div2Pos);
		info.pronunciation = std::string(line.begin() + div2Pos + 1, line.begin() + endPos);
		if (info.suffix.empty()) {
			THROW_EXCEPTION(IOException, "Empty suffix (file: " << suffixListPath << " line: " << lineNumber << ").");
		}
		LOG_DEBUG("suffix: |" << info.suffix << '|' << info.replacement << '|' << info.pronunciation <<'|');
		suffixInfoList_.push_back(std::move(info));
	}
}

const char*
DictionarySearch::getEntry(const char* word)
{
	return augmentedSearch(word);
}

const char*
DictionarySearch::version()
{
	return dict_.version();
}

/**************************************************************************
*
*       function:   augmented search
*
*       purpose:    First looks in main dictionary to see if word is there.
*                   If not, it tries the main dictionary without suffixes,
*                   and if found, tacks on the appropriate ending.
*
*                   NOTE:  some forms will have to be put in the main
*                   dictionary.  For example, "houses" is NOT pronounced as
*                   "house" + "s", or even "house" + "ez".
*
**************************************************************************/
const char*
DictionarySearch::augmentedSearch(const char* orthography)
{
	const char* word;
	const char* pt;
	char* word_type_pos;

	clearBuffers();

	/*  RETURN IMMEDIATELY IF WORD FOUND IN DICTIONARY  */
	if ( (word = dict_.getEntry(orthography)) ) {
		return word;
	}

	/*  LOOP THROUGH SUFFIX LIST  */
	for (const SuffixInfo& suffixInfo : suffixInfoList_) {
		if ( (pt = wordHasSuffix(orthography, suffixInfo.suffix.c_str())) ) {
			/*  TACK ON REPLACEMENT ENDING  */
			std::strcpy(&buffer_[0], orthography);
			*(&buffer_[0] + (pt - orthography)) = '\0';
			std::strcat(&buffer_[0], suffixInfo.replacement.c_str());

			/*  IF WORD FOUND WITH REPLACEMENT ENDING  */
			if ( (word = dict_.getEntry(&buffer_[0])) ) {
				/*  PUT THE FOUND PRONUNCIATION IN THE BUFFER  */
				std::strcpy(&buffer_[0], word);

				/*  FIND THE WORD-TYPE INFO  */
				for (word_type_pos = &buffer_[0]; *word_type_pos && (*word_type_pos != '%'); word_type_pos++)
					;

				/*  SAVE IT INTO WORD TYPE BUFFER  */
				std::strcpy(&wordTypeBuffer_[0], word_type_pos);

				/*  APPEND SUFFIX PRONUNCIATION TO WORD  */
				*word_type_pos = '\0';
				std::strcat(&buffer_[0], suffixInfo.pronunciation.c_str());

				/*  AND PUT BACK THE WORD TYPE  */
				std::strcat(&buffer_[0], &wordTypeBuffer_[0]);

				/*  RETURN WORD WITH SUFFIX AND ORIGINAL WORD TYPE  */
				return &buffer_[0];
			}
		}
	}

	/*  WORD NOT FOUND, EVEN WITH SUFFIX STRIPPED  */
	return nullptr;
}

} /* namespace TextParser */
} /* namespace GS */
