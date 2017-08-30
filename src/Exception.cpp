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

#include "Exception.h"

#include <utility> /* move */
#include <cassert>
#include <cstdio>  /* fprintf */
#include <cstdlib> /* free, malloc */
#include <cstring> /* strcpy, strlen */



namespace GS {

/*******************************************************************************
 * Constructor.
 */
ExceptionString::ExceptionString() noexcept
		: str_{}
{
}

/*******************************************************************************
 * Constructor.
 */
ExceptionString::ExceptionString(const ExceptionString& o) noexcept
		: str_{}
{
	*this = o;
}

/*******************************************************************************
 * Constructor.
 */
ExceptionString::ExceptionString(ExceptionString&& o) noexcept
		: str_{}
{
	*this = std::move(o);
}

/*******************************************************************************
 * Destructor.
 */
ExceptionString::~ExceptionString() noexcept
{
	std::free(str_);
}

/*******************************************************************************
 *
 */
ExceptionString&
ExceptionString::operator=(const ExceptionString& o) noexcept
{
	if (this != &o) {
		if (o.str_ == nullptr) {
			std::free(str_);
			str_ = nullptr;
			return *this;
		}
		std::size_t size = std::strlen(o.str_);
		auto p = static_cast<char*>(std::malloc(size + 1));
		if (p == nullptr) {
			std::fprintf(stderr, "Exception string copy error. String: %s\n", o.str_);
			return *this; // copy failed
		}
		std::free(str_);
		str_ = p;
		std::strcpy(str_, o.str_);
	}
	return *this;
}

/*******************************************************************************
 *
 */
ExceptionString&
ExceptionString::operator=(ExceptionString&& o) noexcept
{
	assert(this != &o);
	std::free(str_);
	str_ = o.str_;
	o.str_ = nullptr;
	return *this;
}

/*******************************************************************************
 *
 */
const char*
ExceptionString::str() const noexcept
{
	return str_ ? str_ : "";
}

/*******************************************************************************
 *
 */
void
ExceptionString::setStr(const char* s) noexcept
{
	if (s == nullptr) {
		std::free(str_);
		str_ = nullptr;
		return;
	}
	std::size_t size = std::strlen(s);
	auto p = static_cast<char*>(std::malloc(size + 1));
	if (p == nullptr) {
		std::fprintf(stderr, "Exception string assignment error. String: %s\n", s);
		return;
	}
	std::free(str_);
	str_ = p;
	std::strcpy(str_, s);
}

/*******************************************************************************
 *
 */
ErrorMessage&
ErrorMessage::operator<<(const std::exception& e)
{
	buffer_ << e.what();
	return *this;
}

/*******************************************************************************
 *
 */
std::string
ErrorMessage::getString() const
{
	return buffer_.str();
}

/*******************************************************************************
 *
 */
const char*
Exception::what() const noexcept
{
	return message_.str();
}

/*******************************************************************************
 *
 */
void
Exception::setMessage(const ErrorMessage& em)
{
	std::string msg = em.getString();
	message_.setStr(msg.c_str());
}

} /* namespace GS */
