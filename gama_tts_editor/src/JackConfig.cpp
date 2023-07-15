/***************************************************************************
 *  Copyright 2021 Marcelo Y. Matuda                                       *
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

#include "JackConfig.h"

#include "ConfigurationData.h"



namespace GS {

std::mutex JackConfig::mutex_;
std::string JackConfig::clientNamePlayer_;
std::string JackConfig::clientNameParamModif_;
std::string JackConfig::clientNameInteractive_;
std::string JackConfig::destinationPortNameRegexp_;

void
JackConfig::setupFromFile(const char* filePath)
{
	std::lock_guard<std::mutex> lock(mutex_);

	ConfigurationData data(filePath);
	clientNamePlayer_          = data.value<std::string>("client_name_player");
	clientNameParamModif_      = data.value<std::string>("client_name_param_modif");
	clientNameInteractive_     = data.value<std::string>("client_name_interactive");
	destinationPortNameRegexp_ = data.value<std::string>("destination_port_name_regexp");
}

std::string
JackConfig::clientNamePlayer()
{
	std::lock_guard<std::mutex> lock(mutex_);
	return clientNamePlayer_;
}

std::string
JackConfig::clientNameParamModif()
{
	std::lock_guard<std::mutex> lock(mutex_);
	return clientNameParamModif_;
}

std::string
JackConfig::clientNameInteractive()
{
	std::lock_guard<std::mutex> lock(mutex_);
	return clientNameInteractive_;
}

std::string
JackConfig::destinationPortNameRegexp()
{
	std::lock_guard<std::mutex> lock(mutex_);
	return destinationPortNameRegexp_;
}

} // namespace GS
