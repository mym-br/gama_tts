/***************************************************************************
 *  Copyright 2016 Marcelo Y. Matuda                                       *
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

#include "JackRingbuffer.h"

#include "Exception.h"



namespace GS {

struct JackRingbufferException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

JackRingbuffer::JackRingbuffer(size_t ringbufferSize)
		: ringbuffer_(NULL)
{
	ringbuffer_ = jack_ringbuffer_create(ringbufferSize);
	if (ringbuffer_ == NULL) {
		THROW_EXCEPTION(JackRingbufferException, "Could not create JACK ringbuffer.");
	}
}

JackRingbuffer::~JackRingbuffer()
{
	jack_ringbuffer_free(ringbuffer_);
}

size_t
JackRingbuffer::readSpace()
{
	return jack_ringbuffer_read_space(ringbuffer_);
}

size_t
JackRingbuffer::writeSpace()
{
	return jack_ringbuffer_write_space(ringbuffer_);
}

size_t
JackRingbuffer::read(char* dest, size_t cnt)
{
	return jack_ringbuffer_read(ringbuffer_, dest, cnt);
}

size_t
JackRingbuffer::write(const char* src, size_t cnt)
{
	return jack_ringbuffer_write(ringbuffer_, src, cnt);
}

size_t
JackRingbuffer::peek(char* dest, size_t cnt)
{
	return jack_ringbuffer_peek(ringbuffer_, dest, cnt);
}

void
JackRingbuffer::advanceRead(size_t cnt)
{
	jack_ringbuffer_read_advance(ringbuffer_, cnt);
}

void
JackRingbuffer::reset()
{
	jack_ringbuffer_reset(ringbuffer_);
}

} /* namespace GS */
