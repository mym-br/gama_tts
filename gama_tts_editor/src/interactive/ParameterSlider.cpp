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

#include "ParameterSlider.h"

#include <cmath> /* round */

#include <QSignalBlocker>

#include "Exception.h"



namespace {

constexpr float EPSILON = 1.0e-10f;

}

//==============================================================================

namespace GS {

/*******************************************************************************
 * Constructor.
 */
ParameterSlider::ParameterSlider(float minimumValue, float maximumValue, QWidget* parent)
		: QSlider(Qt::Horizontal, parent)
		, minimumValue_(minimumValue)
		, maximumValue_(maximumValue)
{
	if (maximumValue < minimumValue + EPSILON) {
		THROW_EXCEPTION(InvalidValueException, "[ParameterSlider] The maximum value must be greater than the minimum value (min: "
				<< minimumValue << " max: " << maximumValue << ").");
	}

	setRange(0, MAXIMUM_SLIDER_VALUE);

	connect(this, &ParameterSlider::valueChanged, this, &ParameterSlider::getSliderValue);

	setParameterValue(minimumValue);
}

/*******************************************************************************
 *
 */
int
ParameterSlider::parameterValueToSliderValue(float value)
{
	return std::round(((value - minimumValue_) / (maximumValue_ - minimumValue_)) * MAXIMUM_SLIDER_VALUE);
}

/*******************************************************************************
 *
 */
float
ParameterSlider::sliderValueToParameterValue(int value)
{
	return (static_cast<float>(value) / MAXIMUM_SLIDER_VALUE) * (maximumValue_ - minimumValue_) + minimumValue_;
}

/*******************************************************************************
 * Slot.
 */
void
ParameterSlider::getSliderValue(int value)
{
	float paramValue = sliderValueToParameterValue(value);

	emit parameterValueChanged(paramValue);
}

/*******************************************************************************
 * Slot.
 */
void
ParameterSlider::setParameterValue(float value)
{
	const float boundValue = qBound(minimumValue_, value, maximumValue_);
	const int sliderValue = parameterValueToSliderValue(boundValue);
	{
		QSignalBlocker sb{this}; // to avoid circular events
		setValue(sliderValue);
	}
}

/*******************************************************************************
 *
 */
void
ParameterSlider::reset(float minimumValue, float maximumValue)
{
	if (maximumValue < minimumValue + EPSILON) {
		THROW_EXCEPTION(InvalidValueException, "[ParameterSlider] The maximum value must be greater than the minimum value (min: "
				<< minimumValue << " max: " << maximumValue << ").");
	}
	minimumValue_ = minimumValue;
	maximumValue_ = maximumValue;
	setParameterValue(minimumValue);
}

} /* namespace GS */
