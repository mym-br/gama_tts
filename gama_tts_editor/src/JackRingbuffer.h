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

#ifndef JACK_RINGBUFFER_H
#define JACK_RINGBUFFER_H

#include <jack/ringbuffer.h>



namespace GS {

class JackRingbuffer {
public:
	explicit JackRingbuffer(size_t ringbufferSize);
	~JackRingbuffer();

	size_t readSpace();
	size_t writeSpace();

	// Returns the number of bytes read.
	size_t read(char* dest, size_t cnt);

	// Returns the number of bytes written.
	size_t write(const char* src, size_t cnt);

	// Returns the number of bytes read.
	size_t peek(char* dest, size_t cnt);

	void advanceRead(size_t cnt);

	void reset(); // not thread safe
private:
	JackRingbuffer(const JackRingbuffer&) = delete;
	JackRingbuffer& operator=(const JackRingbuffer&) = delete;
	JackRingbuffer(JackRingbuffer&&) = delete;
	JackRingbuffer& operator=(JackRingbuffer&&) = delete;

	jack_ringbuffer_t* ringbuffer_;
};

} /* namespace GS */

#endif // JACK_RINGBUFFER_H
