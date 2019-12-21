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
*	Program Module:		number_parser.c
*
*	Programmer:		Leonard Manzara
*
*	Date of completion:	September 9th, 1990
*
*       Last Edit:              February 21st, 1992.  Added seconds to clock
*                                 times, o'clock on the hour.
*
*******************************************************************************
*
*       number_parser.c, in conjunction with number_parser.h, is used to create
*       the function number_parser().  This function can be used to return to
*       the caller the pronunciation of any string containing a numeral.  The
*       calling routine must include number_parser.h to access number_parser()
*       (and also degenerate_string(), another useful function).
*       The include file number_pronunciations.h contains pronunciations for
*       all numbers and symbols; it can be changed as needed to improve
*       the pronunciations.  number_parser.c must be recompiled if any of the
*       header files change.
*
*       number_parser() has two arguments:
*       1) word_ptr - a pointer to a NULL terminated character string,
*          containing the number or number string to be parsed.
*       2) mode - an integer, specifying in what mode the number or
*          number string is to be parsed.  Three constants are defined
*          in number_parser.h:  NP_NORMAL, NP_OVERRIDE_YEARS, and
*          NP_FORCE_SPELL.  Using NP_NORMAL, the function attempts
*          to parse the number string according to the guidelines
*          listed below.  NP_OVERRIDE_YEARS forces the function to
*          interpret numbers from 1000 to 1999 NOT as years, but as
*          ordinary integers.  NP_FORCE_SPELL forces the function to
*          spell out each character of the number string.  This may
*          be useful with very long numbers, or when the numbers are
*          not to be interpreted in the usual way.
*
*       number_parser() returns a pointer to NULL if the word_ptr points
*       to a string which contains NO numerals at all.  It returns a pointer
*       to a NULL terminated character string containing the pronunciation
*       for the number string in all other cases.
*
*       The parser can deal with the following cases:
*       1) Cardinal numbers.
*          a) Will name triads with numbers up to 10^63 (vigintillion);  with
*             numbers longer than this the numerals are pronounced one at a
*             time.  Eg: 1547630 is pronounced as one million, five hundred
*             and forty-seven thousand, six hundred and thirty.
*          b) Cardinal numbers can be with or without commas.  The function
*             checks that the commas are properly placed.  Eg: 23,567 is
*             pronounced as twenty-three thousand, five hundred and sixty-
*             seven.
*       2) Positive and negative numbers.  A + or - sign can be placed before
*          most numbers, except before telephone numbers, clock times, or
*          years.  Eg:  -34.5 is pronounced as negative thirty-four point five.
*       3) Decimal numbers.  All numbers with a decimal point can be
*          pronounced.  Eg:  +34.234 is pronounced as positive thirty-four
*          point two three four.
*       4) Simple fractions.  Number strings with the form  integer/integer
*          are pronounced correctly.  Each integer must NOT contain commas
*          or decimals.  Any + or - sign must precede the first integer,
*          and any % sign must follow the last integer.  Eg:  -3/4% is
*          pronounced as negative three quarters percent.
*       5) Ordinal numbers.  Ordinal numbers up to 10^63 are pronounced
*          correctly, provided the proper suffix (-st, -nd, or -th) is
*          provided.  Eg:  101ST is pronounced as one hundred and first.
*       6) Dollars and cents.  Dollars and cents are pronounced correctly
*          if the $ sign is placed before the number.  An optional + or -
*          sign can be placed before the dollar sign.  Eg:  -$2.01 is
*          pronounced as negative two dollars and one cent.
*       7) Percent.  If a % sign is placed after the number, the word
*          "percent" is also pronounced.  Eg:  2.45% is pronounced as
*          two point four five percent.
*       8) Telephone numbers.  The parser recognizes the following types of
*          telephone numbers:
*          a) 7 digit code.  Eg:  555-2345.
*          b) 10 digit code.  Eg:  203-555-2345
*          c) 11 digit code.  Eg:  1-800-555-2345
*          d) area codes.  Eg:  (203) 555-2345  or  (203)555-2345
*               (Note the optional space above.)
*       9) Clock times.  The function recognizes both normal and military
*          (24 hour) time.  Eg:  9:31 is pronounced nine thirty-one.
*          08:00 is pronounced oh eight hundred.  Seconds are also recognized.
*          Eg. 10:23:14 is pronounced ten twenty-three and 14 seconds.  Non-
*          military times on the hour have o'clock appended.  Eg. 9:00 is
*          pronounced nine o'clock.
*       10) Years.  Integers from 1000 to 1999 are pronounced as two pairs.
*           Eg:  1906 is pronounced as nineteen oh six, NOT one thousand,
*           nine hundred and six.  This default can be changed by setting
*           the mode to NP_OVERRIDE_YEARS.
*
*       If the function cannot put the number string it receives into
*       any of the above cases, it will pronounce the string one character
*       at a time.  If the calling routine wishes to have character-by-
*       character pronunciation as the default, the mode should be set to
*       NP_FORCE_SPELL.  Using the function degenerate_string() will also
*       achieve the same thing.
*
*******************************************************************************
*
*	LIST OF INTERNAL AND LIBRARY FUNCTIONS USED IN THE PROGRAM MODULE
*
*	Internal functions:	                number_parser
*                                               process_word
*                                               initial_parse
*                                               error_check
*                                               degenerate_string
*                                               process_triad
*                                               process_digit
*						
*
*	Library functions:	<string.h>	strlen
*                                               strcmp
*                               <stdlib.h>      atoi
*
******************************************************************************/

#include <cstdlib>
#include <cstring>

#include "english/NumberParser.h"
#include "english/NumberPronunciations.h"
/*  #include "NumberPronunciations_english.h"  (use this for plain english)  */

/*  SYMBOLIC CONSTANTS  ******************************************************/

#define NO                     0       /*  GENERAL PURPOSE FLAGS  */
#define YES                    1
#define NONZERO                1

#define NO_NUMERALS            0       /*  FLAGS RETURNED BY error_check()  */
#define DEGENERATE             1
#define OK                     3

#define SECONDTH_FLAG          1       /*  FLAGS FOR special_flag  */
#define HALF_FLAG              2
#define QUARTER_FLAG           3

#define SEVEN_DIGIT_CODE       1       /*  TELEPHONE FLAGS  */
#define TEN_DIGIT_CODE         2
#define ELEVEN_DIGIT_CODE      3
#define AREA_CODE              4



namespace {

/*  VARIABLES PERTAINING TO TRIADS AND TRIAD NAMES  */
const char* triad_name[3][TRIADS_MAX] = {
	{NULL_STRING, THOUSAND, MILLION, BILLION, TRILLION, QUADRILLION, QUINTILLION,
	 SEXTILLION, SEPTILLION, OCTILLION, NONILLION, DECILLION, UNDECILLION,
	 DUODECILLION, TREDECILLION, QUATTUORDECILLION, QUINDECILLION, SEXDECILLION,
	 SEPTENDECILLION, OCTODECILLION, NOVEMDECILLION, VIGINTILLION},
	{NULL_STRING, THOUSANDTH, MILLIONTH, BILLIONTH, TRILLIONTH, QUADRILLIONTH,
	 QUINTILLIONTH, SEXTILLIONTH, SEPTILLIONTH, OCTILLIONTH, NONILLIONTH,
	 DECILLIONTH, UNDECILLIONTH, DUODECILLIONTH, TREDECILLIONTH,
	 QUATTUORDECILLIONTH, QUINDECILLIONTH, SEXDECILLIONTH, SEPTENDECILLIONTH,
	 OCTODECILLIONTH, NOVEMDECILLIONTH, VIGINTILLIONTH},
	{NULL_STRING, THOUSANDTHS, MILLIONTHS, BILLIONTHS, TRILLIONTHS,
	 QUADRILLIONTHS, QUINTILLIONTHS, SEXTILLIONTHS, SEPTILLIONTHS, OCTILLIONTHS,
	 NONILLIONTHS, DECILLIONTHS, UNDECILLIONTHS, DUODECILLIONTHS,
	 TREDECILLIONTHS, QUATTUORDECILLIONTHS, QUINDECILLIONTHS, SEXDECILLIONTHS,
	 SEPTENDECILLIONTHS, OCTODECILLIONTHS, NOVEMDECILLIONTHS, VIGINTILLIONTHS}
};

} /* namespace */

//==============================================================================

namespace GS {
namespace TextParser {
namespace English {

NumberParser::NumberParser()
		: word_()
		, wordLength_()
		, degenerate_()
		, integerDigits_()
		, fractionalDigits_()
		, commas_()
		, decimal_()
		, dollar_()
		, percent_()
		, negative_()
		, positive_()
		, ordinal_()
		, clock_()
		, slash_()
		, leftParen_()
		, rightParen_()
		, blank_()
		, dollarPlural_()
		, dollarNonzero_()
		, centsPlural_()
		, centsNonzero_()
		, telephone_()
		, leftZeroPad_()
		, rightZeroPad_()
		, ordinalPlural_()
		, fracLeftZeroPad_()
		, fracRightZeroPad_()
		, fracOrdinalTriad_()
		, decimalPos_()
		, dollarPos_()
		, percentPos_()
		, positivePos_()
		, leftParenPos_()
		, rightParenPos_()
		, blankPos_()
		, ordinalTriad_()
		, military_()
		, seconds_()
{
	commasPos_.fill(0);
	negativePos_.fill(0);
	integerDigitsPos_.fill(0);
	fractionalDigitsPos_.fill(0);
	ordinalPos_.fill(0);
	clockPos_.fill(0);
	triad_.fill('\0');
	ordinalBuffer_.fill('\0');
	hour_.fill('\0');
	minute_.fill('\0');
	second_.fill('\0');
}

NumberParser::~NumberParser()
{
}

/******************************************************************************
*
*	function:	process_digit
*
*	purpose:	Appends to output the pronunciation for the input
*                       digit.  If the special_flag is set, the appropriate
*                       special pronunciation is used.  If the ordinal_plural
*                       flag is set, the plural ordinal pronunciations are
*                       used.  If the ordinal flag is set, ordinal
*                       pronunciations are used.  Otherwise standard digit
*                       pronunciations are used.
*
******************************************************************************/
void
NumberParser::processDigit(char digit, int ordinal, int ordinal_plural, int special_flag)
{
	/*  DO SPECIAL PROCESSING IF FLAG SET  */
	if (special_flag == HALF_FLAG) {
		if (ordinal_plural) {
			appendToOutput(HALVES);
		} else {
			appendToOutput(HALF);
		}
	} else if (special_flag == SECONDTH_FLAG) {
		if (ordinal_plural) {
			appendToOutput(SECONDTHS);
		} else {
			appendToOutput(SECONDTH);
		}
	} else if (special_flag == QUARTER_FLAG) {
		if (ordinal_plural) {
			appendToOutput(QUARTERS);
		} else {
			appendToOutput(QUARTER);
		}
	} else if (ordinal_plural) {
		/*  DO PLURAL ORDINALS  */
		switch (digit) {
		case '3': appendToOutput(THIRDS);   break;
		case '4': appendToOutput(FOURTHS);  break;
		case '5': appendToOutput(FIFTHS);   break;
		case '6': appendToOutput(SIXTHS);   break;
		case '7': appendToOutput(SEVENTHS); break;
		case '8': appendToOutput(EIGHTHS);  break;
		case '9': appendToOutput(NINTHS);   break;
		}
	} else if (ordinal) {
		/*  DO SINGULAR ORDINALS  */
		switch (digit) {
		case '0': appendToOutput(ZEROETH); break;
		case '1': appendToOutput(FIRST);   break;
		case '2': appendToOutput(SECOND);  break;
		case '3': appendToOutput(THIRD);   break;
		case '4': appendToOutput(FOURTH);  break;
		case '5': appendToOutput(FIFTH);   break;
		case '6': appendToOutput(SIXTH);   break;
		case '7': appendToOutput(SEVENTH); break;
		case '8': appendToOutput(EIGHTH);  break;
		case '9': appendToOutput(NINTH);   break;
		}
	} else {
		/*  DO ORDINARY DIGITS  */
		switch (digit) {
		case '0': appendToOutput(ZERO);  break;
		case '1': appendToOutput(ONE);   break;
		case '2': appendToOutput(TWO);   break;
		case '3': appendToOutput(THREE); break;
		case '4': appendToOutput(FOUR);  break;
		case '5': appendToOutput(FIVE);  break;
		case '6': appendToOutput(SIX);   break;
		case '7': appendToOutput(SEVEN); break;
		case '8': appendToOutput(EIGHT); break;
		case '9': appendToOutput(NINE);  break;
		}
	}
}

/******************************************************************************
*
*	function:	process_triad
*
*	purpose:	Appends to output the appropriate pronunciation for the
*                       input triad (i.e. hundreds, tens, and ones).  If the
*                       pause flag is set, then a pause is inserted before the
*                       triad proper.  If the ordinal flag is set, ordinal
*                       pronunciations are used.  If the ordinal_plural flag is
*                       set, then plural ordinal pronunciations are used.  The
*                       special flag is not used in this function, but is
*                       passed on to the process_digit() function.  The
*                       right_zero_pad is the pad for the whole word being
*                       parsed, NOT the pad for the input triad.
*
******************************************************************************/
int
NumberParser::processTriad(const char* triad, int pause, int ordinal, int right_zero_pad,
				int ordinal_plural, int special_flag)
{
	/*  IF TRIAD IS 000, RETURN ZERO  */
	if ((triad[0] == '0') && (triad[1] == '0') && (triad[2] == '0')) {
		return 0;
	}

	/*  APPEND PAUSE IF FLAG SET  */
	if (pause) {
		appendToOutput(PAUSE);
	}

	/*  PROCESS HUNDREDS  */
	if (triad[0] >= '1') {
		processDigit(triad[0], NO, NO, NO);
		if (ordinal_plural && (right_zero_pad == 2)) {
			appendToOutput(HUNDREDTHS);
		} else if (ordinal && (right_zero_pad == 2)) {
			appendToOutput(HUNDREDTH);
		} else {
			appendToOutput(HUNDRED);
		}
		if ((triad[1] != '0') || (triad[2] != '0')) {
			appendToOutput(AND);
		}
	}

	/*  PROCESS TENS  */
	if (triad[1] == '1') {
		if (ordinal_plural && (right_zero_pad == 1) && (triad[2] == '0')) {
			appendToOutput(TENTHS);
		} else if (ordinal && (right_zero_pad == 1) && (triad[2] == '0')) {
			appendToOutput(TENTH);
		} else if (ordinal_plural && (right_zero_pad == 0)) {
			switch (triad[2]) {
			case '1': appendToOutput(ELEVENTHS);    break;
			case '2': appendToOutput(TWELFTHS);     break;
			case '3': appendToOutput(THIRTEENTHS);  break;
			case '4': appendToOutput(FOURTEENTHS);  break;
			case '5': appendToOutput(FIFTEENTHS);   break;
			case '6': appendToOutput(SIXTEENTHS);   break;
			case '7': appendToOutput(SEVENTEENTHS); break;
			case '8': appendToOutput(EIGHTEENTHS);  break;
			case '9': appendToOutput(NINETEENTHS);  break;
			}
		} else if (ordinal && (right_zero_pad == 0)) {
			switch (triad[2]) {
			case '1': appendToOutput(ELEVENTH);     break;
			case '2': appendToOutput(TWELFTH);      break;
			case '3': appendToOutput(THIRTEENTH);   break;
			case '4': appendToOutput(FOURTEENTH);   break;
			case '5': appendToOutput(FIFTEENTH);    break;
			case '6': appendToOutput(SIXTEENTH);    break;
			case '7': appendToOutput(SEVENTEENTH);  break;
			case '8': appendToOutput(EIGHTEENTH);   break;
			case '9': appendToOutput(NINETEENTH);   break;
			}
		} else {
			switch (triad[2]) {
			case '0': appendToOutput(TEN);          break;
			case '1': appendToOutput(ELEVEN);       break;
			case '2': appendToOutput(TWELVE);       break;
			case '3': appendToOutput(THIRTEEN);     break;
			case '4': appendToOutput(FOURTEEN);     break;
			case '5': appendToOutput(FIFTEEN);      break;
			case '6': appendToOutput(SIXTEEN);      break;
			case '7': appendToOutput(SEVENTEEN);    break;
			case '8': appendToOutput(EIGHTEEN);     break;
			case '9': appendToOutput(NINETEEN);     break;
			}
		}
	} else if (triad[1] >= '2') {
		if (ordinal_plural && (right_zero_pad == 1)) {
			switch (triad[1]) {
			case '2': appendToOutput(TWENTIETHS);   break;
			case '3': appendToOutput(THIRTIETHS);   break;
			case '4': appendToOutput(FORTIETHS);    break;
			case '5': appendToOutput(FIFTIETHS);    break;
			case '6': appendToOutput(SIXTIETHS);    break;
			case '7': appendToOutput(SEVENTIETHS);  break;
			case '8': appendToOutput(EIGHTIETHS);   break;
			case '9': appendToOutput(NINETIETHS);   break;
			}
		} else if (ordinal && (right_zero_pad == 1)) {
			switch (triad[1]) {
			case '2': appendToOutput(TWENTIETH);    break;
			case '3': appendToOutput(THIRTIETH);    break;
			case '4': appendToOutput(FORTIETH);     break;
			case '5': appendToOutput(FIFTIETH);     break;
			case '6': appendToOutput(SIXTIETH);     break;
			case '7': appendToOutput(SEVENTIETH);   break;
			case '8': appendToOutput(EIGHTIETH);    break;
			case '9': appendToOutput(NINETIETH);    break;
			}
		} else {
			switch (triad[1]) {
			case '2': appendToOutput(TWENTY);       break;
			case '3': appendToOutput(THIRTY);       break;
			case '4': appendToOutput(FORTY);        break;
			case '5': appendToOutput(FIFTY);        break;
			case '6': appendToOutput(SIXTY);        break;
			case '7': appendToOutput(SEVENTY);      break;
			case '8': appendToOutput(EIGHTY);       break;
			case '9': appendToOutput(NINETY);       break;
			}
		}
	}
	/*  PROCESS ONES  */
	if (triad[1] != '1' && triad[2] >= '1') {
		processDigit(triad[2], (ordinal && (right_zero_pad == 0)),
				(ordinal_plural && (right_zero_pad == 0)), special_flag);
	}

	/*  RETURN WITH NONZERO VALUE  */
	return NONZERO;
}

/******************************************************************************
*
*	function:	initial_parse
*
*	purpose:	Finds positions of numbers, commas, and other symbols
*                       within the word.
*
******************************************************************************/
void
NumberParser::initialParse()
{
	/*  PUT NULL BYTE INTO output;  FIND LENGTH OF INPUT WORD  */
	wordLength_ = std::strlen(word_);

	/*  INITIALIZE PARSING VARIABLES  */
	degenerate_ = integerDigits_ = fractionalDigits_ = commas_ = decimal_ = 0;
	dollar_ = percent_ = negative_ = positive_ = ordinal_ = clock_ = slash_ = 0;
	leftParen_ = rightParen_ = blank_ = telephone_ = 0;
	ordinalPlural_ = YES;

	/*  FIND THE POSITION OF THE FOLLOWING CHARACTERS  */
	for (int i = 0; i < wordLength_; i++) {
		switch (word_[i]) {
		case ',':
			if (++commas_ > COMMAS_MAX) {
				degenerate_++;
			} else {
				commasPos_[commas_ - 1]= i;
			}
			break;
		case '.':
			decimal_++;
			decimalPos_ = i;
			break;
		case '$':
			dollar_++;
			dollarPos_ = i;
			break;
		case '%':
			percent_++;
			percentPos_ = i;
			break;
		case '-':
			if (++negative_ > NEGATIVE_MAX) {
				degenerate_++;
			} else {
				negativePos_[negative_ - 1] = i;
			}
			break;
		case '+':
			positive_++;
			positivePos_ = i;
			break;
		case ':':
			if (++clock_ > CLOCK_MAX) {
				degenerate_++;
			} else {
				clockPos_[clock_ - 1] = i;
			}
			break;
		case '/':
			slash_++;
			break;
		case '(':
			leftParen_++;
			leftParenPos_ = i;
			break;
		case ')':
			rightParen_++;
			rightParenPos_ = i;
			break;
		case ' ':
			blank_++;
			blankPos_ = i;
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
			if (decimal_ || slash_) {
				if (++fractionalDigits_ > FRACTIONAL_DIGITS_MAX) {
					degenerate_++;
				} else {
					fractionalDigitsPos_[fractionalDigits_ - 1] = i;
				}
			} else {
				if (++integerDigits_ > INTEGER_DIGITS_MAX) {
					degenerate_++;
				} else {
					integerDigitsPos_[integerDigits_ - 1] = i;
				}
			}
			break;
		case 's':
		case 'S':
		case 't':
		case 'T':
		case 'n':
		case 'N':
		case 'd':
		case 'D':
		case 'r':
		case 'R':
		case 'h':
		case 'H':
			if (++ordinal_ > 2) {
				degenerate_++;
			} else {
				char c = word_[i];
				ordinalPos_[ordinal_ - 1] = i;
				/*  CONVERT TO UPPER CASE IF NECESSARY  */
				ordinalBuffer_[ordinal_ - 1] = ((c >= 'a') && (c <= 'z')) ? c + ('A' - 'a') : c;
				ordinalBuffer_[2] = '\0';
			}
			break;
		default:
			degenerate_++;
			break;
		}
	}

	/*  FIND LEFT ZERO PAD FOR INTEGER PART OF WORD  */
	leftZeroPad_ = 0;
	for (int i = 0; i < integerDigits_; i++) {
		if (word_[integerDigitsPos_[i]] == '0') {
			leftZeroPad_++;
		} else {
			break;
		}
	}
	/*  FIND RIGHT ZERO PAD FOR INTEGER PART OF WORD  */
	rightZeroPad_ = 0;
	for (int i = integerDigits_ - 1; i >= 0; i--) {
		if (word_[integerDigitsPos_[i]] == '0') {
			rightZeroPad_++;
		} else {
			break;
		}
	}
	/*  DETERMINE RIGHT MOST TRIAD TO RECEIVE ORDINAL NAME  */
	ordinalTriad_ = static_cast<int>(rightZeroPad_ / 3.0);

	/*  FIND LEFT ZERO PAD FOR FRACTIONS  */
	fracLeftZeroPad_ = 0;
	for (int i = 0; i < fractionalDigits_; i++) {
		if (word_[fractionalDigitsPos_[i]] == '0') {
			fracLeftZeroPad_++;
		} else {
			break;
		}
	}
	/*  FIND RIGHT ZERO PAD FOR FRACTIONS  */
	fracRightZeroPad_ = 0;
	for (int i = (fractionalDigits_ - 1); i >= 0; i--) {
		if (word_[fractionalDigitsPos_[i]] == '0') {
			fracRightZeroPad_++;
		} else {
			break;
		}
	}
	/*  DETERMINE RIGHT MOST TRIAD TO RECEIVE ORDINAL NAME FOR FRACTIONS  */
	fracOrdinalTriad_ = static_cast<int>(fracRightZeroPad_ / 3.0);
}

/******************************************************************************
*
*	function:	error_check
*
*	purpose:	Checks the initiallly parsed word for format errors.
*			Returns NO_NUMERALS if the word contains no digits,
*                       DEGENERATE if the word contains errors, OK otherwise.
*
******************************************************************************/
int
NumberParser::errorCheck(Mode mode)
{
	/*  IF THERE ARE NO DIGITS THEN RETURN  */
	if ((integerDigits_ + fractionalDigits_) == 0) {
		return NO_NUMERALS;
	}

	/* IF MODE SET TO FORCE_SPELL, USE degenerate_string()  */
	if (mode == Mode::forceSpell) {
		return DEGENERATE;
	}

	/*  CANNOT HAVE UNSPECIFIED SYMBOLS, OR ANY MORE THAN ONE OF EACH OF THE
	FOLLOWING:  . $ % + / ( ) blank  */
	if (degenerate_ || decimal_ > 1 || dollar_ > 1 || percent_ > 1 ||
			positive_ > 1 || slash_ > 1 || leftParen_ > 1 ||
			rightParen_ > 1 || blank_ > 1) {
		return DEGENERATE;
	}

	/*  CHECK FOR TOO MANY DIGITS WHEN COMMAS OR ORDINAL USED  */
	if ((integerDigits_ > (TRIADS_MAX * 3)) && (commas_ || ordinal_)) {
		return DEGENERATE;
	}

	/*  MAKE SURE % SIGN AT FAR RIGHT AND THAT THERE IS NO $ SIGN  */
	if (percent_ && ((percentPos_ != (wordLength_ - 1)) || dollar_)) {
		return DEGENERATE;
	}

	/*  THE + SIGN MUST BE AT THE FAR LEFT OF THE STRING  */
	if (positive_ && (positivePos_ != 0)) {
		return DEGENERATE;
	}

	/*  IF 1 OR MORE (-) SIGNS USED,  MAKE SURE IT IS AT FAR LEFT,
	OR THAT THE NUMBER CORRESPONDS TO STANDARD TELEPHONE FORMATS  */
	if ((negative_ == 1) && (negativePos_[0] != 0)) {
		if ((integerDigits_ == 7) && (negativePos_[0] == 3) &&
				(wordLength_ == 8)) {
			telephone_ = SEVEN_DIGIT_CODE;
		} else if ((negativePos_[0] == 9) && (leftParenPos_ == 0) &&
				(rightParenPos_ == 4) && (blankPos_ == 5) &&
				(wordLength_ == 14) && (integerDigits_ == 10)) {
			telephone_ = AREA_CODE;
		} else if ((negativePos_[0] == 8) && (leftParenPos_ == 0) &&
				(rightParenPos_ == 4) && (wordLength_ == 13) &&
				(integerDigits_ == 10)) {
			telephone_ = AREA_CODE;
		} else {
			return DEGENERATE;
		}
	} else if (negative_ == 2) {
		if ((integerDigits_ == 10) && (negativePos_[0] == 3) &&
				(negativePos_[1] == 7) && (wordLength_ == 12)) {
			telephone_ = TEN_DIGIT_CODE;
		} else {
			return DEGENERATE;
		}
	} else if (negative_ == 3) {
		if ((integerDigits_ == 11) && (negativePos_[0] == 1) &&
				(negativePos_[1] == 5) && (negativePos_[2] == 9) &&
				(wordLength_ == 14)) {
			telephone_ = ELEVEN_DIGIT_CODE;
		} else {
			return DEGENERATE;
		}
	}

	/*  THE ")", "(", AND blank CHARACTERS LEGAL ONLY WHEN AREA CODE  */
	if ((leftParen_ || rightParen_ || blank_) && (telephone_ != AREA_CODE)) {
		return DEGENERATE;
	}

	/*  LEFT ZERO PADS ARE LEGAL ONLY WHEN ONE INTEGER DIGIT, OR IN
	CLOCK TIMES AND TELEPHONE NUMBERS  */
	if (leftZeroPad_ && (integerDigits_ > 1) && (!clock_) && (!telephone_)) {
		return DEGENERATE;
	}

	if (slash_) {
		/*  IF FRACTION, CHECK FOR TOO MANY DIGITS IN NUMERATOR OR DENOMINATOR  */
		if ((integerDigits_ > (TRIADS_MAX * 3)) || (fractionalDigits_ > (TRIADS_MAX * 3))) {
			return DEGENERATE;
		}

		/*  IN FRACTIONS, LEFT ZERO PADS ARE LEGAL ONLY WHEN ONE DIGIT  */
		if (fracLeftZeroPad_ && (fractionalDigits_ > 1)) {
			return DEGENERATE;
		}

		/*  FRACTIONS MUST HAVE DIGITS IN BOTH NUMERATOR AND DENOMINATOR,
			AND CANNOT CONTAIN THE . $ , : SIGNS, OR ORDINAL SUFFIXES  */
		if ((!integerDigits_) || (!fractionalDigits_) ||
				decimal_ || dollar_ || commas_ || clock_ || ordinal_) {
			return DEGENERATE;
		}
	}

	/*  CHECK FOR LEGAL CLOCK TIME FORMATS;  FILL hour AND minute AND second BUFFERS  */
	if (clock_) {
		hour_[0] = minute_[0] = second_[0] = '0';
		hour_[3] = minute_[3] = second_[3] = '\0';
		if (integerDigits_ == 3) {
			if ((wordLength_ != 4) || (clockPos_[0] != 1)) {
				return DEGENERATE;
			}
			hour_[1] = '0';
			hour_[2] = word_[integerDigitsPos_[0]];
			minute_[1] = word_[integerDigitsPos_[1]];
			minute_[2] = word_[integerDigitsPos_[2]];
			seconds_ = NO;
		} else if (integerDigits_ == 4) {
			if ((wordLength_ != 5) || (clockPos_[0] != 2)) {
				return DEGENERATE;
			}
			hour_[1] = word_[integerDigitsPos_[0]];
			hour_[2] = word_[integerDigitsPos_[1]];
			minute_[1] = word_[integerDigitsPos_[2]];
			minute_[2] = word_[integerDigitsPos_[3]];
			seconds_ = NO;
		} else if (integerDigits_ == 5) {
			if ((wordLength_ != 7) || (clockPos_[0] != 1) || (clockPos_[1] != 4)) {
				return DEGENERATE;
			}
			hour_[1] = '0';
			hour_[2] = word_[integerDigitsPos_[0]];
			minute_[1] = word_[integerDigitsPos_[1]];
			minute_[2] = word_[integerDigitsPos_[2]];
			second_[1] = word_[integerDigitsPos_[3]];
			second_[2] = word_[integerDigitsPos_[4]];
			seconds_ = YES;
		} else if (integerDigits_ == 6) {
			if ((wordLength_ != 8) || (clockPos_[0] != 2) || (clockPos_[1] != 5)) {
				return DEGENERATE;
			}
			hour_[1] = word_[integerDigitsPos_[0]];
			hour_[2] = word_[integerDigitsPos_[1]];
			minute_[1] = word_[integerDigitsPos_[2]];
			minute_[2] = word_[integerDigitsPos_[3]];
			second_[1] = word_[integerDigitsPos_[4]];
			second_[2] = word_[integerDigitsPos_[5]];
			seconds_ = YES;
		} else {
			return DEGENERATE;
		}
		{
			int minutes = 0, hours = 0, secs = 0;
			minutes = std::atoi(&minute_[0]);
			hours = std::atoi(&hour_[0]);
			if (seconds_) {
				secs = std::atoi(&second_[0]);
			}
			if (hours > 24 || minutes > 59 || secs > 59) {
				return DEGENERATE;
			}
			military_ = (hours >= 1 && hours <= 12 && (!leftZeroPad_)) ? NO : YES;
		}
	}

	/*  CHECK THAT COMMAS ARE PROPERLY SPACED  */
	if (commas_) {
		if (commasPos_[0] < integerDigitsPos_[0]) {
			return DEGENERATE;
		}
		for (int i = 0; i < commas_ - 1; i++) {
			if (commasPos_[i + 1] != (commasPos_[i] + 4)) {
				return DEGENERATE;
			}
		}
		if (decimal_ && (decimalPos_ != (commasPos_[commas_ - 1] + 4))) {
			return DEGENERATE;
		}
		if (integerDigitsPos_[integerDigits_ - 1] != (commasPos_[commas_ - 1] + 3)) {
			return DEGENERATE;
		}
		if ((integerDigitsPos_[0] + 3) < commasPos_[0]) {
			return DEGENERATE;
		}
	}

	/*  CHECK FOR LEGAL USE OF $ SIGN
	DETERMINE IF DOLLARS AND CENTS ARE PLURAL AND NONZERO  */
	if (dollar_) {
		if ((negative_ || positive_) && (dollarPos_ != 1)) {
			return DEGENERATE;
		}
		if ((!negative_) && (!positive_) && (dollarPos_ != 0)) {
			return DEGENERATE;
		}
		dollarPlural_ = dollarNonzero_ = NO;
		for (int i = integerDigits_ - 1; i >= 0; i--) {
			if (word_[integerDigitsPos_[i]] >= '1') {
				dollarNonzero_ = YES;
				if (i == (integerDigits_ - 1) &&
						(word_[integerDigitsPos_[i]] >= '2')) {
					dollarPlural_ = YES;
					break;
				} else if (i < (integerDigits_ - 1)) {
					dollarPlural_ = YES;
					break;
				}
			}
		}
		centsPlural_ = YES;
		centsNonzero_ = NO;
		for (int i = 0; i < fractionalDigits_; i++) {
			if (word_[fractionalDigitsPos_[i]] >= '1') {
				centsNonzero_ = YES;
				break;
			}
		}
		if ((fractionalDigits_ == 2) && (word_[fractionalDigitsPos_[0]] == '0')
				&& (word_[fractionalDigitsPos_[1]] == '1')) {
			centsPlural_ = NO;
		}
		if ((!dollarNonzero_) && (!centsNonzero_) && (positive_ || negative_)) {
			return DEGENERATE;
		}
	}

	/*  CHECK FOR LEGAL USE OF ORDINAL SUFFIXES  */
	if (ordinal_) {
		char tens_digit = '\0';

		char ones_digit = word_[integerDigitsPos_[integerDigits_ - 1]];
		if (integerDigits_ >= 2) {
			tens_digit = word_[integerDigitsPos_[integerDigits_ - 2]];
		}

		if ((ordinal_ != 2) || (!integerDigits_) ||
				decimal_ || dollar_ || percent_) {
			return DEGENERATE;
		}

		if ((ordinalPos_[0] != (wordLength_ - 2)) ||
				(ordinalPos_[1] != (wordLength_ - 1))) {
			return DEGENERATE;
		}

		if (std::strcmp(&ordinalBuffer_[0], "ST") == 0) {
			if ((ones_digit != '1') || (tens_digit == '1')) {
				return DEGENERATE;
			}
		} else if (std::strcmp(&ordinalBuffer_[0], "ND") == 0) {
			if ((ones_digit != '2') || (tens_digit == '1')) {
				return DEGENERATE;
			}
		} else if (std::strcmp(&ordinalBuffer_[0], "RD") == 0) {
			if ((ones_digit != '3') || (tens_digit == '1')) {
				return DEGENERATE;
			}
		} else if (std::strcmp(&ordinalBuffer_[0], "TH") == 0) {
			if (((ones_digit == '1') || (ones_digit == '2') ||
					(ones_digit == '3')) && (tens_digit != '1')) {
				return DEGENERATE;
			}
		} else {
			return DEGENERATE;
		}
	}

	/*  IF WE GET THIS FAR, THEN THE NUMBER CAN BE PROCESSED NORMALLY  */
	return OK;
}

/******************************************************************************
*
*	function:	process_word
*
*	purpose:	Processes the the input string pointed at by word
*                       and returns a pointer to a NULL terminated string
*                       which contains the corresponding pronunciation.
*
******************************************************************************/
const char*
NumberParser::processWord(Mode mode)
{
	output_.clear();

	/*  SPECIAL PROCESSING OF WORD;  EACH RETURNS IMMEDIATELY  */
	/*  PROCESS CLOCK TIMES  */
	if (clock_) {
		/*  HOUR  */
		if (leftZeroPad_) {
			appendToOutput(OH);
		}
		processTriad(&hour_[0], NO, NO, NO, NO, NO);
		/*  MINUTE  */
		if ((minute_[1] == '0') && (minute_[2] == '0')) {
			if (military_) {
				appendToOutput(HUNDRED);
			} else if (!seconds_) {
				appendToOutput(OCLOCK);
			}
		} else {
			if ((minute_[1] == '0') && (minute_[2] != '0')) {
				appendToOutput(OH);
			}
			processTriad(&minute_[0], NO, NO, NO, NO, NO);
		}
		/*  SECOND  */
		if (seconds_) {
			appendToOutput(AND);
			if ((second_[1] == '0') && (second_[2] == '0')) {
				appendToOutput(ZERO);
			} else {
				processTriad(&second_[0], NO, NO, NO, NO, NO);
			}

			if ((second_[1] == '0') && (second_[2] == '1')) {
				appendToOutput(SECOND);
			} else {
				appendToOutput(SECONDS);
			}
		}
		return output_.c_str();
	}
	/*  PROCESS TELEPHONE NUMBERS  */
	if (telephone_ == SEVEN_DIGIT_CODE) {
		for (int i = 0; i < 3; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
		appendToOutput(PAUSE);
		for (int i = 3; i < 7; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
		return output_.c_str();
	} else if (telephone_ == TEN_DIGIT_CODE) {
		for (int i = 0; i < 3; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
		appendToOutput(PAUSE);
		for (int i = 3; i < 6; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
		appendToOutput(PAUSE);
		for (int i = 6; i < 10; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
		return output_.c_str();
	} else if (telephone_ == ELEVEN_DIGIT_CODE) {
		processDigit(word_[integerDigitsPos_[0]], NO, NO, NO);
		if ((word_[integerDigitsPos_[1]] != '0') &&
				(word_[integerDigitsPos_[2]] == '0') &&
				(word_[integerDigitsPos_[3]] == '0')) {
			processDigit(word_[integerDigitsPos_[1]], NO, NO, NO);
			appendToOutput(HUNDRED);
		} else {
			appendToOutput(PAUSE);
			for (int i = 1; i < 4; i++) {
				processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
			}
		}
		appendToOutput(PAUSE);
		for (int i = 4; i < 7; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
		appendToOutput(PAUSE);
		for (int i = 7; i < 11; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
		return output_.c_str();
	} else if (telephone_ == AREA_CODE) {
		appendToOutput(AREA);
		appendToOutput(CODE);
		for (int i = 0; i < 3; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
		appendToOutput(PAUSE);
		for (int i = 3; i < 6; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
		appendToOutput(PAUSE);
		for (int i = 6; i < 10; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
		return output_.c_str();
	}
	/*  PROCESS ZERO DOLLARS AND ZERO CENTS  */
	if (dollar_ && (!dollarNonzero_) && (!centsNonzero_)) {
		appendToOutput(ZERO);
		appendToOutput(DOLLARS);
		return output_.c_str();
	}
	/*  PROCESS FOR YEAR IF INTEGER IN RANGE 1000 TO 1999  */
	if ((integerDigits_ == 4) && (wordLength_ == 4) &&
			(word_[integerDigitsPos_[0]] == '1') && (mode != Mode::overrideYears)) {
		triad_[0] = '0';
		triad_[1] = word_[integerDigitsPos_[0]];
		triad_[2] = word_[integerDigitsPos_[1]];
		processTriad(&triad_[0], NO, NO, NO, NO, NO);
		if ((word_[integerDigitsPos_[2]] == '0') && (word_[integerDigitsPos_[3]] == '0')) {
			appendToOutput(HUNDRED);
		} else if (word_[integerDigitsPos_[2]] == '0') {
			appendToOutput(OH);
			processDigit(word_[integerDigitsPos_[3]], NO, NO, NO);
		} else {
			triad_[0] = '0';
			triad_[1] = word_[integerDigitsPos_[2]];
			triad_[2] = word_[integerDigitsPos_[3]];
			processTriad(&triad_[0], NO, NO, NO, NO, NO);
		}
		return output_.c_str();
	}

	/*  ORDINARY SEQUENTIAL PROCESSING  */
	/*  APPEND POSITIVE OR NEGATIVE IF INDICATED  */
	if (positive_) {
		appendToOutput(POSITIVE);
	} else if (negative_) {
		appendToOutput(NEGATIVE);
	}

	/*  PROCESS SINGLE INTEGER DIGIT  */
	if (integerDigits_ == 1) {
		if ((word_[integerDigitsPos_[0]] == '0') && dollar_) {
			;
		} else {
			processDigit(word_[integerDigitsPos_[0]], ordinal_, NO, NO);
		}
		ordinalPlural_ = (word_[integerDigitsPos_[0]] == '1') ? NO : YES;
	} else if ((integerDigits_ >= 2) && (integerDigits_ <= (TRIADS_MAX * 3))) {
		/*  PROCESS INTEGERS AS TRIADS, UP TO MAX LENGTH  */
		for (int i = 0; i < 3; i++) {
			triad_[i] = '0';
		}
		int digit_index = 0;
		int index = static_cast<int>((integerDigits_ - 1) / 3.0);
		int num_digits = integerDigits_ - (index * 3);
		int triad_index = 3 - num_digits;
		int pause_flag = NO;
		for (int i = index; i >= 0; i--) {
			while (num_digits--) {
				triad_[triad_index++] = word_[integerDigitsPos_[digit_index++]];
			}
			if (processTriad(&triad_[0], pause_flag, (ordinal_ && (ordinalTriad_ == i)),
					rightZeroPad_, NO, NO) == NONZERO) {
				if (ordinal_ && (ordinalTriad_ == i)) {
					appendToOutput(triad_name[1][i]);
				} else {
					appendToOutput(triad_name[0][i]);
				}
				pause_flag = YES;
			}
			if ((i == 1) && (word_[integerDigitsPos_[digit_index]] == '0') &&
					((word_[integerDigitsPos_[digit_index + 1]] != '0') ||
					(word_[integerDigitsPos_[digit_index + 2]] != '0'))) {
				appendToOutput(AND);
				pause_flag = NO;
			}
			triad_index = 0;
			num_digits = 3;
		}
	} else if ((integerDigits_ > (TRIADS_MAX * 3)) && (!commas_) && (!ordinal_)) {
		/*  PROCESS EXTREMELY LARGE NUMBERS AS STREAM OF SINGLE DIGITS  */
		for (int i = 0; i < integerDigits_; i++) {
			processDigit(word_[integerDigitsPos_[i]], NO, NO, NO);
		}
	}

	/*  APPEND DOLLAR OR DOLLARS IF NEEDED  */
	if (dollar_ && dollarNonzero_) {
		if (fractionalDigits_ && (fractionalDigits_ != 2)) {
			;
		} else if (dollarPlural_) {
			appendToOutput(DOLLARS);
		} else if (!dollarPlural_) {
			appendToOutput(DOLLAR);
		}
		if (centsNonzero_ && (fractionalDigits_ == 2)) {
			appendToOutput(AND);
		}
	}

	/*  APPEND POINT IF FRACTIONAL DIGITS, NO SLASH,
		AND IF NOT .00 DOLLAR FORMAT  */
	if (fractionalDigits_ && !slash_ &&
			(!dollar_ || fractionalDigits_ != 2)) {
		appendToOutput(POINT);
		for (int i = 0; i < fractionalDigits_; i++) {
			processDigit(word_[fractionalDigitsPos_[i]], NO, NO, NO);
		}
	} else if (slash_) {
		/*  PROCESS DENOMINATOR OF FRACTIONS  */

		if (((integerDigits_ >= 3) && (fractionalDigits_ >= 3)) ||
				(word_[integerDigitsPos_[integerDigits_ - 1]] == '0')) {
			appendToOutput(PAUSE);
		}

		char tens_digit = '\0';

		char ones_digit = word_[fractionalDigitsPos_[fractionalDigits_ - 1]];
		if (fractionalDigits_ >= 2) {
			tens_digit = word_[fractionalDigitsPos_[fractionalDigits_ - 2]];
		}

		ordinal_ = YES;
		int special_flag = NO;
		if ((ones_digit == '0' && tens_digit == '\0') ||
				(ones_digit == '1' && tens_digit != '1')) {
			appendToOutput(OVER);
			ordinal_ = ordinalPlural_ = NO;
		} else if (ones_digit == '2') {
			if (tens_digit == '\0') {
				special_flag = HALF_FLAG;
			} else if (tens_digit != '1') {
				special_flag = SECONDTH_FLAG;
			}
		} else if (ones_digit == '4' && tens_digit == '\0') {
			special_flag = QUARTER_FLAG;
		}

		if (fractionalDigits_ == 1) {
			processDigit(ones_digit, ordinal_, ordinalPlural_, special_flag);
		} else if (fractionalDigits_ >= 2 && (fractionalDigits_ <= (TRIADS_MAX * 3))) {
			for (int i = 0; i < 3; i++) {
				triad_[i] = '0';
			}
			int digit_index = 0;
			int index = static_cast<int>((fractionalDigits_ - 1) / 3.0);
			int num_digits = fractionalDigits_ - (index * 3);
			int triad_index = 3 - num_digits;
			int pause_flag = NO;
			for (int i = index; i >= 0; i--) {
				while (num_digits--) {
					triad_[triad_index++] = word_[fractionalDigitsPos_[digit_index++]];
				}
				if (processTriad(&triad_[0], pause_flag,
						(ordinal_ && (fracOrdinalTriad_ == i)),
						fracRightZeroPad_,
						(ordinalPlural_ && (fracOrdinalTriad_ == i)),
						(special_flag && (fracOrdinalTriad_ == i))) == NONZERO) {
					if (ordinalPlural_ && (fracOrdinalTriad_ == i)) {
						appendToOutput(triad_name[2][i]);
					} else if (ordinal_ && (fracOrdinalTriad_ == i)) {
						appendToOutput(triad_name[1][i]);
					} else {
						appendToOutput(triad_name[0][i]);
					}
					pause_flag = YES;
				}
				if ((i == 1) &&
						(word_[fractionalDigitsPos_[digit_index]] == '0') &&
						((word_[fractionalDigitsPos_[digit_index + 1]] != '0') ||
						(word_[fractionalDigitsPos_[digit_index + 2]] != '0'))) {
					appendToOutput(AND);
					pause_flag = NO;
				}
				triad_index = 0;
				num_digits = 3;
			}
		}
	} else if (dollar_ && centsNonzero_ && (fractionalDigits_ == 2)) {
		/*  APPEND CENTS  */

		triad_[0] = '0';
		triad_[1] = word_[fractionalDigitsPos_[0]];
		triad_[2] = word_[fractionalDigitsPos_[1]];
		if (processTriad(&triad_[0], NO, NO, NO, NO, NO) == NONZERO) {
			if (centsPlural_) {
				appendToOutput(CENTS);
			} else {
				appendToOutput(CENT);
			}
		}
	}

	/*  APPEND DOLLARS IF NOT $.00 FORMAT  */
	if (dollar_ && fractionalDigits_ && (fractionalDigits_ != 2)) {
		appendToOutput(DOLLARS);
	}

	/*  APPEND PERCENT IF NECESSARY  */
	if (percent_) {
		appendToOutput(PERCENT);
	}

	/*  RETURN OUTPUT TO CALLER  */
	return output_.c_str();
}

void
NumberParser::appendToOutput(const char* s)
{
	output_.append(s);
}

/******************************************************************************
*
*	function:	number_parser
*
*	purpose:	Returns a pointer to a NULL terminated character string
*                       which contains the pronunciation for the string pointed
*                       at by the argument word_ptr.
*
******************************************************************************/
const char*
NumberParser::parse(const char* word, Mode mode)
{
	/*  MAKE POINTER TO WORD TO BE PARSED GLOBAL TO THIS FILE  */
	word_ = word;

	/*  DO INITIAL PARSE OF WORD  */
	initialParse();

	/*  DO ERROR CHECKING OF INPUT  */
	int status = errorCheck(mode);

	/*  IF NO NUMBERS, RETURN NULL;  IF CONTAINS ERRORS,
	 DO CHAR-BY-CHAR SPELLING;  ELSE, PROCESS NORMALLY  */
	if (status == NO_NUMERALS) {
		return nullptr;
	} else if (status == DEGENERATE) {
		return degenerateString(word_);
	} else if (status == OK) {
		return processWord(mode);
	}

	/*  IF HERE, RETURN NULL  */
	return nullptr;
}

/******************************************************************************
*
*	function:	degenerate_string
*
*	purpose:	Returns a pointer to a NULL terminated string which
*                       contains a character-by-character pronunciation for
*                       the NULL terminated character string pointed at by
*                       the argument word.
*
******************************************************************************/
const char*
NumberParser::degenerateString(const char* word)
{
	output_.clear();
	int wordLength = std::strlen(word);

	/*  APPEND PROPER PRONUNCIATION FOR EACH CHARACTER  */
	for (int i = 0; i < wordLength; i++) {
		switch (word[i]) {
		case ' ': appendToOutput(BLANK);               break;
		case '!': appendToOutput(EXCLAMATION_POINT);   break;
		case '"': appendToOutput(DOUBLE_QUOTE);        break;
		case '#': appendToOutput(NUMBER_SIGN);         break;
		case '$': appendToOutput(DOLLAR_SIGN);         break;
		case '%': appendToOutput(PERCENT_SIGN);        break;
		case '&': appendToOutput(AMPERSAND);           break;
		case '\'':appendToOutput(SINGLE_QUOTE);        break;
		case '(': appendToOutput(OPEN_PARENTHESIS);    break;
		case ')': appendToOutput(CLOSE_PARENTHESIS);   break;
		case '*': appendToOutput(ASTERISK);            break;
		case '+': appendToOutput(PLUS_SIGN);           break;
		case ',': appendToOutput(COMMA);               break;
		case '-': appendToOutput(HYPHEN);              break;
		case '.': appendToOutput(PERIOD);              break;
		case '/': appendToOutput(SLASH);               break;
		case '0': appendToOutput(ZERO);                break;
		case '1': appendToOutput(ONE);                 break;
		case '2': appendToOutput(TWO);                 break;
		case '3': appendToOutput(THREE);               break;
		case '4': appendToOutput(FOUR);                break;
		case '5': appendToOutput(FIVE);                break;
		case '6': appendToOutput(SIX);                 break;
		case '7': appendToOutput(SEVEN);               break;
		case '8': appendToOutput(EIGHT);               break;
		case '9': appendToOutput(NINE);                break;
		case ':': appendToOutput(COLON);               break;
		case ';': appendToOutput(SEMICOLON);           break;
		case '<': appendToOutput(OPEN_ANGLE_BRACKET);  break;
		case '=': appendToOutput(EQUAL_SIGN);          break;
		case '>': appendToOutput(CLOSE_ANGLE_BRACKET); break;
		case '?': appendToOutput(QUESTION_MARK);       break;
		case '@': appendToOutput(AT_SIGN);             break;
		case 'A':
		case 'a': appendToOutput(A);                   break;
		case 'B':
		case 'b': appendToOutput(B);                   break;
		case 'C':
		case 'c': appendToOutput(C);                   break;
		case 'D':
		case 'd': appendToOutput(D);                   break;
		case 'E':
		case 'e': appendToOutput(E);                   break;
		case 'F':
		case 'f': appendToOutput(F);                   break;
		case 'G':
		case 'g': appendToOutput(G);                   break;
		case 'H':
		case 'h': appendToOutput(H);                   break;
		case 'I':
		case 'i': appendToOutput(I);                   break;
		case 'J':
		case 'j': appendToOutput(J);                   break;
		case 'K':
		case 'k': appendToOutput(K);                   break;
		case 'L':
		case 'l': appendToOutput(L);                   break;
		case 'M':
		case 'm': appendToOutput(M);                   break;
		case 'N':
		case 'n': appendToOutput(N);                   break;
		case 'O':
		case 'o': appendToOutput(O);                   break;
		case 'P':
		case 'p': appendToOutput(P);                   break;
		case 'Q':
		case 'q': appendToOutput(Q);                   break;
		case 'R':
		case 'r': appendToOutput(R);                   break;
		case 'S':
		case 's': appendToOutput(S);                   break;
		case 'T':
		case 't': appendToOutput(T);                   break;
		case 'U':
		case 'u': appendToOutput(U);                   break;
		case 'V':
		case 'v': appendToOutput(V);                   break;
		case 'W':
		case 'w': appendToOutput(W);                   break;
		case 'X':
		case 'x': appendToOutput(X);                   break;
		case 'Y':
		case 'y': appendToOutput(Y);                   break;
		case 'Z':
		case 'z': appendToOutput(Z);                   break;
		case '[': appendToOutput(OPEN_SQUARE_BRACKET); break;
		case '\\':appendToOutput(BACKSLASH);           break;
		case ']': appendToOutput(CLOSE_SQUARE_BRACKET);break;
		case '^': appendToOutput(CARET);               break;
		case '_': appendToOutput(UNDERSCORE);          break;
		case '`': appendToOutput(GRAVE_ACCENT);        break;
		case '{': appendToOutput(OPEN_BRACE);          break;
		case '|': appendToOutput(VERTICAL_BAR);        break;
		case '}': appendToOutput(CLOSE_BRACE);         break;
		case '~': appendToOutput(TILDE);               break;
		default:  appendToOutput(UNKNOWN);             break;
		}
	}
	return output_.c_str();
}

} /* namespace English */
} /* namespace TextParser */
} /* namespace GS */
