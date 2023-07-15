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

#include "english/Syllabify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>



/*  LOCAL DEFINES  ***********************************************************/
#define MAX_LEN    1024
#define isvowel(c) ((c)=='a' || (c)=='e' || (c)=='i' || (c)=='o' || (c)=='u' )
#define LEFT       begin_syllable
#define RIGHT      end_syllable



namespace {

/*  LIST OF PHONEME PATTERNS THAT CAN BEGIN A SYLLABLE  */

const char* begin_syllable[] = {
	"s_p_l",
	"s_p_r",
	"s_p_y",
	"s_p",
	"s_t_r",
	"s_t_y",
	"s_t",
	"s_k_l",
	"s_k_r",
	"s_k_y",
	"s_k_w",
	"s_k",
	"p_l",
	"p_r",
	"p_y",
	"t_r",
	"k_l",
	"k_r",
	"k_y",
	"k_w",
	"sh_r",
	"sh_l",
	"sh",
	"b_l",
	"b_r",
	"b_y",
	"b_w",
	"d_r",
	"d_y",
	"d_w",
	"g_l",
	"g_r",
	"g_y",
	"g_w",
	"d_r",
	"d_y",
	"d_w",
	"dh",
	"b",
	"d",
	"f",
	"g",
	"h",
	"j",
	"k",
	"l",
	"m",
	"n",
	"p",
	"r",
	"s",
	"t",
	"v",
	"w",
	"y",
	"z",
	0
};

/*  LIST OF PHONEME PATTERNS THAT CAN END A SYLLABLE  */
const char* end_syllable[] = {
	"b",
	"d",
	"er",
	"f",
	"g",
	"h",
	"j",
	"k",
	"l",
	"m",
	"n",
	"p",
	"r",
	"s",
	"f_t",
	"s_k",
	"s_p",
	"r_b",
	"r_d",
	"r_g",
	"l_b",
	"l_d",
	"n_d",
	"ng_k",
	"ng_z",
	"n_z",
	"l_f",
	"r_f",
	"l_v",
	"r_v",
	"l_th",
	"r_th",
	"m_th",
	"ng_th",
	"r_dh",
	"p_s",
	"t_s",
	"l_p",
	"r_p",
	"m_p",
	"l_ch",
	"r_ch",
	"n_ch",
	"l_k",
	"r_k",
	"ng_k",
	"l_j",
	"r_j",
	"n_j",
	"r_l",
	"r_l_d",
	"r_n_d",
	"r_n_t",
	"r_l_z",
	"r_n_z",
	"r_m_z",
	"r_m_th",
	"r_n",
	"r_m",
	"r_s",
	"l_s",
	"l_th",
	"r_f",
	"r_t",
	"l_t",
	"r_k",
	"k_s",
	"d_z",
	"t_th",
	"k_t",
	"p_t",
	"r_k",
	"s_t",
	"th",
	"sh",
	"zh",
	"t",
	"v",
	"w",
	"y",
	"z",
	0
};



/*  DATA TYPES  **************************************************************/
typedef char phone_type;

int syllableBreak(const char* cluster);
void createCvSignature(char* ptr, phone_type* arr);
char* add1Phone(char* t);
void extractConsonantCluster(char* ptr, phone_type* type, std::vector<char>& cluster);
int nextConsonantCluster(phone_type* pt);
int checkCluster(const char* p, const char** match_array);



/******************************************************************************
*
*	function:	syllable_break
*
*	purpose:	Returns -2 if could not break the cluster.
*
******************************************************************************/
int
syllableBreak(const char* cluster)
{
	const char* left_cluster;
	const char* right_cluster;
	char temp[MAX_LEN];
	int offset, length;

	/*  GET LENGTH OF CLUSTER  */
	length = strlen(cluster);

	/*  INITIALLY WE SHALL RETURN THE FIRST 'POSSIBLE' MATCH  */
	for (offset = -1; offset <= length; offset++) {
		if (offset == -1 || offset == length || cluster[offset] == '_' || cluster[offset] == '.') {
			strcpy(temp, cluster);
			if (offset >= 0) {
				temp[offset] = 0;
			}
			left_cluster = (offset < 0) ? temp : (offset == length ? temp + length : temp + (offset + 1));
			/*  POINTS TO BEGINNING OR NULL  */
			right_cluster = (offset >= 0) ? temp : temp + length;
			/*  NOW THEY POINT TO EITHER A LEFT/RIGHT HANDED CLUSTER OR A NULL STRING  */
			if (checkCluster(left_cluster, LEFT) && checkCluster(right_cluster, RIGHT)) {
				/*  IF THIS IS A POSSIBLE BREAK */
				/*  TEMPORARY:  WILL STORE LIST OF POSSIBLES AND PICK A 'BEST' ONE  */
				return offset;
			}
		}
	}

	/*  IF HERE, RETURN ERROR  */
	return -2;
}

void
createCvSignature(char* ptr, phone_type* arr)
{
	phone_type* arr_next;

	arr_next = arr;
	while (*ptr) {
		*arr_next++ = isvowel(*ptr) ? 'v' : 'c';
		ptr = add1Phone(ptr);
	}
	*arr_next = 0;
}

char*
add1Phone(char *t)
{
	while (*t && *t != '_' && *t != '.') {
		t++;
	}

	while (*t == '_' || *t == '.') {
		t++;
	}

	return t;
}

void
extractConsonantCluster(char* ptr, phone_type* type, std::vector<char>& cluster)
{
	char* newptr = ptr;

	while (*type == 'c') {
		type++;
		newptr = add1Phone(newptr);
	}

	cluster.assign(strlen(ptr) + 1, '\0');
	strcpy(&cluster[0], ptr);
	int offset = newptr - ptr - 1;

	if (offset >= 0) {
		cluster[offset] = '\0';
	} else {
		fprintf(stderr, "offset error\n");  // what's this??
	}
}

/******************************************************************************
*
*	function:	next_consonant_cluster
*
*	purpose:	Takes a pointer to phone_type and returns an integer
*                       offset from that point to the start of the next
*                       consonant cluster (or 0 if there are no vowels between
*                       the pointer and the end of the word, or if this is the
*                       second-last cluster and the word doesn't end with a
*                       vowel. Basically, 0 means to stop.)
*
******************************************************************************/
int
nextConsonantCluster(phone_type *pt)
{
	phone_type* pt_var;
	phone_type* pt_temp;

	pt_var = pt;
	while (*pt_var == 'c') pt_var++;

	while (*pt_var == 'v') pt_var++;

	/*  CHECK TO SEE IF WE ARE NOW ON THE FINAL CLUSTER OF THE WORD WHICH IS AT
		THE END OF THE WORD  */
	pt_temp = pt_var;

	while (*pt_temp == 'c') pt_temp++;

	return (*pt_var && *pt_temp) ? pt_var - pt : 0;
}

/******************************************************************************
*
*	function:	check_cluster
*
*	purpose:	Returns 1 if it is a possible match, 0 otherwise.
*
******************************************************************************/
int
checkCluster(const char* p, const char** match_array)
{
	const char** i;

	/*  EMPTY COUNTS AS A MATCH  */
	if (!*p) return 1;

	i = match_array;
	while (*i) {
		if (strcmp(*i, p) == 0) {
			return 1;
		}
		i++;
	}
	return 0;
}

} /* namespace */

//==============================================================================

namespace GS {
namespace TextParser {
namespace English {

/******************************************************************************
*
*	function:	syllabify
*
*	purpose:	Steps along until probable syllable beginning is found,
*                       taking the longest possible first; then continues
*			skipping vowels until a possible syllable end is found
*                       (again taking the longest possible.)  Changes '_' to
*                       '.' where it occurs between syllable end and start.
*
******************************************************************************/
int
syllabify(char* word)
{
	int i, n, temp, number_of_syllables = 0;
	phone_type cv_signature[MAX_LEN], *current_type;
	char *ptr;
	std::vector<char> cluster;

	/*  INITIALIZE THIS ARRAY TO 'c' (CONSONANT), 'v' (VOWEL), 0 (END)  */
	ptr = word;
	createCvSignature(ptr, cv_signature);
	current_type = cv_signature;

	/*  WHILE THERE IS ANOTHER CONSONANT CLUSTER (NOT THE LAST)  */
	while ( (temp = nextConsonantCluster(current_type)) ) {
		number_of_syllables++;

		/*  UPDATE CURRENT TYPE POINTER  */
		current_type += temp;

		/*  MOVE PTR TO POINT TO THAT CLUSTER  */
		for (i = 0; i < temp; i++) {
			ptr = add1Phone(ptr);
		}

		/*  EXTRACT THE CLUSTER INTO A SEPARATE STRING  */
		extractConsonantCluster(ptr, current_type, cluster);

		/*  DETERMINE WHERE THE PERIOD GOES (OFFSET FROM PTR, WHICH COULD BE -1)  */
		n = syllableBreak(&cluster[0]);

		/*  MARK THE SYLLABLE IF POSSIBLE  */
		if (n != -2) {
			*(ptr + n) = '.';
		}
	}

	/*  RETURN NUMBER OF SYLLABLES  */
	return number_of_syllables ? number_of_syllables : 1;
}

} /* namespace English */
} /* namespace TextParser */
} /* namespace GS */
