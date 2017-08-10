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

#include "english/LetterToSound.h"

#include <array>
#include <cstddef> /* std::size_t */
#include <cstdio> /* sprintf */
#include <cstring> /* strcat, strcmp, strcpy, strlen */

#include "Text.h"
#include "english/ApplyStress.h"
#include "english/IspTrans.h"
#include "english/NumberPronunciations.h"
#include "english/Syllabify.h"

/*  LOCAL DEFINES  ***********************************************************/
#define WORD_TYPE_UNKNOWN          "j"
#define WORD_TYPE_DELIMITER        '%'
#define MAX_WORD_LENGTH            1024
#define MAX_PRONUNCIATION_LENGTH   8192
#define SPELL_STRING_LEN   8192



namespace {

using namespace GS;

/*  DATA TYPES  **************************************************************/
typedef struct {
	const char* tail;
	const char* type;
} tail_entry;

/*  GLOBAL VARIABLES (LOCAL TO THIS FILE)  ***********************************/
const tail_entry tail_list[] = {
	{"ly", "d"},
	{"er", "ca"},
	{"ish", "c"},
	{"ing", "cb"},
	{"se", "b"},
	{"ic", "c"},
	{"ify", "b"},
	{"ment", "a"},
	{"al", "c"},
	{"ed", "bc"},
	{"es", "ab"},
	{"ant", "ca"},
	{"ent", "ca"},
	{"ist", "a"},
	{"ism", "a"},
	{"gy", "a"},
	{"ness", "a"},
	{"ous", "c"},
	{"less", "c"},
	{"ful", "c"},
	{"ion", "a"},
	{"able", "c"},
	{"en", "c"},
	{"ry", "ac"},
	{"ey", "c"},
	{"or", "a"},
	{"y", "c"},
	{"us", "a"},
	{"s", "ab"},
	{nullptr, nullptr}
};



bool wordEndsWith(const char* word, const char* string);
const char* wordType(const char* word);
int wordToPatphone(char* word);
int spellIt(char* word);
int allCaps(char* in);
const char* vowelBefore(const char* start, const char* position);
int member(char element, const char* set);
char finalS(char** endOfWord);
int ieToY(char** endOfWord);
int markFinalE(char* in, char** endOfWord);
const char* endsWith(const char* end, const char* set);
const char* suffix(const char* in, const char* end, const char* sufList);
void insertMark(char** end, char* at);
void convertToUppercase(char& c);
void convertToLowercase(char& c);
int longMedialVowels(char* in, char** endOfWord);
void medialSilentE(char* in, char** endOfWord);
void medialS(char* in, char** endOfWord);

const char* letters[] = {
	BLANK, EXCLAMATION_POINT, DOUBLE_QUOTE, NUMBER_SIGN, DOLLAR_SIGN,
	PERCENT_SIGN, AMPERSAND, SINGLE_QUOTE, OPEN_PARENTHESIS, CLOSE_PARENTHESIS,
	ASTERISK, PLUS_SIGN, COMMA, HYPHEN, PERIOD, SLASH,
	ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,
	COLON, SEMICOLON, OPEN_ANGLE_BRACKET, EQUAL_SIGN, CLOSE_ANGLE_BRACKET,
	QUESTION_MARK, AT_SIGN,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	OPEN_SQUARE_BRACKET, BACKSLASH, CLOSE_SQUARE_BRACKET, CARET, UNDERSCORE,
	GRAVE_ACCENT,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	OPEN_BRACE, VERTICAL_BAR, CLOSE_BRACE, TILDE, UNKNOWN
};

const char* finalESuffixList1 =
	"elba/ylba/de/ne/re/yre/tse/ye/gni/ssel/yl/tnem/ssen/ro/luf/";

const char* finalESuffixList2 =
	"ci/laci/";



// Only works if c is an alphabetic ASCII character.
void
convertToUppercase(char& c)
{
	c &= 0xdf;
}

// Only works if c is an alphabetic ASCII character.
void
convertToLowercase(char& c)
{
	c |= 0x20;
}

bool
wordEndsWith(const char* word, const char* string)
{
	const std::size_t wordLen = std::strlen(word);
	const std::size_t stringLen = std::strlen(string);
	if (wordLen < stringLen) return false;
	return std::strcmp(word + wordLen - stringLen, string) == 0;
}

/******************************************************************************
*
*	function:	word_type
*
*	purpose:	Returns the word type based on the word spelling.
*
******************************************************************************/
const char*
wordType(const char* word)
{
	const tail_entry* list_ptr;

	/*  IF WORD END MATCHES LIST, RETURN CORRESPONDING TYPE  */
	for (list_ptr = tail_list; list_ptr->tail; list_ptr++) {
		if (wordEndsWith(word, list_ptr->tail)) {
			return list_ptr->type;
		}
	}

	/*  ELSE RETURN UNKNOWN WORD TYPE  */
	return WORD_TYPE_UNKNOWN;
}

/******************************************************************************
*
*	function:	spell_it
*
*	purpose:
*
*
*       arguments:      word
*
*	internal
*	functions:	none
*
*	library
*	functions:	strcpy
*
******************************************************************************/
int
spellIt(char* word)
{
	std::array<char, SPELL_STRING_LEN> spell_string;

	char* s = &spell_string[0];
	const char* t;
	char* hold = word;

	/*  EAT THE '#'  */
	word++;

	while (*word != '#') {
		if (*word < ' ') {
			if (*word == '\t') {
				t = "'t_aa_b";
			} else {
				t = "'u_p_s";	/* (OOPS!) */
			}
		} else {
			t = letters[*word - ' '];
		}
		word++;
		while (*t) {
			*s++ = *t++;
		}
	}

	*s = '\0';

	std::strcpy(hold, &spell_string[0]);
	return 2;
}

int
allCaps(char* in)
{
	int all_up = 1;
	int force_up = 0;

	in++;
	if (*in == '#') { // empty
		return 1;
	}

	while (*in != '#') {
		if ((*in <= 'z') && (*in >= 'a'))
			all_up = 0;
		else if ((*in <= 'Z') && (*in >= 'A'))
			convertToLowercase(*in);
		else if (*in != '\'')
			force_up = 1;
		in++;
	}
	return all_up || force_up;
}

int
wordToPatphone(char* word)
{
	char replace_s = 0;

	/*  FIND END OF WORD  */
	char* end_of_word = word + 1;
	while (*end_of_word != '#') end_of_word++;

	/*  IF NO LITTLE LETTERS SPELL THE WORD  */
	if (allCaps(word)) {
		return spellIt(word);
	}

	/*  IF SINGLE LETTER, SPELL IT  */
	if (end_of_word == (word + 2)) {
		return spellIt(word);
	}

	/*  IF NO VOWELS SPELL THE WORD  */
	if (!vowelBefore(word, end_of_word)) {
		return spellIt(word);
	}

	/*  KILL ANY TRAILING S  */
	replace_s = finalS(&end_of_word);

	/*  FLIP IE TO Y  */
	ieToY(&end_of_word);

	markFinalE(word, &end_of_word);
	longMedialVowels(word, &end_of_word);
	medialSilentE(word, &end_of_word);
	medialS(word, &end_of_word);

	if (replace_s) {
		*end_of_word++ = replace_s;
		*end_of_word = '#';
	}
	*++end_of_word = 0;
	return 0;
}

/******************************************************************************
*
*       function:     vowel_before
*
*       purpose:      Return the position of a vowel prior to 'position'.
*                     If no vowel prior return 0.
*
******************************************************************************/
const char*
vowelBefore(const char* start, const char* position)
{
	position--;
	while (position >= start) {
		if (member(*position, "aeiouyAEIOUY")) {
			return position;
		}
		position--;
	}
	return nullptr;
}

/******************************************************************************
*
*	function:	member
*
*	purpose:	Return true if element in set, false otherwise.
*
******************************************************************************/
int
member(char element, const char* set)
{
	while (*set) {
		if (element == *set) {
			return 1;
		}
		++set;
	}

	return 0;
}

/******************************************************************************
*
*	function:	final_s
*
*	purpose:	Check for a final s, strip it if found and return s or
*                       z, or else return false.  Don't strip if it's the only
*                       character.
*
******************************************************************************/
char
finalS(char** endOfWord) // the size of the string must be at least 4 ("#cc#")
{
	char* end = *endOfWord;
	char retval = '\0';

	/*  STRIP TRAILING S's  */
	if ((*(end - 1) == '\'') && (*(end - 2) == 's')) {
		*--end = '#';
		*--end = '#';
		*endOfWord = end;

		if (member(*(end - 1), "cfkpt")) {
			retval = 's';
		} else {
			retval = 'z';
		}

		/*  STRIP 'S  */
		if (*(end - 1) == '\'') {
			*--end = '#';
			*endOfWord = end;
		}
	}

	return retval;
}

/******************************************************************************
*
*	function:	ie_to_y
*
*	purpose:	If final two characters are "ie" replace with "y" and
*                       return true.
*
******************************************************************************/
int
ieToY(char** endOfWord)
{
	char* t = *endOfWord;

	if ((*(t - 2) == 'i') && (*(t - 1) == 'e')) {
		*(t - 2) = 'y';
		*(t - 1) = '#';
		*endOfWord = --t;
		return 1;
	}
	return 0;
}

int
markFinalE(char* in, char** endOfWord)
{
	char* end = *endOfWord;
	char* prior_char;
	char* temp;

	/*  McIlroy 4.3 - a)  */
	/*  IF ONLY ONE VOWEL IN WORD && IT IS AN 'e' AT THE END  */
	if ((*(end - 1) == 'e') && !vowelBefore(in, end - 1)) {
		*(end - 1) = 'E';
		return 1;
	}

	/*  McIlroy 4.3 - g)  */
	/*  LOOK FOR #^[aeiouy]* [aeiouy] ^[aeiouywx] [al | le | re | us | y]  */
	/*  IF FOUND CHANGE       ------   TO UPPER CASE */
	if ( (prior_char = const_cast<char*>(endsWith(end, "#la/#el/#er/#su/#y/"))) ) {
		if (!member(*prior_char, "aeiouywx")) {
			if (member(*--prior_char, "aeiouy")) {
				if (!vowelBefore(in, prior_char)) {
					convertToUppercase(*prior_char);
				}
			}
		}
	}

	/* McIlroy 4.3 - a)  */
	temp = prior_char = end - 1;
	while ( (prior_char = const_cast<char*>(suffix(in, prior_char, finalESuffixList1))) ) {
		insertMark(&end, prior_char);
		temp = prior_char;
	}

	prior_char = temp;
	if ( (prior_char = const_cast<char*>(suffix(in, prior_char, finalESuffixList2))) ) {
		insertMark(&end, prior_char);
		*endOfWord = end;
		return 0;
	}

	prior_char = temp;
	if ( (prior_char = const_cast<char*>(suffix(in, prior_char, "e/"))) ) {
		if (prior_char[2] != 'e') {
			if (prior_char[2] != '|') {
				insertMark(&end, prior_char);
			}
		} else {
			*endOfWord = end;
			return 0;
		}
	} else {
		prior_char = temp;
	}

	/*  McIlroy 4.3 -b)  */
	if (((prior_char[1] == '|') && (member(prior_char[2], "aeio")))
			|| (member(prior_char[1], "aeio"))) {
		if (!member(*prior_char, "aeiouywx")) {
			if (member(*(prior_char - 1), "aeiouy")) {
				if (!member(*(prior_char - 2), "aeo")) {
					convertToUppercase(*(prior_char - 1));
				}
			}
		}

		/*  McIlroy 4.3 -c)  */
		if ((*prior_char == 'h') && (*(prior_char - 1) == 't')) {
			if (member(*(prior_char - 2), "aeiouy")) {
				if (!member(*(prior_char - 3), "aeo")) {
					convertToUppercase(*(prior_char - 2));
				}
				*(prior_char - 1) = 'T';
				*prior_char = 'H';
			}
		}
	}

	/*  McIlroy 4.3 - d)  */
	if ((member(*prior_char, "iuy")) && !vowelBefore(in, prior_char)) {
		convertToUppercase(*prior_char);
		*endOfWord = end;
		return 0;
	}

	/*  McIlroy 4.3 - e)  */
	if ((*(prior_char + 1) == 'e') && (member(*prior_char, "cg"))) {
		temp = const_cast<char*>(vowelBefore(in, prior_char));
		if (vowelBefore(in, temp)) {
			convertToLowercase(*temp);
			*endOfWord = end;
			return 0;
		}
	}

	/*  McIlroy 4.3 - f)  */
	if ((*prior_char == 'l') && (*(prior_char - 1) == 'E')) {
		convertToLowercase(*(prior_char - 1));
	}
	*endOfWord = end;

	return 0;
}

/******************************************************************************
*
*	function:	ends_with
*
*	purpose:	Return 0 if word doesn't end with set element, else
*                       pointer to char before ending.
*
******************************************************************************/
// Example of set: "#la/#el/#er/#su/#y/" (the suffixes are reversed).
const char*
endsWith(const char* end, const char* set)
{
	const char* temp;

	while (*set) {
		temp = end + 1;
		while (*--temp == *set) {
			set++;
		}
		if (*set == '/') {
			return temp;
		}
		while (*set++ != '/');
	}
	return nullptr;
}

/******************************************************************************
*
*	function:	suffix
*
*	purpose:	Find suffix if vowel in word before the suffix.
*                       Return 0 if failed, or pointer to character which
*			preceeds the suffix.
*
******************************************************************************/
const char*
suffix(const char* in, const char* end, const char* sufList)
{
	const char* temp = endsWith(end, sufList);
	if (temp && vowelBefore(in, temp + 1)) {
		return temp;
	}
	return nullptr;
}

void
insertMark(char** end, char* at)
{
	char* temp = *end;

	at++;
	if (*at == 'e') at++;
	if (*at == '|') return;

	while (temp >= at) {
		temp[1] = *temp;
		--temp;
	}

	*at = '|';
	++(*end);
}

int
longMedialVowels(char* in, char** endOfWord)
{
	char* end = *endOfWord;
	char* position;

	/*  McIlroy 4.4 - a  */
	for (position = in; position < end - 3; position++) {
		if (member(position[0], "aeiou")) {
			continue;
		}
		if (position[1] != 'u') {
			continue;
		}
		if (member(position[2], "aeiouwxy|")) {
			continue;
		}
		if (member(Text::toLower(position[3]), "aeiouy")) {
			convertToUppercase(position[1]);
			continue;
		}
		if ((!(member(position[2], "bcdfgkpt")) || (position[3] != 'r'))) {
			continue;
		}
		if (member(Text::toLower(position[4]), "aeiouy")) {
			convertToUppercase(position[1]);
		}
		/*  TO FIX cupric WE HAVE TO CHECK FOR |vowel HERE  */
	}

	/*  McIlroy 4.4 b, b  */
	for (position = in; position < end - 3; position++) {
		if (!member(position[0], "aeo")) {
			continue;
		}
		if (member(position[1], "aehiouwxy")) {
			continue;
		}
		if ((position[2] == 'h') && (position[1] == 't')) {
			if (((member(position[3], "ie")) && (member(Text::toLower(position[4]), "aou")))
					|| ((position[3] == 'i') && (position[4] == 'e') && (position[5] == 'n'))) {
				convertToUppercase(*position);
			}
			continue;
		}
		if (member(position[1], "bcdfgkpt")) {
			if ((position[2] == 'r') && (position[3] == 'i'))
				if (member(Text::toLower(position[4]), "aou")) {
					convertToUppercase(*position);
					continue;
				}
		}
		if (((member(position[2], "ie")) && (member(Text::toLower(position[3]), "aou")))
				|| ((position[2] == 'i') && (position[3] == 'e') && (position[4] == 'n'))) {
			convertToUppercase(*position);
		}
	}

	/*  McIlroy 4.4 - c  */
	position = in;
	while (!member(Text::toLower(*position), "aeiouy") && (position < end)) {
		position++;
	}
	if (position == end) {
		return 0;
	}
	if ((member(Text::toLower(position[1]), "aou"))
			&& ((*position == 'i') || ((*position == 'y') && (position + 1 > in)))) {
		convertToUppercase(*position);
	}

	return 0;
}

void
medialSilentE(char* in, char** endOfWord)
{
	char* end = *endOfWord;
	char* position;
	int index;

	for (position = in + 2; position < end - 5; position++) {
		if (!member(position[0], "bcdfgmnprst")) {
			continue;		/* c */
		}
		if (!member(position[1], "bdfgkpt")) {
			continue;		/* k */
		}
		if ((position[2] != 'l') || (position[3] != 'e')) {
			continue;		/* le */
		}
		if (member(Text::toLower(position[4]), "aeiouy")) {
			continue;		/* s */
		}
		if (position[4] == '|') {
			continue;
		}

		index = 5;
		while (!member(Text::toLower(position[index]), "aeiouy|")) {	/* he */
			index++;
			if (&position[index] >= end) {
				index = 0;
				break;
			}
		}

		if (!index) continue;
		if (position[index] == '|') {
			continue;
		}
		if ((position[index] == 'e') && (position[index + 1] == '|')) {
			continue;
		}
		insertMark(&end, &position[3]);
		break;
	}

	for (position = in; position < end - 5; position++) {
		if ((member(position[0], "aeiou#"))) {
			continue;
		}
		if (!member(position[1], "aiouy")) {
			continue;
		}
		if (member(Text::toLower(position[2]), "aehiouwxy")) {
			continue;
		}
		if (position[3] != 'e') {
			continue;
		}
		if (member(Text::toLower(position[4]), "aeiouynr")) {
			continue;
		}

		index = 5;
		if ((position[index] == '|') ||
				((position[index] == 'e') && (position[++index] == '|'))) {
			continue;
		}
		index++;
		if (!member(Text::toLower(position[index]), "aeiouy")) {
			continue;
		}
		insertMark(&end, &position[3]);
		convertToUppercase(position[1]);
		break;
	}

	for (position = in + 1; position < end - 5; position++) {
		if (position[0] != 'o') {
			continue;
		}
		if (!member(position[1], "aiouyU")) {
			continue;
		}
		if (member(Text::toLower(position[2]), "aehiouwxy")) {
			continue;
		}
		if (position[3] != 'e') {
			continue;
		}
		if (member(Text::toLower(position[4]), "aeiouynr")) {
			continue;
		}
		index = 5;
		if ((position[index] == '|') ||
				((position[index] == 'e') && (position[++index] == '|'))) {
			continue;
		}
		index++;
		if (!member(Text::toLower(position[index]), "aeiouy")) {
			continue;
		}
		insertMark(&end, &position[3]);
		break;
	}
	*endOfWord = end;
}

void
medialS(char* in, char** eow)
{
	char* end = *eow;

	while (in < end - 1) {
		if ((member(Text::toLower(*in), "aeiouy")) && (in[1] == 's')
				&& (member(in[2], "AEIOUYaeiouym"))) {
			convertToUppercase(in[1]);
		}
		in++;
	}
}

} /* namespace */

//==============================================================================

namespace GS {
namespace English {

/******************************************************************************
*
*	function:	letter_to_sound
*
*	purpose:	Returns pronunciation of word based on letter-to-sound
*                       rules.  Returns NULL if any error (rare).
*
******************************************************************************/
void
letterToSound(const char* word, std::vector<char>& pronunciation)
{
	char buffer[MAX_WORD_LENGTH + 3];
	int number_of_syllables = 0;

	pronunciation.assign(MAX_PRONUNCIATION_LENGTH + 1, '\0');

	/*  FORMAT WORD  */
	std::sprintf(buffer, "#%s#", word);

	/*  CONVERT WORD TO PRONUNCIATION  */
	if (!wordToPatphone(buffer)) {
		ispTrans(buffer, &pronunciation[0]);
		/*  ATTEMPT TO MARK SYLL/STRESS  */
		number_of_syllables = syllabify(&pronunciation[0]);
		if (applyStress(&pronunciation[0], word)) {
			// Error.
			pronunciation.clear();
			return;
		}
	} else {
		std::strcpy(&pronunciation[0], buffer);
	}

	/*  APPEND WORD_TYPE_DELIMITER  */
	pronunciation[std::strlen(&pronunciation[0]) - 1] = WORD_TYPE_DELIMITER;

	/*  GUESS TYPE OF WORD  */
	if (number_of_syllables != 1) {
		std::strcat(&pronunciation[0], wordType(word));
	} else {
		std::strcat(&pronunciation[0], WORD_TYPE_UNKNOWN);
	}
}

} /* namespace English */
} /* namespace GS */
