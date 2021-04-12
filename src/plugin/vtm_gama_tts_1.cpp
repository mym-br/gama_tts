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

#include "vtm_gama_tts_1.h"

#include <iostream>

#include "VocalTractModel0.h"



extern "C" {

void*
GAMA_TTS_get_vocal_tract_model(void* config_data, int is_interactive)
{
	try {
		return new GS::VTM::VocalTractModel0<float>(*reinterpret_cast<GS::ConfigurationData*>(config_data), is_interactive);
	} catch (std::exception& exc) {
		std::cerr << "Error in the constructor of VocalTractModel0: " << exc.what() << std::endl;
		return NULL;
	} catch (...) {
		std::cerr << "Unknown exception thrown by the constructor of VocalTractModel0." << std::endl;
		return NULL;
	}
}

} // extern "C"
