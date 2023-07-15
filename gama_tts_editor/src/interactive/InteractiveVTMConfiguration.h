/***************************************************************************
 *  Copyright 2007, 2008, 2014 Marcelo Y. Matuda                           *
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

#ifndef INTERACTIVE_VTM_CONFIGURATION_H_
#define INTERACTIVE_VTM_CONFIGURATION_H_

#include <memory>
#include <string>
#include <vector>

#include "ConfigurationData.h"
#include "Index.h"



namespace GS {

struct InteractiveVTMConfiguration {
public:
	Index index;
	std::string configDirPath;
	std::unique_ptr<ConfigurationData> data;
	std::unique_ptr<ConfigurationData> vtmData;

	std::vector<std::string> dynamicParamNameList;
	std::vector<std::string> dynamicParamLabelList;
	std::vector<float>       dynamicParamMinList;
	std::vector<float>       dynamicParamMaxList;
	std::vector<float>       dynamicParamList;

	std::vector<std::string> staticParamNameList;
	std::vector<std::string> staticParamLabelList;
	std::vector<float>       staticParamMinList;
	std::vector<float>       staticParamMaxList;

	explicit InteractiveVTMConfiguration(const char* configDirPath);

	// Reloads the configuration file.
	// If an error occurs, the old data will remain on this object.
	void reload();

	float staticParameter(int parameter);
	void setStaticParameter(int parameter, float value);
	void setOutputRate(float value);
private:
	std::string vtmConfigFilePath() const;
	std::string variantConfigFilePath() const;
};

} /* namespace GS */

#endif /* INTERACTIVE_VTM_CONFIGURATION_H_ */
