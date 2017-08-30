/***************************************************************************
 *  Copyright 2014 Marcelo Y. Matuda                                       *
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
#include <sstream>
#include <string>

// __func__ is defined in C99/C++11.
// __PRETTY_FUNCTION__ is a gcc extension.
#ifdef __GNUC__
# define GS_EXCEPTION_FUNCTION_NAME __PRETTY_FUNCTION__
#else
# define GS_EXCEPTION_FUNCTION_NAME __func__
#endif

#define THROW_EXCEPTION(E,M) \
	do {\
		E exc;\
		try { \
			GS::ErrorMessage em;\
			em << M << "\n[file: " << __FILE__ << "]\n[function: " << GS_EXCEPTION_FUNCTION_NAME << "]\n[line: " << __LINE__ << "]";\
			exc.setMessage(em);\
		} catch (...) {}\
		throw exc;\
	} while (false)



namespace GS {

/*******************************************************************************
 *
 */
class ExceptionString {
public:
	ExceptionString() noexcept;
	ExceptionString(const ExceptionString& o) noexcept;
	ExceptionString(ExceptionString&& o) noexcept;
	~ExceptionString() noexcept;

	ExceptionString& operator=(const ExceptionString& o) noexcept;
	ExceptionString& operator=(ExceptionString&& o) noexcept;
	const char* str() const noexcept;
	void setStr(const char* s) noexcept;
private:
	char* str_;
};

/*******************************************************************************
 *
 * This class may throw std::bad_alloc.
 */
class ErrorMessage {
public:
	ErrorMessage() = default;
	~ErrorMessage() = default;

	template<typename T> ErrorMessage& operator<<(const T& messagePart) {
		buffer_ << messagePart;
		return *this;
	}

	ErrorMessage& operator<<(const std::exception& e);
	std::string getString() const;
private:
	ErrorMessage(const ErrorMessage&) = delete;
	ErrorMessage& operator=(const ErrorMessage&) = delete;

	std::ostringstream buffer_;
};

/*******************************************************************************
 *
 */
class Exception : public std::exception {
public:
	virtual const char* what() const noexcept;

	// May throw std::bad_alloc.
	void setMessage(const ErrorMessage& em);
private:
	ExceptionString message_;
};



class AudioException                    : public Exception {};
class EndOfBufferException              : public Exception {};
class ExternalProgramExecutionException : public Exception {};
class InvalidCallException              : public Exception {};
class InvalidDirectoryException         : public Exception {};
class InvalidFileException              : public Exception {};
class InvalidParameterException         : public Exception {};
class InvalidStateException             : public Exception {};
class InvalidValueException             : public Exception {};
class IOException                       : public Exception {};
class MissingValueException             : public Exception {};
class ParsingException                  : public Exception {};
class TextParserException               : public Exception {};
class ValidationException               : public Exception {};
class VTMControlModelException          : public Exception {};
class VTMException                      : public Exception {};
class UnavailableResourceException      : public Exception {};
class WrongBufferSizeException          : public Exception {};
class XMLException                      : public Exception {};

} /* namespace GS */

#endif /* EXCEPTION_H_ */
