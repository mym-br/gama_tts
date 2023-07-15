/***************************************************************************
 *  Copyright 2017 Marcelo Y. Matuda                                       *
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

#include "ParameterLineEdit.h"

#include <QDoubleValidator>
#include <QFont>
#include <QFontMetrics>

#include "Exception.h"



namespace {

constexpr float EPSILON = 1.0e-10f;
constexpr int NUM_SIGNIFICANT_DIGITS = 5;

}

namespace GS {

ParameterLineEdit::ParameterLineEdit(int parameter, float minimumValue, float maximumValue, float value, QWidget* parent)
		: QLineEdit(parent)
		, parameter_(parameter)
		, minimumValue_(minimumValue)
		, maximumValue_(maximumValue)
{
	if (maximumValue < minimumValue + EPSILON) {
		THROW_EXCEPTION(InvalidValueException, "[ParameterSlider] The maximum value must be greater than the minimum value (parameter: "
				<< parameter_ << " min: " << minimumValue_ << " max: " << maximumValue_ << ").");
	}

	setFont(QFont("monospace"));
	QFontMetrics fm = fontMetrics();
	setFixedWidth(fm.horizontalAdvance(" -0.0000 "));

	QDoubleValidator* v = new QDoubleValidator(this); // not using range, because it does not work well with single precision values
	v->setNotation(QDoubleValidator::StandardNotation);
	setValidator(v);

	setParameterValue(value);

	connect(this, &ParameterLineEdit::editingFinished, this, &ParameterLineEdit::getValueFromEditedText);
}

void
ParameterLineEdit::reset(float minimumValue, float maximumValue, float value)
{
	if (maximumValue < minimumValue + EPSILON) {
		THROW_EXCEPTION(InvalidValueException, "[ParameterSlider] The maximum value must be greater than the minimum value (parameter: "
				<< parameter_ << " min: " << minimumValue << " max: " << maximumValue << ").");
	}

	minimumValue_ = minimumValue;
	maximumValue_ = maximumValue;

	setParameterValue(value);
}

// Slot.
void
ParameterLineEdit::setParameterValue(float value)
{
	setValue(value);
	showValue();

	emit parameterValueChanged(parameter_, value_);
	emit parameterValueChanged(value_);
}

// Slot.
void
ParameterLineEdit::setParameterValueFromSlider(float value)
{
	setValue(value);
	showValue();

	emit parameterValueChanged(parameter_, value_);
}

// Slot.
void
ParameterLineEdit::getValueFromEditedText()
{
	bool ok;
	float value = text().toFloat(&ok);
	if (ok) {
		setParameterValue(value);
	}
}

void
ParameterLineEdit::setValue(float value)
{
	value_ = qBound(minimumValue_, value, maximumValue_);
}

void
ParameterLineEdit::showValue()
{
	setText(QString::number(value_, 'g', NUM_SIGNIFICANT_DIGITS));
	setCursorPosition(0);
}

} /* namespace GS */
