/***************************************************************************
 *  Copyright 2015, 2017 Marcelo Y. Matuda                                 *
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

#include <cstdlib>
#include <cstring> /* strcmp */
#include <fstream>
#include <iomanip>
#include <iostream>

#include "global.h"
#include "ModuleController.h"
#include "RtAudio.h"

#define PROGRAM_NAME "sd_gama_tts"



void
showUsage()
{
	std::cerr << "\nGamaTTS " << GAMA_TTS_VERSION << " module for Speech Dispatcher.\n\n";
	std::cerr << "Usage:\n"
			<< PROGRAM_NAME << " <config. file path>\n"
			<< "        Run module.\n"
			<< PROGRAM_NAME << " -o\n"
			<< "        List audio output devices.\n"
			<< std::endl;
}

void
showAudioOutputDevices()
{
	RtAudio audio;

	unsigned int devices = audio.getDeviceCount();
	if (devices == 0) {
		std::cout << "No audio output devices found." << std::endl;
		return;
	}

	std::cout << "Audio output devices:\n";
	RtAudio::DeviceInfo info;
	for (unsigned int i = 0; i < devices; ++i) {
		info = audio.getDeviceInfo(i);
		if (info.probed && info.outputChannels > 0) {
			std::cout << std::setw(3) << i << ": " << info.name;
			if (info.isDefaultOutput) {
				std::cout << " (default)";
			}
			std::cout << '\n';
		}
	}
	std::cout << std::endl;
}



int
main(int argc, char* argv[])
{
	if (argc != 2) {
		showUsage();
		return EXIT_FAILURE;
	}

	if (std::strcmp(argv[1], "-o") == 0) {
		showAudioOutputDevices();
		return EXIT_SUCCESS;
	}
	if (argv[1][0] == '-') {
		showUsage();
		return EXIT_FAILURE;
	}
	if (!std::ifstream{argv[1]}) {
		std::cerr << "Could not open the file \"" << argv[1] << "\"." << std::endl;
		return EXIT_FAILURE;
	}

	ModuleController controller(std::cin, std::cout, argv[1]);
	controller.exec();

	return EXIT_SUCCESS;
}
