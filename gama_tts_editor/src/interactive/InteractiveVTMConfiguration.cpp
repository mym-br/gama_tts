/***************************************************************************
 *  Copyright 2007, 2008, 2014, 2017 Marcelo Y. Matuda                     *
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

#include "InteractiveVTMConfiguration.h"

#include <QString>

#include "Exception.h"
#include "global.h"



namespace GS {

/*******************************************************************************
 * Constructor.
 */
InteractiveVTMConfiguration::InteractiveVTMConfiguration(const char* configDirPath)
		: index(configDirPath)
		, configDirPath(configDirPath)
		, data(std::make_unique<ConfigurationData>(index.entry("interactive_file")))
		, dynamicParamNameList(data->value<unsigned int>("num_dynamic_parameters"))
		, dynamicParamLabelList(dynamicParamNameList.size())
		, dynamicParamMinList(  dynamicParamNameList.size())
		, dynamicParamMaxList(  dynamicParamNameList.size())
		, dynamicParamList(     dynamicParamNameList.size())
		, staticParamNameList(data->value<unsigned int>("num_static_parameters"))
		, staticParamLabelList(staticParamNameList.size())
		, staticParamMinList(  staticParamNameList.size())
		, staticParamMaxList(  staticParamNameList.size())
{
	{
		QString         nameKey{"dynamic_param-%1-name"};
		QString        labelKey{"dynamic_param-%1-label"};
		QString     minValueKey{"dynamic_param-%1-min"};
		QString     maxValueKey{"dynamic_param-%1-max"};
		QString defaultValueKey{"dynamic_param-%1-default"};

		const std::size_t n = dynamicParamNameList.size();
		for (std::size_t i = 0; i < n; ++i) {
			dynamicParamNameList[i]  = data->value<std::string>(  nameKey.arg(i).toStdString());
			dynamicParamLabelList[i] = data->value<std::string>( labelKey.arg(i).toStdString());
			dynamicParamMinList[i]   = data->value<float>(    minValueKey.arg(i).toStdString());
			dynamicParamMaxList[i]   = data->value<float>(    maxValueKey.arg(i).toStdString());
			if (dynamicParamMinList[i] >= dynamicParamMaxList[i]) {
				THROW_EXCEPTION(InvalidValueException, "The mininum value must be less than the maximum value. Parameter name: "
						<< dynamicParamNameList[i] << '.');
			}
			dynamicParamList[i]      = data->value<float>(defaultValueKey.arg(i).toStdString(),
									dynamicParamMinList[i], dynamicParamMaxList[i]);
		}
	}
	{
		QString     nameKey{"static_param-%1-name"};
		QString    labelKey{"static_param-%1-label"};
		QString minValueKey{"static_param-%1-min"};
		QString maxValueKey{"static_param-%1-max"};

		const std::size_t n = staticParamNameList.size();
		for (std::size_t i = 0; i < n; ++i) {
			staticParamNameList[i]  = data->value<std::string>(  nameKey.arg(i).toStdString());
			staticParamLabelList[i] = data->value<std::string>( labelKey.arg(i).toStdString());
			staticParamMinList[i]   = data->value<float>(    minValueKey.arg(i).toStdString());
			staticParamMaxList[i]   = data->value<float>(    maxValueKey.arg(i).toStdString());
			if (staticParamMinList[i] >= staticParamMaxList[i]) {
				THROW_EXCEPTION(InvalidValueException, "The mininum value must be less than the maximum value. Parameter name: "
						<< staticParamNameList[i] << '.');
			}
		}
	}

	vtmData = std::make_unique<ConfigurationData>(vtmConfigFilePath());
	vtmData->insert(ConfigurationData(variantConfigFilePath()));
}



void
InteractiveVTMConfiguration::reload()
{
	InteractiveVTMConfiguration newConfig{configDirPath.c_str()};

	if (newConfig.dynamicParamNameList.size() != dynamicParamNameList.size()) {
		THROW_EXCEPTION(InvalidValueException, "The number of dynamic parameters is different.");
	}
	if (newConfig.staticParamNameList.size() != staticParamNameList.size()) {
		THROW_EXCEPTION(InvalidValueException, "The number of static parameters is different.");
	}

	*this = std::move(newConfig);
}

float
InteractiveVTMConfiguration::staticParameter(int parameter)
{
	return vtmData->value<float>(staticParamNameList[parameter]);
}

void
InteractiveVTMConfiguration::setStaticParameter(int parameter, float value)
{
	vtmData->put(staticParamNameList[parameter], value);
}

void
InteractiveVTMConfiguration::setOutputRate(float value)
{
	vtmData->put("output_rate", value);
}

std::string
InteractiveVTMConfiguration::vtmConfigFilePath() const
{
	return index.entry("vtm_file");
}

std::string
InteractiveVTMConfiguration::variantConfigFilePath() const
{
	return index.entry("variant_dir") + data->value<std::string>("variant_name") + ".txt";
}

} /* namespace GS */
