/***************************************************************************
 *  Copyright 2015 Marcelo Y. Matuda                                       *
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
/***************************************************************************
 *  The code that handles the communication with the JACK server was
 *  based on simple_client.c from JACK 1.9.10.
 ***************************************************************************/

#ifndef JACK_CLIENT_H
#define JACK_CLIENT_H

#include <jack/jack.h>

#include "Exception.h"



namespace GS {

struct JackClientException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

struct JackPorts {
public:
	const char** list; // NULL-terminated list of strings

	JackPorts() : list(NULL) { }
	~JackPorts() { jack_free(list); }
private:
	JackPorts(const JackPorts&) = delete;
	JackPorts& operator=(const JackPorts&) = delete;
	JackPorts(JackPorts&&) = delete;
	JackPorts& operator=(JackPorts&&) = delete;
};

class JackClient {
public:
	explicit JackClient(const char* clientName);
	~JackClient();

	void setProcessCallback(JackProcessCallback callback, void* arg);
	void setShutdownCallback(JackShutdownCallback callback, void* arg);
	jack_port_t* registerPort(const char* portName, const char* portType,
			unsigned long flags, unsigned long bufferSize);
	jack_nframes_t getSampleRate();
	void activate();
	void getPorts(const char* portNamePattern, const char* typeNamePattern,
			unsigned long flags, JackPorts& ports);
	void connect(const char* sourcePort, const char* destinationPort);

	static const char* portName(const jack_port_t* port);
private:
	JackClient(const JackClient&) = delete;
	JackClient& operator=(const JackClient&) = delete;
	JackClient(JackClient&&) = delete;
	JackClient& operator=(JackClient&&) = delete;

	jack_client_t* client_;
};

} /* namespace GS */

#endif // JACK_CLIENT_H
