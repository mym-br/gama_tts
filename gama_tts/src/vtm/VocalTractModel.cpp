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

#include "VocalTractModel.h"

#include "ConfigurationData.h"
#include "Exception.h"
#include "VocalTractModel0.h"
#include "VocalTractModel2.h"
#include "VocalTractModel4.h"
#include "VocalTractModel5.h"



namespace GS {
namespace VTM {

std::unique_ptr<VocalTractModel>
VocalTractModel::getInstance(const ConfigurationData& data, bool interactive)
{
	const unsigned int modelNumber = data.value<unsigned int>("model");
	switch (modelNumber) {
	case 0:
		return std::make_unique<VocalTractModel0<double>>(data, interactive);
	case 1:
		return std::make_unique<VocalTractModel0<float>>(data, interactive);
	case 2:
		return std::make_unique<VocalTractModel2<double, 1>>(data, interactive);
	case 3:
		return std::make_unique<VocalTractModel2<double, 3>>(data, interactive);
	case 4:
		return std::make_unique<VocalTractModel4<double, 1>>(data, interactive);
	case 5:
		return std::make_unique<VocalTractModel5<double, 1>>(data, interactive);
	default:
		THROW_EXCEPTION(InvalidValueException, "[VocalTractModel::getInstance] Invalid vocal tract model number: " << modelNumber << '.');
	}
}

} /* namespace VTM */
} /* namespace GS */
