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

#ifndef JACK_CONFIG_H
#define JACK_CONFIG_H

#include <mutex>
#include <string>



namespace GS {

class JackConfig {
public:
	static void setupFromFile(const char* filePath);

	static std::string clientNamePlayer();
	static std::string clientNameParamModif();
	static std::string clientNameInteractive();
	static std::string destinationPortNameRegexp();
private:
	JackConfig() = delete;
	~JackConfig() = delete;
	JackConfig(const JackConfig&) = delete;
	JackConfig& operator=(const JackConfig&) = delete;
	JackConfig(JackConfig&&) = delete;
	JackConfig& operator=(JackConfig&&) = delete;

	static std::mutex mutex_;
	static std::string clientNamePlayer_;
	static std::string clientNameParamModif_;
	static std::string clientNameInteractive_;
	static std::string destinationPortNameRegexp_;
};

} // namespace GS

#endif // JACK_CONFIG_H
