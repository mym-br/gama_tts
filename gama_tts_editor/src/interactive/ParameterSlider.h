/***************************************************************************
 *  Copyright 2008, 2014, 2017 Marcelo Y. Matuda                           *
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

#ifndef PARAMETER_SLIDER_H_
#define PARAMETER_SLIDER_H_

#include "global.h"

#include <QSlider>



namespace GS {

class ParameterSlider : public QSlider {
	Q_OBJECT
public:
	ParameterSlider(float minimumValue, float maximumValue, QWidget* parent);
	virtual ~ParameterSlider() = default;

	void reset(float minimumValue, float maximumValue);
signals:
	void parameterValueChanged(float value);
public slots:
	void setParameterValue(float value);
private slots:
	void getSliderValue(int value);
private:
	enum {
		MAXIMUM_SLIDER_VALUE = 99999
	};

	ParameterSlider(const ParameterSlider&) = delete;
	ParameterSlider& operator=(const ParameterSlider&) = delete;
	ParameterSlider(ParameterSlider&&) = delete;
	ParameterSlider& operator=(ParameterSlider&&) = delete;

	int parameterValueToSliderValue(float value);
	float sliderValueToParameterValue(int value);

	float minimumValue_;
	float maximumValue_;
};

} /* namespace GS */

#endif /* PARAMETER_SLIDER_H_ */
