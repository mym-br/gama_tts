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

#include "JackClient.h"

#include <iomanip>
#include <iostream>
#include <sstream>



namespace GS {

JackClient::JackClient(const char* clientName)
		: client_(nullptr)
{
	jack_status_t status;

	// Open a client connection to the JACK server.
	client_ = jack_client_open(clientName, JackNullOption, &status, 0);
	if (client_ == NULL) {
		try {
			std::ostringstream msg;
			msg << "jack_client_open() failed, status = 0x"
				<< std::hex << std::setw(2) << std::setfill('0') << status;
			if (status & JackServerFailed) {
				msg << ". Unable to connect to the JACK server.";
			}
			THROW_EXCEPTION(JackClientException, msg.str());
		} catch (...) {
			THROW_EXCEPTION(JackClientException, "Unable to connect to the JACK server.");
		}
	}
	if (status & JackServerStarted) {
		std::cout << "[GS::JackClient::JackClient] JACK server started." << std::endl;
	}
	if (status & JackNameNotUnique) {
		clientName = jack_get_client_name(client_);
		std::cout << "[GS::JackClient::JackClient] Unique name '" << clientName << "' assigned." << std::endl;
	}
}

JackClient::~JackClient()
{
	if (jack_client_close(client_)) {
		std::cerr << "[GS::JackClient::~JackClient] Error in jack_client_close()." << std::endl;
	}
}

void
JackClient::setProcessCallback(JackProcessCallback callback, void* arg)
{
	// Tell the JACK server to call 'callback()' whenever there is work to be done.
	if (jack_set_process_callback(client_, callback, arg)) {
		THROW_EXCEPTION(JackClientException, "Unable to connect to the JACK server.");
	}
}

void
JackClient::setShutdownCallback(JackShutdownCallback callback, void* arg)
{
	// Tell the JACK server to call 'callback()' if it ever shuts down, either entirely,
	// or if it just decides to stop calling us.
	jack_on_shutdown(client_, callback, arg);
}

jack_port_t*
JackClient::registerPort(const char* portName, const char* portType, unsigned long flags, unsigned long bufferSize)
{
	// Create the output port.
	jack_port_t* outputPort = jack_port_register(client_, portName, portType, flags, bufferSize);
	if (outputPort == NULL) {
		THROW_EXCEPTION(JackClientException, "No more JACK ports available.");
	}
	return outputPort;
}

jack_nframes_t
JackClient::getSampleRate()
{
	return jack_get_sample_rate(client_);
}

void
JackClient::activate()
{
	// Tell the JACK server that we are ready to roll. Our process callback will start running now.
	if (jack_activate(client_)) {
		THROW_EXCEPTION(JackClientException, "Cannot activate the client.");
	}
}

void
JackClient::getPorts(const char* portNamePattern, const char* typeNamePattern, unsigned long flags, JackPorts& ports)
{
	ports.list = jack_get_ports(client_, portNamePattern, typeNamePattern, flags);
}

void
JackClient::connect(const char* sourcePort, const char* destinationPort)
{
	int retVal = jack_connect(client_, sourcePort, destinationPort);
	if (retVal == EEXIST) {
		std::cerr << "[JackClient::connect] Connection from the port " << sourcePort
				<< " to the port " << destinationPort << " is already made." << std::endl;
	} else if (retVal != 0) {
		THROW_EXCEPTION(JackClientException, "Cannot connect the port " << sourcePort
				<< " to the port " << destinationPort << '.');
	}
}

const char*
JackClient::portName(const jack_port_t* port)
{
	return jack_port_name(port);
}

} /* namespace GS */
