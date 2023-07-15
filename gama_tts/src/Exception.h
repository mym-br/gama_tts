/*
 * Copyright 2014, 2017, 2018, 2019 Marcelo Y. Matuda
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GS_EXCEPTION_H_
#define GS_EXCEPTION_H_

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

#endif /* GS_EXCEPTION_H_ */
