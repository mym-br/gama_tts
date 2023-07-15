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

#include "english/ApplyStress.h"

#include <stdio.h>
#include <string.h>
#include <vector>

/*  LOCAL DEFINES  ***********************************************************/
#define MAX_SYLLS      100
#define isvowel(c)     (((c)=='a') || ((c)=='e') || ((c)=='i') || ((c)=='o') || ((c)=='u') )

/*  SUFFIX TYPES  */
#define AUTOSTRESSED   0
#define PRESTRESS1     1
#define PRESTRESS2     2
#define PRESTRESS3     3	/* actually prestressed 1/2, but can't use '/' in identifier */
#define NEUTRAL        4



namespace {

/*  DATA TYPES  **************************************************************/
struct suff_data {
	const char* suff;
	int         type;
	int         sylls;
};

/*  GLOBAL VARIABLES (LOCAL TO THIS FILE)  ***********************************/
const struct suff_data suffix_list[] = {
/*  AUTOSTRESSED: (2nd entry 0)  */
	{"ade", 0, 1},
	{"aire", 0, 1},
	{"aise", 0, 1},
	{"arian", 0, 1},
	{"arium", 0, 1},
	{"cidal", 0, 2},
	{"cratic", 0, 2},
	{"ee", 0, 1},
	{"een", 0, 1},
	{"eer", 0, 1},
	{"elle", 0, 1},
	{"enne", 0, 1},
	{"ential", 0, 2},
	{"esce", 0, 1},
	{"escence", 0, 2},
	{"escent", 0, 2},
	{"ese", 0, 1},
	{"esque", 0, 1},
	{"esse", 0, 1},
	{"et", 0, 1},
	{"ette", 0, 1},
	{"eur", 0, 1},
	{"faction", 0, 2},
	{"ician", 0, 2},
	{"icious", 0, 2},
	{"icity", 0, 3},
	{"ation", 0, 2},
	{"self", 0, 1},
/* PRESTRESS1: (2nd entry 1) */
	{"cracy", 1, 2},
	{"erie", 1, 2},
	{"ety", 1, 2},
	{"ic", 1, 1},
	{"ical", 1, 2},
	{"ssion", 1, 1},
	{"ia", 1, 1},
	{"metry", 1, 2},
/* PRESTRESS2: (2nd entry 2) */
	{"able", 2, 1},   /*  NOTE: McIl GIVES WRONG SYLL. CT. */
	{"ast", 2, 1},
	{"ate", 2, 1},
	{"atory", 2, 3},
	{"cide", 2, 1},
	{"ene", 2, 1},
	{"fy", 2, 1},
	{"gon", 2, 1},
	{"tude", 2, 1},
	{"gram", 2, 1},
/* PRESTRESS 1/2: (2nd entry 3) */
	{"ad", 3, 1},
	{"al", 3, 1},
	{"an", 3, 1},	   /*  OMIT?  */
	{"ancy", 3, 2},
	{"ant", 3, 1},
	{"ar", 3, 1},
	{"ary", 3, 2},
	{"ative", 3, 2},
	{"ator", 3, 2},
	{"ature", 3, 2},
	{"ence", 3, 1},
	{"ency", 3, 2},
	{"ent", 3, 1},
	{"ery", 3, 2},
	{"ible", 3, 1},   /*  BUG  */
	{"is", 3, 1},
/* STRESS NEUTRAL: (2nd entry 4) */
	{"acy", 4, 2},
	{"age", 4, 1},
	{"ance", 4, 1},
	{"edly", 4, 2},
	{"edness", 4, 2},
	{"en", 4, 1},
	{"er", 4, 1},
	{"ess", 4, 1},
	{"ful", 4, 1},
	{"hood", 4, 1},
	{"less", 4, 1},
	{"ness", 4, 1},
	{"ish", 4, 1},
	{"dom", 4, 1},
	{0, 0, 0}	   /*  END MARKER  */
};

/*  STRESS REPELLENT PREFICES  */
const char* prefices[] = {
	"ex",
	"ac",
	"af",
	"de",
	"in",
	"non",
	0
};



int stressSuffix(const char* orthography, int* type);
bool light(const char* sb);
bool prefix(const char* orthography);



int
stressSuffix(const char* orthography, int* type)
{
	int t = 0, a, c;
	const char* b;

	c = strlen(orthography);
	while (suffix_list[t].suff) {
		b = suffix_list[t].suff;
		a = strlen(b);
		if ((a <= c) && strcmp(b, orthography + c - a) == 0) {
			*type = suffix_list[t].type;
			return suffix_list[t].sylls;
		}
		t++;
	}
	return 0;
}

/******************************************************************************
*
*	function:	light
*
*	purpose:	Determine if a syllable is light.
*
******************************************************************************/
bool
light(const char* sb)
{
	while (!isvowel(*sb)) {
		sb++;
	}
	while (isvowel(*sb) || (*sb == '_') || (*sb == '.')) {
		sb++;
	}
	if (!*sb) {
		return true;
	}
	while ((*sb != '_') && (*sb != '.') && *sb) {
		sb++;
	}
	if (!*sb) {
		return true;
	}
	while (((*sb == '_') || (*sb == '.')) && *sb) {
		sb++;
	}
	if (!*sb) {
		return true;
	}

	return isvowel(*sb);
}

bool
prefix(const char* orthography)
{
	int t = 0, l, m;
	const char* a;

	m = strlen(orthography);
	while ( (a = prefices[t++]) ) {
		if (((l = strlen(a)) <= m) && strncmp(a, orthography, l) == 0) {
			return true;
		}
	}

	return false;
}

} /* namespace */

//==============================================================================

namespace GS {
namespace TextParser {
namespace English {

/******************************************************************************
*
*	function:	apply_stress
*
*	purpose:	Find all syllables and make an array of pointers to
*                       them.  Mark each as either weak or strong in a separate
*                       array;  use the table of stress-affecting affices to
*                       find any.  If none, look for stress-repellent prefices.
*                       Decide which syllable gets the stress marker;  insert
*                       it at the pointer to that syllable.  Returns nonzero
*                       if an error occurred.
*
******************************************************************************/
int
applyStress(char* buffer, const char* orthography)
{
	if (buffer[0] == '\0') {
		return 1;
	}

	std::vector<char*> syllArray;
	bool lastWasBreak = true;
	for (char* spt = buffer; *spt; spt++) {
		if (lastWasBreak) {
			lastWasBreak = false;
			syllArray.push_back(spt);
		}
		if (*spt == '.') {
			lastWasBreak = true;
		}
	}
	// syllArray has at least one element here.

	int numSylls = syllArray.size();
	if (numSylls > MAX_SYLLS) {
		return 1;
	}

	/*  RETURNS SYLLABLE NO. (FROM THE END) THAT IS THE START OF A STRESS-AFFECTING
	SUFFIX, 0 IF NONE; AND TYPE  */
	int type;
	const int suffixSylls = stressSuffix(orthography, &type);
	int syll = -1;
	if (suffixSylls > 0) {
		if (type == AUTOSTRESSED) {
			syll = numSylls - suffixSylls;
		} else if (type == PRESTRESS1) {
			syll = numSylls - suffixSylls - 1;
		} else if (type == PRESTRESS2) {
			syll = numSylls - suffixSylls - 2;
		} else if (type == PRESTRESS3) {
			syll = numSylls - suffixSylls - 1;
			if (syll >= 0 && light(syllArray[syll])) {
				syll--;
			}
		} else if (type == NEUTRAL) {
			numSylls -= suffixSylls;
		}
	}

	if ((syll < 0) && prefix(orthography) && (numSylls >= 2)) {
		syll = 1;
	}

	if (syll < 0) {		/* if as yet unsuccessful */
		syll = numSylls - 2;
		if (syll < 0) {
			syll = 0;
		}
		if (light(syllArray[syll])) {
			syll--;
		}
	}

	if (syll < 0) {
		syll = 0;
	}

	char* spt = syllArray[syll];
	char ich = '\'';
	while (ich) {
		char temp = *spt;
		*spt = ich;
		ich = temp;
		spt++;
	}
	*spt = '\0';

	return 0;
}

} /* namespace English */
} /* namespace TextParser */
} /* namespace GS */
