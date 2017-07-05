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

/******************************************************************************
*
*     parser_module.c
*
*     History:
*
*     July 7th, 1992          Completed.
*     December 12th, 1994     Added word begin /w and utterance
*                             boundary # markers.
*     January 5th, 1995       Fixed illegal_slash_code() so that it will
*                             recognize the new /w code when doing raw mode
*                             checking.  The # marker is a phone, so the new
*                             validPhone() function should return this as
*                             valid.  Also changed all closing of streams to
*                             use NX_FREEBUFFER instead of NX_TRUNCATEBUFFER,
*                             eliminating a potential memory leak.  The NeXT
*                             documentation is wrong, since it recommends
*                             using NX_TRUNCATEBUFFER, plus NXGetMemoryBuffer()
*                             and vm_deallocate() calls to free the internal
*                             stream buffer.
*     March 7th, 1995         Fixed bug when using medial punctuation (,;:)
*                             at the end of an utterance.
*
******************************************************************************/

#include "english/EnglishTextParser.h"

#include <cmath>
#include <cctype> /* isprint */
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "Exception.h"
#include "Log.h"
#include "english/LetterToSound.h"



/*  LOCAL DEFINES  ***********************************************************/

#define WORD                  0
#define PUNCTUATION           1
#define PRONUNCIATION         1

#define AND                   "and"
#define PLUS                  "plus"
#define IS_LESS_THAN          "is less than"
#define IS_GREATER_THAN       "is greater than"
#define EQUALS                "equals"
#define MINUS                 "minus"
#define AT                    "at"

#define ABBREVIATION          0
#define EXPANSION             1

#define STATE_UNDEFINED       (-1)
#define STATE_BEGIN           0
#define STATE_WORD            1
#define STATE_MEDIAL_PUNC     2
#define STATE_FINAL_PUNC      3
#define STATE_END             4

#define CHUNK_BOUNDARY        "/c"
#define TONE_GROUP_BOUNDARY   "//"
#define FOOT_BEGIN            "/_"
#define TONIC_BEGIN           "/*"
#define SECONDARY_STRESS      "/\""
#define LAST_WORD             "/l"
#define TAG_BEGIN             "/t"
#define WORD_BEGIN            "/w"
#define UTTERANCE_BOUNDARY    "#"
#define MEDIAL_PAUSE          "^"
#define LONG_MEDIAL_PAUSE     "^ ^ ^"
#define SILENCE_PHONE         "^"

#define TG_UNDEFINED          "/x"
#define TG_STATEMENT          "/0"
#define TG_EXCLAMATION        "/1"
#define TG_QUESTION           "/2"
#define TG_CONTINUATION       "/3"
#define TG_HALF_PERIOD        "/4"

#define UNDEFINED_POSITION    (-1)

#define TTS_FALSE             0
#define TTS_TRUE              1
#define TTS_NO                0
#define TTS_YES               1

#define SYMBOL_LENGTH_MAX     12

#define WORD_LENGTH_MAX       1024
#define SILENCE_MAX           5.0
#define SILENCE_PHONE_LENGTH  0.1     /*  SILENCE PHONE IS 100ms  */

#define DEFAULT_END_PUNC      "."
#define MODE_NEST_MAX         100

#define NON_PHONEME           0
#define PHONEME               1
#define MAX_PHONES_PER_CHUNK  1500
#define MAX_FEET_PER_CHUNK    100

/*  Dictionary Ordering Definitions  */
#define TTS_EMPTY                       0
#define TTS_NUMBER_PARSER               1
#define TTS_DICTIONARY_1                2
#define TTS_DICTIONARY_2                3
#define TTS_DICTIONARY_3                4
#define TTS_LETTER_TO_SOUND             5

#define TTS_PARSER_SUCCESS       (-1)
#define TTS_PARSER_FAILURE       0              /*  OR GREATER THAN 0 IF     */
						/*  POSITION OF ERROR KNOWN  */

#define TEXT_PARSER_DIR "/text_parser/"
#define SUFFIX_LIST_FILE "suffix_list.txt"
#define ABBREVIATIONS_FILE "abbreviations.txt"
#define ABBREVIATIONS_WITH_NUMBER_FILE "abbreviations_with_number.txt"
#define SPECIAL_ACRONYMS_FILE "special_acronyms.txt"



namespace {

using namespace GS::VTMControlModel;

void printStream(std::stringstream& stream, std::size_t streamLength);
void getState(const char* buffer, std::size_t length, std::size_t* i, int* current_state, int* next_state, char* word);
void setToneGroup(std::stringstream& stream, long tg_pos, const char* word);
int anotherWordFollows(const char* buffer, std::size_t length, std::size_t i, TextParser::Mode mode);
int isIsolated(char* buffer, std::size_t len, std::size_t i);
int partOfNumber(char* buffer, std::size_t len, std::size_t i);
int numberFollows(char* buffer, std::size_t len, std::size_t i);
void deleteEllipsis(char* buffer, std::size_t length, std::size_t* i);
int convertDash(char* buffer, std::size_t length, std::size_t* i);
int isTelephoneNumber(char* buffer, std::size_t length, std::size_t i);
int isPunctuation(char c);
int wordFollows(const char* buffer, std::size_t length, std::size_t i, TextParser::Mode mode);
void expandLetterMode(const char* buffer, std::size_t length, std::stringstream& stream);
int isAllUpperCase(const char* word);
char* toLowerCase(char* word);
int containsPrimaryStress(const char* pronunciation);
int convertedStress(char* pronunciation);
int isPossessive(char* word);
void safetyCheck(std::stringstream& stream, std::size_t* streamLength);
void insertChunkMarker(std::stringstream& stream, long insert_point, char tg_type);
void checkTonic(std::stringstream& stream, long start_pos, long end_pos);
void conditionInput(const char* input, std::size_t inputLength, char* output, std::size_t* outputLength);



/******************************************************************************
*
*       function:       print_stream
*
*       purpose:        Prints out the contents of a parser stream
*
******************************************************************************/
void
printStream(std::stringstream& stream, std::size_t streamLength)
{
	printf("stream length = %-ld\n<begin>", streamLength);

	/*  REWIND STREAM TO BEGINNING  */
	stream.seekg(0);

	/*  PRINT LOOP  */
	for (std::size_t i = 0; i < streamLength; i++) {
		char c = stream.get();
		switch (c) {
		case '\0':
			printf("\\0");
			break;
		default:
			printf("%c", c);
			break;
		}
	}
	printf("<end>\n");
}

/******************************************************************************
*
*       function:       get_state
*
*       purpose:        Determines the current state and next state in buffer.
*                       A word or punctuation is put into word.
*
******************************************************************************/
void
getState(const char* buffer, std::size_t length, std::size_t* i,
		int* current_state, int* next_state, char* word)
{
	int k;
	int state = 0;
	int* state_buffer[2];
	std::size_t j;

	/*  PUT STATE POINTERS INTO ARRAY  */
	state_buffer[0] = current_state;
	state_buffer[1] = next_state;

	/*  GET 2 STATES  */
	for (j = *i; j < length && state < 2; j++) { // only need two states

		/*  SKIP WHITE  */
		if (buffer[j] == ' ') {
			continue;
		}

		/*  PUNCTUATION  */
		if (isPunctuation(buffer[j])) {
			if ((buffer[j] == '.') && ((j+1) < length) && isdigit(buffer[j+1])) {
				;  /*  DO NOTHING, HANDLE AS WORD BELOW  */
			} else {
				/*  SET STATE ACCORDING TO PUNCUATION TYPE  */
				switch (buffer[j]) {
				case '.':
				case '!':
				case '?':  *(state_buffer[state]) = STATE_FINAL_PUNC;  break;
				case ';':
				case ':':
				case ',':  *(state_buffer[state]) = STATE_MEDIAL_PUNC;  break;
				}

				/*  PUT PUNCTUATION INTO WORD BUFFER, SET OUTSIDE COUNTER, IN CURRENT STATE  */
				if (state == 0) {
					word[0] = buffer[j];
					word[1] = '\0';
					*i = j;
				}

				/*  INCREMENT STATE  */
				state++;
				continue;
			}
		}

		/*  WORD  */
		if (state == 0) {
			/*  PUT WORD INTO BUFFER  */
			k = 0;
			do {
				word[k++] = buffer[j++];
			} while ((j < length) && (buffer[j] != ' ') && (k < WORD_LENGTH_MAX));
			word[k] = '\0'; j--;

			/*  BACK UP IF WORD ENDS WITH PUNCTUATION  */
			while (k >= 1) {
				if (isPunctuation(word[k-1])) {
					word[--k] = '\0';
					j--;
				} else {
					break;
				}
			}

			/*  SET OUTSIDE COUNTER  */
			*i = j;
		}

		/*  SET STATE TO WORD, INCREMENT STATE  */
		*(state_buffer[state++]) = STATE_WORD;
	}

	/*  IF HERE, THEN END OF INPUT BUFFER, INDICATE END STATE  */
	if (state == 0) {
		/*  SET STATES  */
		*current_state = STATE_END;
		*next_state = STATE_UNDEFINED;
		/*  BLANK OUT WORD BUFFER  */
		word[0] = '\0';
		/*  SET OUTSIDE COUNTER  */
		*i = j;
	} else if (state == 1) {
		*next_state = STATE_END;
	}
}

/******************************************************************************
*
*       function:       set_tone_group
*
*       purpose:        Set the tone group marker according to the punctuation
*                       passed in as "word".  The marker is inserted in the
*                       stream at position "tg_pos".
*
******************************************************************************/
void
setToneGroup(std::stringstream& stream, long tg_pos, const char* word)
{
	/*  RETURN IMMEDIATELY IF tg_pos NOT LEGAL  */
	if (tg_pos == UNDEFINED_POSITION) {
		THROW_EXCEPTION(GS::TextParserException, "Invalid tg_pos.");
	}

	/*  GET CURRENT POSITION IN STREAM  */
	long current_pos = static_cast<long>(stream.tellp());

	/*  SEEK TO TONE GROUP MARKER POSITION  */
	stream.seekp(tg_pos);

	/*  WRITE APPROPRIATE TONE GROUP TYPE  */
	switch (word[0]) {
	case '.':
		stream << TG_STATEMENT;
		break;
	case '!':
		stream << TG_EXCLAMATION;
		break;
	case '?':
		stream << TG_QUESTION;
		break;
	case ',':
		stream << TG_CONTINUATION;
		break;
	case ';':
		stream << TG_HALF_PERIOD;
		break;
	case ':':
		stream << TG_CONTINUATION;
		break;
	default:
		THROW_EXCEPTION(GS::TextParserException, "Invalid character: '" << word[0] << "'.");
	}

	/*  SEEK TO ORIGINAL POSITION ON STREAM  */
	stream.seekp(current_pos);
}

/******************************************************************************
*
*       function:       another_word_follows
*
*       purpose:        Returns 1 if another word follows in buffer, after
*                       position i.  Else, 0 is returned.
*
******************************************************************************/
int
anotherWordFollows(const char* buffer, std::size_t length, std::size_t i, TextParser::Mode mode)
{
	if ((mode == TextParser::MODE_NORMAL) || (mode == TextParser::MODE_EMPHASIS)) {
		for (std::size_t j = i + 1; j < length; j++) {
			/*  WORD HAS BEEN FOUND  */
			if (!isPunctuation(buffer[j])) {
				return 1;
			}
		}
	}

	/*  IF HERE, THEN NO WORD FOLLOWS  */
	return 0;
}

/******************************************************************************
*
*       function:       is_isolated
*
*       purpose:        Returns 1 if character at position i is isolated,
*                       i.e. is surrounded by space.  Returns
*                       0 otherwise.
*
******************************************************************************/
int
isIsolated(char* buffer, std::size_t len, std::size_t i)
{
	if (((i == 0) || ((i > 0) && (buffer[i-1] == ' '))) &&
			((i == (len-1)) || (((i+1) < len) && (buffer[i+1] == ' ')))) {
		return 1;
	} else {
		return 0;
	}
}

/******************************************************************************
*
*       function:       part_of_number
*
*       purpose:        Returns 1 if character at position i is part of
*                       a number (including mixtures with non-numeric
*                       characters).  Returns 0 otherwise.
*
******************************************************************************/
int
partOfNumber(char* buffer, std::size_t len, std::size_t i)
{
	const std::size_t i0 = i;
	while ((i > 0) && (buffer[--i] != ' ')) {
		if (isdigit(buffer[i])) {
			return 1;
		}
	}
	i = i0;
	while ((++i < len) && (buffer[i] != ' ')) {
		if (isdigit(buffer[i])) {
			return 1;
		}
	}

	return 0;
}

/******************************************************************************
*
*       function:       number_follows
*
*       purpose:        Returns a 1 if at least one digit follows the character
*                       at position i, up to white space.
*                       Returns 0 otherwise.
*
******************************************************************************/
int
numberFollows(char* buffer, std::size_t len, std::size_t i)
{
	while ((++i < len) && (buffer[i] != ' ')) {
		if (isdigit(buffer[i])) {
			return 1;
		}
	}

	return 0;
}

/******************************************************************************
*
*       function:       delete_ellipsis
*
*       purpose:        Deletes three dots in a row (disregarding white
*                       space).  If four dots, then the last three are
*                       deleted.
*
******************************************************************************/
void
deleteEllipsis(char* buffer, std::size_t length, std::size_t* i)
{
	/*  SET POSITION OF FIRST DOT  */
	std::size_t pos1 = *i, pos2, pos3;

	/*  IGNORE ANY WHITE SPACE  */
	while (((*i+1) < length) && (buffer[*i+1] == ' ')) {
		(*i)++;
	}
	/*  CHECK FOR 2ND DOT  */
	if (((*i+1) < length) && (buffer[*i+1] == '.')) {
		pos2 = ++(*i);
		/*  IGNORE ANY WHITE SPACE  */
		while (((*i+1) < length) && (buffer[*i+1] == ' ')) {
			(*i)++;
		}
		/*  CHECK FOR 3RD DOT  */
		if (((*i+1) < length) && (buffer[*i+1] == '.')) {
			pos3 = ++(*i);
			/*  IGNORE ANY WHITE SPACE  */
			while (((*i+1) < length) && (buffer[*i+1] == ' ')) {
				(*i)++;
			}
			/*  CHECK FOR 4TH DOT  */
			if (((*i+1) < length) && (buffer[*i+1] == '.')) {
				buffer[pos2] = buffer[pos3] = buffer[++(*i)] = ' ';
			} else {
				buffer[pos1] = buffer[pos2] = buffer[pos3] = ' ';
			}
		}
	}
}

/******************************************************************************
*
*       function:       convert_dash
*
*       purpose:        Converts "--" to ", ", and "---" to ",  "
*                       Returns 1 if this is done, 0 otherwise.
*
******************************************************************************/
int
convertDash(char* buffer, std::size_t length, std::size_t* i)
{
	/*  SET POSITION OF INITIAL DASH  */
	std::size_t pos1 = *i;

	/*  CHECK FOR 2ND DASH  */
	if (((*i+1) < length) && (buffer[*i+1] == '-')) {
		buffer[pos1] = ',';
		buffer[++(*i)] = ' ';
		/*  CHECK FOR 3RD DASH  */
		if (((*i+1) < length) && (buffer[*i+1] == '-'))
			buffer[++(*i)] = ' ';
		return 1;
	}

	/*  RETURN ZERO IF NOT CONVERTED  */
	return 0;
}

/******************************************************************************
*
*       function:       is_telephone_number
*
*       purpose:        Returns 1 if string at position i in buffer is of the
*                       form:  (ddd)ddd-dddd
*                       where each d is a digit.
*
******************************************************************************/
int
isTelephoneNumber(char* buffer, std::size_t length, std::size_t i)
{
	/*  CHECK FORMAT: (ddd)ddd-dddd  */
	if ( ((i+12) < length) &&
			isdigit(buffer[i+1]) && isdigit(buffer[i+2]) && isdigit(buffer[i+3]) &&
			(buffer[i+4] == ')') &&
			isdigit(buffer[i+5]) && isdigit(buffer[i+6]) && isdigit(buffer[i+7]) &&
			(buffer[i+8] == '-') &&
			isdigit(buffer[i+9]) && isdigit(buffer[i+10]) && isdigit(buffer[i+11]) && isdigit(buffer[i+12]) ) {
		/*  MAKE SURE STRING ENDS WITH WHITE SPACE, MODE, OR PUNCTUATION  */
		if ( ((i+13) == length) ||
					( ((i+13) < length) &&
						(isPunctuation(buffer[i+13]) || (buffer[i+13] == ' ')) )
				) {
			/*  RETURN 1 IF ALL ABOVE CONDITIONS ARE MET  */
			return 1;
		}
	}
	/*  IF HERE, THEN STRING IS NOT IN SPECIFIED FORMAT  */
	return 0;
}

/******************************************************************************
*
*       function:       is_punctuation
*
*       purpose:        Returns 1 if character is a .,;:?!
*                       Returns 0 otherwise.
*
******************************************************************************/
int
isPunctuation(char c)
{
	switch (c) {
	case '.':
	case ',':
	case ';':
	case ':':
	case '?':
	case '!':
		return 1;
	default:
		return 0;
	}
}

/******************************************************************************
*
*       function:       word_follows
*
*       purpose:        Returns a 1 if a word or speakable symbol (letter mode)
*                       follows the position i in buffer. Returns a 0 if any
*                       punctuation (except . as part of number) follows.
*
******************************************************************************/
int
wordFollows(const char* buffer, std::size_t length, std::size_t i, TextParser::Mode mode)
{
	switch (mode) {
	case TextParser::MODE_NORMAL:
	case TextParser::MODE_EMPHASIS:
		for (std::size_t j = i + 1; j < length; j++) {
			/*  IGNORE WHITE SPACE  */
			if (buffer[j] == ' ') {
				continue;
			} else if (isPunctuation(buffer[j])) {
				/*  PUNCTUATION MEANS NO WORD FOLLOWS (UNLESS PERIOD PART OF NUMBER)  */
				if ((buffer[j] == '.') && ((j+1) < length) && isdigit(buffer[j+1])) {
					return 1;
				} else {
					return 0;
				}
			} else { /*  ELSE, SOME WORD FOLLOWS  */
				return 1;
			}
		}
		// Falls through.
	case TextParser::MODE_LETTER:
		/*  IF LETTER MODE CONTAINS ANY SYMBOLS, THEN RETURN 1  */
		return 1;
	default:
		return 0;
	}
}

/******************************************************************************
*
*       function:       expand_letter_mode
*
*       purpose:        Expands contents of letter mode string to word or
*                       words.  A comma is added after each expansion, except
*                       the last letter when it is followed by punctuation.
*
******************************************************************************/
void
expandLetterMode(const char* buffer, std::size_t length, std::stringstream& stream)
{
	for (std::size_t i = 0; i < length; ++i) {
		/*  CONVERT LETTER TO WORD OR WORDS  */
		switch (buffer[i]) {
		case ' ': stream << "blank";                break;
		case '!': stream << "exclamation point";    break;
		case '"': stream << "double quote";         break;
		case '#': stream << "number sign";          break;
		case '$': stream << "dollar";               break;
		case '%': stream << "percent";              break;
		case '&': stream << "ampersand";            break;
		case '\'':stream << "single quote";         break;
		case '(': stream << "open parenthesis";     break;
		case ')': stream << "close parenthesis";    break;
		case '*': stream << "asterisk";             break;
		case '+': stream << "plus sign";            break;
		case ',': stream << "comma";                break;
		case '-': stream << "hyphen";               break;
		case '.': stream << "period";               break;
		case '/': stream << "slash";                break;
		case '0': stream << "zero";                 break;
		case '1': stream << "one";                  break;
		case '2': stream << "two";                  break;
		case '3': stream << "three";                break;
		case '4': stream << "four";                 break;
		case '5': stream << "five";                 break;
		case '6': stream << "six";                  break;
		case '7': stream << "seven";                break;
		case '8': stream << "eight";                break;
		case '9': stream << "nine";                 break;
		case ':': stream << "colon";                break;
		case ';': stream << "semicolon";            break;
		case '<': stream << "open angle bracket";   break;
		case '=': stream << "equal sign";           break;
		case '>': stream << "close angle bracket";  break;
		case '?': stream << "question mark";        break;
		case '@': stream << "at sign";              break;
		case 'A':
		case 'a': stream << 'A';                    break;
		case 'B':
		case 'b': stream << 'B';                    break;
		case 'C':
		case 'c': stream << 'C';                    break;
		case 'D':
		case 'd': stream << 'D';                    break;
		case 'E':
		case 'e': stream << 'E';                    break;
		case 'F':
		case 'f': stream << 'F';                    break;
		case 'G':
		case 'g': stream << 'G';                    break;
		case 'H':
		case 'h': stream << 'H';                    break;
		case 'I':
		case 'i': stream << 'I';                    break;
		case 'J':
		case 'j': stream << 'J';                    break;
		case 'K':
		case 'k': stream << 'K';                    break;
		case 'L':
		case 'l': stream << 'L';                    break;
		case 'M':
		case 'm': stream << 'M';                    break;
		case 'N':
		case 'n': stream << 'N';                    break;
		case 'O':
		case 'o': stream << 'O';                    break;
		case 'P':
		case 'p': stream << 'P';                    break;
		case 'Q':
		case 'q': stream << 'Q';                    break;
		case 'R':
		case 'r': stream << 'R';                    break;
		case 'S':
		case 's': stream << 'S';                    break;
		case 'T':
		case 't': stream << 'T';                    break;
		case 'U':
		case 'u': stream << 'U';                    break;
		case 'V':
		case 'v': stream << 'V';                    break;
		case 'W':
		case 'w': stream << 'W';                    break;
		case 'X':
		case 'x': stream << 'X';                    break;
		case 'Y':
		case 'y': stream << 'Y';                    break;
		case 'Z':
		case 'z': stream << 'Z';                    break;
		case '[': stream << "open square bracket";  break;
		case '\\':stream << "back slash";           break;
		case ']': stream << "close square bracket"; break;
		case '^': stream << "caret";                break;
		case '_': stream << "under score";          break;
		case '`': stream << "grave accent";         break;
		case '{': stream << "open brace";           break;
		case '|': stream << "vertical bar";         break;
		case '}': stream << "close brace";          break;
		case '~': stream << "tilde";                break;
		default:  stream << "unknown";              break;
		}
		stream << ' ';
		//stream << ", ";
	}
}

/******************************************************************************
*
*       function:       is_all_upper_case
*
*       purpose:        Returns 1 if all letters of the word are upper case,
*                       0 otherwise.
*
******************************************************************************/
int
isAllUpperCase(const char* word)
{
	while (*word) {
		if (!isupper(*word)) {
			return 0;
		}
		word++;
	}

	return 1;
}

/******************************************************************************
*
*       function:       to_lower_case
*
*       purpose:        Converts any upper case letter in word to lower case.
*
******************************************************************************/
char*
toLowerCase(char* word)
{
	char* ptr = word;

	while (*ptr) {
		if (isupper(*ptr)) {
			*ptr = tolower(*ptr);
		}
		ptr++;
	}

	return word;
}

/******************************************************************************
*
*       function:       contains_primary_stress
*
*       purpose:        Returns 1 if the pronunciation contains '.
*                       Otherwise 0 is returned.
*
******************************************************************************/
int
containsPrimaryStress(const char* pronunciation)
{
	for ( ; *pronunciation && (*pronunciation != '%'); pronunciation++) {
		if (*pronunciation == '\'') {
			return TTS_YES;
		}
	}

	return TTS_NO;
}

/******************************************************************************
*
*       function:       converted_stress
*
*       purpose:        Returns 1 if the first " is converted to a ',
*                       otherwise 0 is returned.
*
******************************************************************************/
int
convertedStress(char* pronunciation)
{
	/*  LOOP THRU PRONUNCIATION UNTIL " FOUND, REPLACE WITH '  */
	for ( ; *pronunciation && (*pronunciation != '%'); pronunciation++) {
		if (*pronunciation == '"') {
			*pronunciation = '\'';
			return TTS_YES;
		}
	}

	/*  IF HERE, NO " FOUND  */
	return TTS_NO;
}

/******************************************************************************
*
*       function:       is_possessive
*
*       purpose:        Returns 1 if 's is found at end of word, and removes
*                       the 's ending from the word.  Otherwise, 0 is returned.
*
******************************************************************************/
int
isPossessive(char* word)
{
	/*  LOOP UNTIL 's FOUND, REPLACE ' WITH NULL  */
	for ( ; *word; word++) {
		if ((*word == '\'') && (*(word+1) == 's') && (*(word+2) == '\0')) {
			*word = '\0';
			return TTS_YES;
		}
	}

	/*  IF HERE, NO 's FOUND, RETURN FAILURE  */
	return TTS_NO;
}

/******************************************************************************
*
*       function:       safety_check
*
*       purpose:        Checks to make sure that there are not too many feet
*                       phones per chunk.  If there are, the input is split
*                       into two or more chunks.
*
******************************************************************************/
void
safetyCheck(std::stringstream& stream, std::size_t* streamLength)
{
	int number_of_feet = 0, number_of_phones = 0, state = NON_PHONEME;
	long last_word_pos = UNDEFINED_POSITION, last_tg_pos = UNDEFINED_POSITION;
	char last_tg_type = '0';
	char c;

	/*  REWIND STREAM TO BEGINNING  */
	stream.seekg(0);

	/*  LOOP THROUGH STREAM, INSERTING NEW CHUNK MARKERS IF NECESSARY  */
	while (stream.get(c) && c != '\0') {
		switch (c) {
		case '%':
			/*  IGNORE SUPER RAW MODE CONTENTS  */
			while (stream.get(c) && c != '%') {
				if (c == '\0') {
					stream.unget();
					break;
				}
			}
			state = NON_PHONEME;
			break;
		case '/':
			/*  SLASH CODES  */
			if (!stream.get(c)) {
				THROW_EXCEPTION(GS::EndOfBufferException, "Could not get a character from the stream.");
			}
			switch (c) {
			case 'c':
				/*  CHUNK MARKER (/c)  */
				number_of_feet = number_of_phones = 0;
				break;
			case '_':
			case '*':
				/*  FOOT AND TONIC FOOT MARKERS  */
				if (++number_of_feet > MAX_FEET_PER_CHUNK) {
					/*  SPLIT STREAM INTO TWO CHUNKS  */
					insertChunkMarker(stream, last_word_pos, last_tg_type);
					setToneGroup(stream, last_tg_pos, ",");
					checkTonic(stream, last_tg_pos, last_word_pos);
				}
				break;
			case 't':
				/*  IGNORE TAGGING MODE CONTENTS  */
				/*  SKIP WHITE  */
				while (stream.get(c) && c == ' ')
					;
				stream.unget();
				/*  SKIP OVER TAG NUMBER  */
				while (stream.get(c) && c != ' ') {
					if (c == '\0') {
						stream.unget();
						break;
					}
				}
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
				/*  REMEMBER TONE GROUP TYPE AND POSITION  */
				last_tg_type = c;
				last_tg_pos = static_cast<long>(stream.tellg()) - 2;
				break;
			default:
				/*  IGNORE ALL OTHER SLASH CODES  */
				break;
			}
			state = NON_PHONEME;
			break;
		case '.':
		case '_':
		case ' ':
			/*  END OF PHONE (AND WORD) DELIMITERS  */
			if (state == PHONEME) {
				if (++number_of_phones > MAX_PHONES_PER_CHUNK) {
					/*  SPLIT STREAM INTO TWO CHUNKS  */
					insertChunkMarker(stream, last_word_pos, last_tg_type);
					setToneGroup(stream, last_tg_pos, ",");
					checkTonic(stream, last_tg_pos, last_word_pos);
					state = NON_PHONEME;
					break;
				}
				if (c == ' ') {
					last_word_pos = static_cast<long>(stream.tellg());
				}
			}
			state = NON_PHONEME;
			break;
		default:
			state = PHONEME;
			break;
		}
	}

	/*  BE SURE TO RESET LENGTH OF STREAM  */
	*streamLength = stream.tellg();
}

/******************************************************************************
*
*       function:       insert_chunk_marker
*
*       purpose:        Insert chunk markers and associated markers in the
*                       stream at the insert point.  Use the tone group type
*                       passed in as an argument.
*
******************************************************************************/
void
insertChunkMarker(std::stringstream& stream, long insert_point, char tg_type)
{
	char c;
	std::stringstream temp_stream;

	/*  COPY STREAM FROM INSERT POINT TO END TO BUFFER TO ANOTHER STREAM  */
	stream.seekg(insert_point);
	while (stream.get(c) && c != '\0') {
		temp_stream << c;
	}
	temp_stream << '\0';

	/*  PUT IN MARKERS AT INSERT POINT  */
	stream.seekp(insert_point);
	stream << TONE_GROUP_BOUNDARY << ' ' << CHUNK_BOUNDARY << ' '
		<< TONE_GROUP_BOUNDARY << " /" << tg_type << ' ';
	long new_position = static_cast<long>(stream.tellp()) - 9;

	/*  APPEND CONTENTS OF TEMPORARY STREAM  */
	temp_stream.seekg(0);
	while (temp_stream.get(c) && c != '\0') {
		stream << c;
	}
	stream << '\0';

	/*  POSITION THE STREAM AT THE NEW /c MARKER  */
	stream.seekp(new_position);
}

/******************************************************************************
*
*       function:       check_tonic
*
*       purpose:        Checks to see if a tonic marker is present in the
*                       stream between the start and end positions.  If no
*                       tonic is present, then put one in at the last foot
*                       marker if it exists.
*
******************************************************************************/
void
checkTonic(std::stringstream& stream, long start_pos, long end_pos)
{
	long last_foot_pos = UNDEFINED_POSITION;

	/*  REMEMBER CURRENT POSITION IN STREAM  */
	const long temp_pos = static_cast<long>(stream.tellp());

	/*  CALCULATE EXTENT OF STREAM TO LOOP THROUGH  */
	long extent = end_pos - start_pos;

	/*  REWIND STREAM TO START POSITION  */
	stream.seekg(start_pos);

	/*  LOOP THROUGH STREAM, DETERMINING LAST FOOT POSITION, AND PRESENCE OF TONIC  */
	char c;
	for (long i = 0; i < extent; i++) {
		if (stream.get(c) && c == '/' && ++i < extent) {
			if (!stream.get(c)) {
				THROW_EXCEPTION(GS::EndOfBufferException, "Could not get a character from the stream.");
			}
			switch (c) {
			case '_':
				last_foot_pos = static_cast<long>(stream.tellg()) - 1;
				break;
			case '*':
				/*  GO TO ORIGINAL POSITION ON STREAM, AND RETURN IMMEDIATELY  */
				//NXSeek(stream, temp_pos, NX_FROMSTART);
				//TODO: check
				return;
			}
		}
	}

	/*  IF HERE, NO TONIC, SO INSERT TONIC MARKER  */
	if (last_foot_pos != UNDEFINED_POSITION) {
		stream.seekp(last_foot_pos);
		stream << '*';
	}

	/*  GO TO ORIGINAL POSITION ON STREAM  */
	stream.seekp(temp_pos);
}

/******************************************************************************
*
*       function:       condition_input
*
*       purpose:        Converts all non-printable characters (except escape
*                       character to blanks.  Also connects words hyphenated
*                       over a newline.
*
******************************************************************************/
void
conditionInput(const char* input, std::size_t inputLength, char* output, std::size_t* outputLength)
{
	std::size_t j = 0;

	for (std::size_t i = 0; i < inputLength; i++) {
		if ((input[i] == '-') && (i > 0) && isalpha(input[i-1])) {
			/*  CONNECT HYPHENATED WORD OVER NEWLINE  */
			std::size_t ii = i;
			/*  IGNORE ANY WHITE SPACE UP TO NEWLINE  */
			while (((ii+1) < inputLength) && (input[ii+1] != '\n') && isspace(input[ii+1])) {
				ii++;
			}
			/*  IF NEWLINE, THEN CONCATENATE WORD  */
			if (((ii+1) < inputLength) && input[ii+1] == '\n') {
				i = ++ii;
				/*  IGNORE ANY WHITE SPACE  */
				while (((i+1) < inputLength) && isspace(input[i+1])) {
					i++;
				}
			} else { /*  ELSE, OUTPUT HYPHEN  */
				output[j++] = input[i];
			}
		//} else if ( !isascii(input[i]) ) {
		//TODO: Complete UTF-8 support.
		// Temporary solution to allow UTF-8 characters.
		} else if (isascii(input[i]) && !isprint(input[i])) {
			/*  CONVERT NONPRINTABLE CHARACTERS TO SPACE  */
			output[j++] = ' ';
		} else {
			/*  PASS EVERYTHING ELSE THROUGH  */
			output[j++] = input[i];
		}
	}

	/*  BE SURE TO APPEND NULL TO STRING  */
	output[j] = '\0';
	*outputLength = j;
}

} /* namespace */

//==============================================================================

namespace GS {
namespace English {

EnglishTextParser::EnglishTextParser(
			const std::string& configDirPath,
			const std::string& dictionary1Path,
			const std::string& dictionary2Path,
			const std::string& dictionary3Path,
			Mode mode)
		: mode_{mode}
{
	std::ostringstream suffixFilePathStream;
	suffixFilePathStream << configDirPath << TEXT_PARSER_DIR SUFFIX_LIST_FILE;
	std::string suffixFilePath = suffixFilePathStream.str();

	if (dictionary1Path != "none") {
		dict1_ = std::make_unique<VTMControlModel::DictionarySearch>();
		std::ostringstream filePath;
		filePath << configDirPath << TEXT_PARSER_DIR << dictionary1Path;
		dict1_->load(filePath.str().c_str(), suffixFilePath.c_str());
	}
	if (dictionary2Path != "none") {
		dict2_ = std::make_unique<VTMControlModel::DictionarySearch>();
		std::ostringstream filePath;
		filePath << configDirPath << TEXT_PARSER_DIR << dictionary2Path;
		dict2_->load(filePath.str().c_str(), suffixFilePath.c_str());
	}
	if (dictionary3Path != "none") {
		dict3_ = std::make_unique<VTMControlModel::DictionarySearch>();
		std::ostringstream filePath;
		filePath << configDirPath << TEXT_PARSER_DIR << dictionary3Path;
		dict3_->load(filePath.str().c_str(), suffixFilePath.c_str());
	}

	dictionaryOrder_[0] = TTS_NUMBER_PARSER;
	dictionaryOrder_[1] = TTS_DICTIONARY_1;
	dictionaryOrder_[2] = TTS_DICTIONARY_2;
	dictionaryOrder_[3] = TTS_DICTIONARY_3;
	dictionaryOrder_[4] = TTS_LETTER_TO_SOUND;
	dictionaryOrder_[5] = TTS_EMPTY;

	std::ostringstream abbrevFilePath;
	abbrevFilePath << configDirPath << TEXT_PARSER_DIR ABBREVIATIONS_FILE;
	abbrevMap_.load(abbrevFilePath.str().c_str());

	std::ostringstream abbrevWithNumberFilePath;
	abbrevWithNumberFilePath << configDirPath << TEXT_PARSER_DIR ABBREVIATIONS_WITH_NUMBER_FILE;
	abbrevWithNumberMap_.load(abbrevWithNumberFilePath.str().c_str());

	std::ostringstream specialAcronymsFilePath;
	specialAcronymsFilePath << configDirPath << TEXT_PARSER_DIR SPECIAL_ACRONYMS_FILE;
	specialAcronymsMap_.load(specialAcronymsFilePath.str().c_str());
}

EnglishTextParser::~EnglishTextParser()
{
}

/******************************************************************************
*
*       function:       parseText
*
*       purpose:        Takes plain english input, and produces phonetic
*                       output suitable for further processing in the TTS
*                       system.  If a parse error occurs, a value of 0 or
*                       above is returned.  Usually this will point to the
*                       position of the error in the input buffer, but in
*                       later stages of the parse only a 0 is returned since
*                       positional information is lost.  If no parser error,
*                       then TTS_PARSER_SUCCESS is returned.
*
******************************************************************************/
std::string
EnglishTextParser::parse(const char* text)
{
	std::size_t buffer_length, stream1_length, stream2_length;

	const std::size_t input_length = strlen(text);

	if (Log::debugEnabled) {
		printf("PHONETIC STRING INPUT [%s]\n", text);
	}

	std::vector<char> buffer(input_length + 1);

	/*  CONDITION INPUT:  CONVERT NON-PRINTABLE CHARS TO SPACES
	    CONNECT WORDS HYPHENATED OVER A NEWLINE  */
	conditionInput(text, input_length, &buffer[0], &buffer_length);

	if (Log::debugEnabled) {
		printf("PHONETIC STRING BUFFER [%s]\n", &buffer[0]);
	}

	std::stringstream stream1;

	/*  STRIP OUT OR CONVERT UNESSENTIAL PUNCTUATION  */
	stripPunctuation(&buffer[0], buffer_length, stream1, &stream1_length);

	if (Log::debugEnabled) {
		printf("PHONETIC STRING STREAM 1\n");
		printStream(stream1, stream1_length);
	}

	std::stringstream stream2;

	/*  DO FINAL CONVERSION  */
	finalConversion(stream1, stream1_length, stream2, &stream2_length);

	/*  DO SAFETY CHECK;  MAKE SURE NOT TOO MANY FEET OR PHONES PER CHUNK  */
	safetyCheck(stream2, &stream2_length);

	if (Log::debugEnabled) {
		printf("PHONETIC STRING STREAM 2\n");
		printStream(stream2, stream2_length);
	}

	std::string phoneticString = stream2.str();
	if (phoneticString.length() < 2) {
		THROW_EXCEPTION(InvalidStateException, "Empty phonetic string.");
	}
	return phoneticString.substr(0, phoneticString.size() - 1); // the last character is '\0'
}

/******************************************************************************
*
*       function:       lookup_word
*
*       purpose:        Returns the pronunciation of word, and sets dict to
*                       the dictionary in which it was found.  Relies on the
*                       global dictionaryOrder.
*
******************************************************************************/
const char*
EnglishTextParser::lookupWord(const char* word)
{
	if (Log::debugEnabled) {
		printf("lookupWord word: [%s]\n", word);
	}

	/*  SEARCH DICTIONARIES IN USER ORDER TILL PRONUNCIATION FOUND  */
	for (int i = 0; i < DICTIONARY_ORDER_SIZE; i++) {
		switch (dictionaryOrder_[i]) {
		case TTS_EMPTY:
			break;
		case TTS_NUMBER_PARSER:
			{
				const char* pron = numberParser_.parse(word, NumberParser::NORMAL);
				if (pron != nullptr) {
					return pron;
				}
			}
			break;
		case TTS_DICTIONARY_1:
			if (dict1_) {
				const char* entry = dict1_->getEntry(word);
				if (entry != nullptr) {
					return entry;
				}
			}
			break;
		case TTS_DICTIONARY_2:
			if (dict2_) {
				const char* entry = dict2_->getEntry(word);
				if (entry != nullptr) {
					return entry;
				}
			}
			break;
		case TTS_DICTIONARY_3:
			if (dict3_) {
				const char* entry = dict3_->getEntry(word);
				if (entry != nullptr) {
					return entry;
				}
			}
			break;
		case TTS_LETTER_TO_SOUND:
			/*  THIS IS GUARANTEED TO FIND A PRONUNCIATION OF SOME SORT  */
			letterToSound(word, pronunciation_);
			if (!pronunciation_.empty()) {
				return &pronunciation_[0];
			} else {
				return numberParser_.degenerateString(word);
			}
			break;
		default:
			break;
		}
	}

	return nullptr;
}

/******************************************************************************
*
*       function:       final_conversion
*
*       purpose:        Converts contents of stream1 to stream2.  Adds chunk,
*                       tone group, and associated markers;  expands words to
*                       pronunciations, and also expands other modes.
*
******************************************************************************/
void
EnglishTextParser::finalConversion(std::stringstream& stream1, std::size_t stream1Length,
				std::stringstream& stream2, std::size_t* stream2Length)
{
	long tg_marker_pos = UNDEFINED_POSITION;
	int prior_tonic = TTS_FALSE;
	int last_written_state = STATE_BEGIN;
	int current_state;
	int next_state;
	char word[WORD_LENGTH_MAX+1];

	/*  REWIND STREAM2 BACK TO BEGINNING  */
	stream2.str("");

	/*  GET MEMORY BUFFER ASSOCIATED WITH STREAM1  */
	std::string stream1String = stream1.str();
	const char* input = stream1String.data();

	/*  MAIN LOOP  */
	for (std::size_t i = 0; i < stream1Length; i++) {

		/*  GET STATE INFORMATION  */
		getState(input, stream1Length, &i, &current_state, &next_state, word);

		/*  ACTION ACCORDING TO CURRENT STATE  */
		switch (current_state) {
		case STATE_WORD:
			/*  ADD BEGINNING MARKERS IF NECESSARY (SWITCH FALL-THRU DESIRED)  */
			switch (last_written_state) {
			case STATE_BEGIN:
				stream2 << CHUNK_BOUNDARY << ' ';
				// Falls through.
			case STATE_FINAL_PUNC:
				stream2 << TONE_GROUP_BOUNDARY << ' ';
				prior_tonic = TTS_FALSE;
				// Falls through.
			case STATE_MEDIAL_PUNC:
				stream2 << TG_UNDEFINED << ' ';
				tg_marker_pos = static_cast<long>(stream2.tellp()) - 3;
				stream2 << UTTERANCE_BOUNDARY << ' ';
			}

			switch (mode_) {
			case MODE_NORMAL:
				/*  PUT IN WORD MARKER  */
				stream2 << WORD_BEGIN << ' ';
				/*  ADD LAST WORD MARKER AND TONICIZATION IF NECESSARY  */
				switch (next_state) {
				case STATE_MEDIAL_PUNC:
				case STATE_FINAL_PUNC:
				case STATE_END:
					/*  PUT IN LAST WORD MARKER  */
					stream2 << LAST_WORD << ' ';
					/*  WRITE WORD TO STREAM WITH TONIC IF NO PRIOR TONICIZATION  */
					expandWord(word, !prior_tonic, stream2);
					break;
				default:
					/*  WRITE WORD TO STREAM WITHOUT TONIC  */
					expandWord(word, TTS_NO, stream2);
					break;
				}
				break;
			case MODE_EMPHASIS:
				/*  START NEW TONE GROUP IF PRIOR TONIC ALREADY SET  */
				if (prior_tonic) {
					setToneGroup(stream2, tg_marker_pos, ",");
					stream2 << TONE_GROUP_BOUNDARY << ' ' << TG_UNDEFINED << ' ';
					tg_marker_pos = static_cast<long>(stream2.tellp()) - 3;
				}
				/*  PUT IN WORD MARKER  */
				stream2 << WORD_BEGIN << ' ';
				/*  MARK LAST WORD OF TONE GROUP, IF NECESSARY  */
				if ((next_state == STATE_MEDIAL_PUNC) ||
						(next_state == STATE_FINAL_PUNC) ||
						(next_state == STATE_END) ||
						(next_state == STATE_WORD)) {
					stream2 << LAST_WORD << ' ';
				}
				/*  TONICIZE WORD  */
				expandWord(word, TTS_YES, stream2);
				prior_tonic = TTS_TRUE;
				break;
			case MODE_LETTER:
				expandWord(word, TTS_NO, stream2);
				break;
			}

			last_written_state = STATE_WORD;
			break;

		case STATE_MEDIAL_PUNC:
			/*  APPEND LAST WORD MARK, PAUSE, TONE GROUP MARK  */
			if (last_written_state == STATE_WORD) {
				if ((next_state != STATE_END) &&
						anotherWordFollows(input, stream1Length, i, mode_)) {
					if (strcmp(word, ",") == 0) {
						stream2 << UTTERANCE_BOUNDARY << ' ' << MEDIAL_PAUSE << ' ';
					} else {
						stream2 << UTTERANCE_BOUNDARY << ' ' << LONG_MEDIAL_PAUSE << ' ';
					}
				} else if (next_state == STATE_END) {
					stream2 << UTTERANCE_BOUNDARY << ' ';
				}
				stream2 << TONE_GROUP_BOUNDARY << ' ';
				prior_tonic = TTS_FALSE;
				setToneGroup(stream2, tg_marker_pos, word);
				tg_marker_pos = UNDEFINED_POSITION;
				last_written_state = STATE_MEDIAL_PUNC;
			}
			break;

		case STATE_FINAL_PUNC:
			if (last_written_state == STATE_WORD) {
				stream2 << UTTERANCE_BOUNDARY << ' '
					<< TONE_GROUP_BOUNDARY << ' ' << CHUNK_BOUNDARY << ' ';
				prior_tonic = TTS_FALSE;
				setToneGroup(stream2, tg_marker_pos, word);
				tg_marker_pos = UNDEFINED_POSITION;
				last_written_state = STATE_FINAL_PUNC;
			}
			break;

		case STATE_END:
			break;
		}
	}

	/*  FINAL STATE  */
	switch (last_written_state) {
	case STATE_MEDIAL_PUNC:
		stream2 << CHUNK_BOUNDARY;
		break;

	case STATE_WORD:
		stream2 << UTTERANCE_BOUNDARY << ' ';
		stream2 << TONE_GROUP_BOUNDARY << ' ' << CHUNK_BOUNDARY;
		setToneGroup(stream2, tg_marker_pos, DEFAULT_END_PUNC);
		break;

	case STATE_BEGIN:
		THROW_EXCEPTION(TextParserException, "Invalid state: STATE_BEGIN.");
		break;
	}

	/*  BE SURE TO ADD NULL TO END OF STREAM  */
	stream2 << '\0';

	/*  SET STREAM2 LENGTH  */
	*stream2Length = stream2.tellp();
}

/******************************************************************************
*
*       function:       expand_word
*
*       purpose:        Write pronunciation of word to stream.  Deal with
*                       possessives if necessary.  Also, deal with single
*                       characters, and upper case words (including special
*                       acronyms) if necessary.  Add special marks if word
*                       is tonic.
*
******************************************************************************/
void
EnglishTextParser::expandWord(char* word, int is_tonic, std::stringstream& stream)
{
	const char* pronunciation;
	int possessive = TTS_NO;
	char last_phoneme[SYMBOL_LENGTH_MAX+1];
	char* last_phoneme_ptr;

	/*  STRIP OF POSSESSIVE ENDING IF WORD ENDS WITH 's, SET FLAG  */
	possessive = isPossessive(word);

	/*  USE degenerate_string IF WORD IS A SINGLE CHARACTER
	    (EXCEPT SMALL, NON-POSSESSIVE A)  */
	if ((strlen(word) == 1) && isalpha(word[0])) {
		if (strcmp(word, "a") == 0 && !possessive) {
			pronunciation = "uh";
		} else {
			pronunciation = numberParser_.degenerateString(word);
		}
	} else if (isAllUpperCase(word)) {
		/*  ALL UPPER CASE WORDS PRONOUNCED ONE LETTER AT A TIME,
		    EXCEPT SPECIAL ACRONYMS  */
		if (!(pronunciation = isSpecialAcronym(word))) {
			pronunciation = numberParser_.degenerateString(word);
		}
	} else { /*  ALL OTHER WORDS ARE LOOKED UP IN DICTIONARIES, AFTER CONVERTING TO LOWER CASE  */
		pronunciation = lookupWord(toLowerCase(word));
	}
	if (!pronunciation) {
		THROW_EXCEPTION(InvalidValueException, "Pronunciation not found for word \"" << word << "\".");
	}

	/*  ADD FOOT BEGIN MARKER TO FRONT OF WORD IF IT HAS NO PRIMARY STRESS AND IT IS
	    TO RECEIVE A TONIC;  IF ONLY A SECONDARY STRESS MARKER, CONVERT TO PRIMARY  */
	long last_foot_begin = UNDEFINED_POSITION;
	if (is_tonic && !containsPrimaryStress(pronunciation)) {
		std::string s{pronunciation};
		if (convertedStress(const_cast<char*>(s.c_str()))) {
			pronunciation = s.c_str();
		} else {
			stream << FOOT_BEGIN;
			last_foot_begin = static_cast<long>(stream.tellp()) - 2;
		}
	}

	/*  PRINT PRONUNCIATION TO STREAM, UP TO WORD TYPE MARKER (%)  */
	/*  KEEP TRACK OF LAST PHONEME  */
	const char* ptr = pronunciation;
	last_phoneme[0] = '\0';
	last_phoneme_ptr = last_phoneme;
	while (*ptr && (*ptr != '%')) {
		switch (*ptr) {
		case '\'':
			stream << FOOT_BEGIN;
			last_foot_begin = static_cast<long>(stream.tellp()) - 2;
			last_phoneme[0] = '\0';
			last_phoneme_ptr = last_phoneme;
			break;
		case '"':
			stream << SECONDARY_STRESS;
			last_phoneme[0] = '\0';
			last_phoneme_ptr = last_phoneme;
			break;
		case '_':
		case '.':
			stream << *ptr;
			last_phoneme[0] = '\0';
			last_phoneme_ptr = last_phoneme;
			break;
		case ' ':
			/*  SUPPRESS UNNECESSARY BLANKS  */
			if (*(ptr+1) && (*(ptr+1) != ' ')) {
				stream << *ptr;
				last_phoneme[0] = '\0';
				last_phoneme_ptr = last_phoneme;
			}
			break;
		default:
			stream << *ptr;
			*last_phoneme_ptr++ = *ptr;
			*last_phoneme_ptr = '\0';
			break;
		}
		ptr++;
	}

	/*  ADD APPROPRIATE ENDING TO PRONUNCIATION IF POSSESSIVE  */
	if (possessive) {
		// hardcoded
		if (strcmp(last_phoneme, "p") == 0 || strcmp(last_phoneme, "t") == 0 ||
				strcmp(last_phoneme, "k") == 0 || strcmp(last_phoneme, "f") == 0 ||
				strcmp(last_phoneme, "th") == 0) {
			stream << "_s";
		} else if (strcmp(last_phoneme, "s") == 0 || strcmp(last_phoneme, "sh") == 0 ||
				strcmp(last_phoneme, "z") == 0 || strcmp(last_phoneme, "zh") == 0 ||
				strcmp(last_phoneme, "j") == 0 || strcmp(last_phoneme, "ch") == 0) {
			stream << ".uh_z";
		} else {
			stream << "_z";
		}
	}

	/*  ADD SPACE AFTER WORD  */
	stream << ' ';

	/*  IF TONIC, CONVERT LAST FOOT MARKER TO TONIC MARKER  */
	if (is_tonic && (last_foot_begin != UNDEFINED_POSITION)) {
		long temporaryPosition = static_cast<long>(stream.tellp());
		stream.seekp(last_foot_begin);
		stream << TONIC_BEGIN;
		stream.seekp(temporaryPosition);
	}
}

/******************************************************************************
*
*       function:       expand_abbreviation
*
*       purpose:        Expands listed abbreviations.  Two lists are used (see
*                       abbreviations.h):  one list expands unconditionally,
*                       the other only if the abbreviation is followed by a
*                       number.  The abbreviation p. is expanded to page.
*                       Single alphabetic characters have periods deleted, but
*                       no expansion is made.  They are also capitalized.
*                       Returns 1 if expansion made (i.e. period is deleted),
*                       0 otherwise.
*
******************************************************************************/
int
EnglishTextParser::expandAbbreviation(char* buffer, std::size_t length, std::size_t i, std::stringstream& stream)
{
	/*  DELETE PERIOD AFTER SINGLE CHARACTER (EXCEPT p.)  */
	if ( (i == 1) || ( (i >= 2) &&
				( (buffer[i-2] == ' ') || (buffer[i-2] == '.') )
				) ) {
		if (isalpha(buffer[i-1])) {
			if ((buffer[i-1] == 'p') && ((i == 1) || ((i >= 2) && (buffer[i-2] != '.')) ) ) {
				/*  EXPAND p. TO page  */
				stream.seekp(-1, std::ios_base::cur);
				stream << "page ";
			} else {
				/*  ELSE, CAPITALIZE CHARACTER IF NECESSARY, BLANK OUT PERIOD  */
				stream.seekp(-1, std::ios_base::cur);
				if (islower(buffer[i-1])) {
					buffer[i-1] = toupper(buffer[i-1]);
				}
				stream << buffer[i-1] << ' ';
			}
			/*  INDICATE ABBREVIATION EXPANDED  */
			return 1;
		}
	}

	std::size_t word_length = 0;

	/*  GET LENGTH OF PRECEDING ISOLATED STRING, UP TO 4 CHARACTERS  */
	for (std::size_t j = 2; j <= 4; j++) {
		if ( (i == j) ||
				((i >= j + 1) && (buffer[i-(j+1)] == ' ')) ) {
			if (isalpha(buffer[i-j]) && isalpha(buffer[i-j+1])) {
				word_length = j;
				break;
			}
		}
	}

	char word[5];

	/*  IS ABBREVIATION ONLY IF WORD LENGTH IS 2, 3, OR 4 CHARACTERS  */
	if ((word_length >= 2) && (word_length <= 4)) {
		/*  GET ABBREVIATION  */
		std::size_t k, j;
		for (k = 0, j = i - word_length; k < word_length; k++) {
			word[k] = buffer[j++];
		}
		word[k] = '\0';

		/*  EXPAND THESE ABBREVIATIONS ONLY IF FOLLOWED BY NUMBER  */
		const char* entry = abbrevWithNumberMap_.getEntry(word);
		if (entry) {
			/*  IGNORE WHITE SPACE  */
			while (((i+1) < length) && (buffer[i+1] == ' ')) {
				i++;
			}
			/*  EXPAND ONLY IF NUMBER FOLLOWS  */
			if (numberFollows(buffer, length, i)) {
				stream.seekp(-word_length, std::ios_base::cur);
				stream << entry << ' ';
				return 1;
			}
		}

		/*  EXPAND THESE ABBREVIATIONS UNCONDITIONALLY  */
		entry = abbrevMap_.getEntry(word);
		if (entry) {
			stream.seekp(-word_length, std::ios_base::cur);
			stream << entry << ' ';
			return 1;
		}
	}

	/*  IF HERE, THEN NO EXPANSION MADE  */
	return 0;
}

/******************************************************************************
*
*       function:       strip_punctuation
*
*       purpose:        Deletes unnecessary punctuation, and converts some
*                       punctuation to another form.
*
******************************************************************************/
void
EnglishTextParser::stripPunctuation(char* buffer, std::size_t length, std::stringstream& stream, std::size_t* streamLength)
{
	/*  DELETE OR CONVERT PUNCTUATION  */

	if ((mode_ == MODE_NORMAL) || (mode_ == MODE_EMPHASIS)) {
		for (std::size_t i = 0; i < length; i++) {
			switch (buffer[i]) {
			case '[':
				buffer[i] = '(';
				break;
			case ']':
				buffer[i] = ')';
				break;
			case '-':
				if (!convertDash(buffer, length, &i) &&
						!numberFollows(buffer, length, i) &&
						!isIsolated(buffer, length, i)) {
					buffer[i] = ' ';
				}
				break;
			case '+':
				if (!partOfNumber(buffer, length, i) && !isIsolated(buffer, length, i)) {
					buffer[i] = ' ';
				}
				break;
			case '\'':
				if (!((i > 0) && isalpha(buffer[i-1]) && ((i+1) < length) && isalpha(buffer[i+1]))) {
					buffer[i] = ' ';
				}
				break;
			case '.':
				deleteEllipsis(buffer, length, &i);
				break;
			case '/':
			case '$':
			case '%':
				if (!partOfNumber(buffer, length, i)) {
					buffer[i] = ' ';
				}
				break;
			case '<':
			case '>':
			case '&':
			case '=':
			case '@':
				if (!isIsolated(buffer, length, i)) {
					buffer[i] = ' ';
				}
				break;
			case '"':
			case '`':
			case '#':
			case '*':
			case '\\':
			case '^':
			case '_':
			case '|':
			case '~':
			case '{':
			case '}':
				buffer[i] = ' ';
				break;
			default:
				break;
			}
		}
	}

	/*  SECOND PASS  */
	stream.str("");
	int status = PUNCTUATION;

	if ((mode_ == MODE_NORMAL) || (mode_ == MODE_EMPHASIS)) {
		for (std::size_t i = 0; i < length; i++) {
			switch (buffer[i]) {
			case '(':
				/*  CONVERT (?) AND (!) TO BLANKS  */
				if ( ((i+2) < length) && (buffer[i+2] == ')') &&
						((buffer[i+1] == '!') || (buffer[i+1] == '?')) ) {
					buffer[i] = buffer[i+1] = buffer[i+2] = ' ';
					stream << "   ";
					i += 2;
					continue;
				}
				/*  ALLOW TELEPHONE NUMBER WITH AREA CODE:  (403)274-3877  */
				if (isTelephoneNumber(buffer, length, i)) {
					int j;
					for (j = 0; j < 12; j++) {
						stream << buffer[i++];
					}
					status = WORD;
					continue;
				}
				/*  CONVERT TO COMMA IF PRECEDED BY WORD, FOLLOWED BY WORD  */
				if ((status == WORD) && wordFollows(buffer, length, i, mode_)) {
					buffer[i] = ' ';
					stream << ", ";
					status = PUNCTUATION;
				} else {
					buffer[i] = ' ';
					stream << ' ';
				}
				break;
			case ')':
				/*  CONVERT TO COMMA IF PRECEDED BY WORD, FOLLOWED BY WORD  */
				if ((status == WORD) && wordFollows(buffer, length, i, mode_)) {
					buffer[i] = ',';
					stream << ", ";
					status = PUNCTUATION;
				} else {
					buffer[i] = ' ';
					stream << ' ';
				}
				break;
			case '&':
				stream << AND;
				status = WORD;
				break;
			case '+':
				if (isIsolated(buffer, length, i)) {
					stream << PLUS;
				} else {
					stream << '+';
				}
				status = WORD;
				break;
			case '<':
				stream << IS_LESS_THAN;
				status = WORD;
				break;
			case '>':
				stream << IS_GREATER_THAN;
				status = WORD;
				break;
			case '=':
				stream << EQUALS;
				status = WORD;
				break;
			case '-':
				if (isIsolated(buffer, length, i)) {
					stream << MINUS;
				} else {
					stream << '-';
				}
				status = WORD;
				break;
			case '@':
				stream << AT;
				status = WORD;
				break;
			case '.':
				if (!expandAbbreviation(buffer, length, i, stream)) {
					stream << buffer[i];
					status = PUNCTUATION;
				}
				break;
			default:
				stream << buffer[i];
				if (isPunctuation(buffer[i])) {
					status = PUNCTUATION;
				} else if (isalnum(buffer[i])) {
					status = WORD;
				}
				break;
			}
		}
	} else if (mode_ == MODE_LETTER) {
		/*  EXPAND LETTER MODE CONTENTS TO PLAIN WORDS OR SINGLE LETTERS  */
		expandLetterMode(buffer, length, stream);
	} else { /*  ELSE PASS CHARACTERS STRAIGHT THROUGH  */
		for (std::size_t i = 0; i < length; i++) {
			stream << buffer[i];
		}
	}

	/*  SET STREAM LENGTH  */
	*streamLength = stream.tellp();
}

/******************************************************************************
*
*       function:       is_special_acronym
*
*       purpose:        Returns a pointer to the pronunciation of a special
*                       acronym if it is defined in the list.  Otherwise,
*                       NULL is returned.
*
******************************************************************************/
const char*
EnglishTextParser::isSpecialAcronym(const char* word)
{
	return specialAcronymsMap_.getEntry(word);
}

} /* namespace English */
} /* namespace GS */
