/***************************************************************************
 *  Copyright 2014, 2017, 2018, 2019 Marcelo Y. Matuda                     *
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

#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

// __func__ is defined in C99/C++11.
// __PRETTY_FUNCTION__ is a GCC extension.
#ifdef __GNUG__
# define GS_EXCEPTION_FUNCTION_NAME __PRETTY_FUNCTION__
#else
# define GS_EXCEPTION_FUNCTION_NAME __func__
#endif

namespace GS {

#define THROW_EXCEPTION(E,M) \
	do {\
		std::ostringstream throwExceptionBuf_;\
		std::string throwExceptionMsg_;\
		try {\
			throwExceptionBuf_ << M << "\n[file: " << __FILE__ <<\
			"]\n[function: " << GS_EXCEPTION_FUNCTION_NAME <<\
			"]\n[line: " << __LINE__ << "]";\
			throwExceptionMsg_ = throwExceptionBuf_.str();\
		} catch (...) {\
			std::cerr << "Exception caught during error message processing." << std::endl;\
		}\
		throw E(throwExceptionMsg_); /* E(throwExceptionMsg_) may throw std::bad_alloc */\
	} while (false)



// The constructors of these types may throw std::bad_alloc.
struct Exception : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct AudioException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct EndOfBufferException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct ExternalProgramExecutionException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct InvalidCallException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct InvalidDirectoryException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct InvalidFileException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct InvalidParameterException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct InvalidStateException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct InvalidValueException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct IOException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct MissingValueException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct ParsingException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct TextParserException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct ValidationException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct VTMControlModelException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct VTMException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct UnavailableResourceException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct WrongBufferSizeException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
struct XMLException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

} // namespace GS

#endif /* EXCEPTION_H_ */
