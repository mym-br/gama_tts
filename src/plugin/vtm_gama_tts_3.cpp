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

#include "vtm_gama_tts_3.h"

#include <iostream>

#include "VocalTractModel2.h"



extern "C" {

void*
GAMA_TTS_construct_vocal_tract_model(void* config_data, int is_interactive)
{
	try {
		return new GS::VTM::VocalTractModel2<double, 3>(*reinterpret_cast<GS::ConfigurationData*>(config_data), is_interactive);
	} catch (std::exception& exc) {
		std::cerr << "Error in the constructor of VocalTractModel2: " << exc.what() << std::endl;
		return NULL;
	} catch (...) {
		std::cerr << "Unknown exception thrown by the constructor of VocalTractModel2." << std::endl;
		return NULL;
	}
}

void
GAMA_TTS_destruct_vocal_tract_model(void *vtm)
{
	delete reinterpret_cast<GS::VTM::VocalTractModel2<double, 3>*>(vtm);
}

} // extern "C"
