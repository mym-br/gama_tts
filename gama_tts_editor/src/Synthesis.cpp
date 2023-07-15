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

#include "Synthesis.h"

#include "Index.h"
#include "Controller.h"
#include "ParameterModificationSynthesis.h"



namespace GS {

Synthesis::Synthesis(const AppConfig& appConfigRef)
	: appConfig(appConfigRef)
	, vtmController()
	, paramModifSynth()
	, refModel()
	, refVtmController()
{
}

Synthesis::~Synthesis()
{
}

void
Synthesis::clear()
{
	refVtmController.reset();
	refModel.reset();
	paramModifSynth.reset();
	vtmController.reset();
	index.reset();
}

void
Synthesis::setup(VTMControlModel::Model* model)
{
	if (!model) {
		clear();
		return;
	}
	try {
		index = std::make_unique<Index>(appConfig.projectDir.toStdString());
		vtmController = std::make_unique<VTMControlModel::Controller>(*index, *model);
	} catch (...) {
		clear();
		throw;
	}
}

} // namespace GS
