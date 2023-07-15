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

#ifndef PARAMETER_LINE_EDIT_H_
#define PARAMETER_LINE_EDIT_H_

#include <QLineEdit>
#include <QString>



namespace GS {

class ParameterLineEdit : public QLineEdit {
	Q_OBJECT
public:
	ParameterLineEdit(int parameter, float minimumValue, float maximumValue, float value, QWidget* parent=nullptr);
	virtual ~ParameterLineEdit() = default;

	void reset(float minimumValue, float maximumValue, float value);
	float parameterValue() const { return value_; }
	QString parameterString() const { return text(); }
signals:
	// Sends the value to the vocal tract model.
	void parameterValueChanged(int parameter, float value);
	// Sends the value to the slider.
	void parameterValueChanged(float value);
public slots:
	void setParameterValue(float value);
	void setParameterValueFromSlider(float value);
private slots:
	void getValueFromEditedText();
private:
	ParameterLineEdit(const ParameterLineEdit&) = delete;
	ParameterLineEdit& operator=(const ParameterLineEdit&) = delete;
	ParameterLineEdit(ParameterLineEdit&&) = delete;
	ParameterLineEdit& operator=(ParameterLineEdit&&) = delete;

	void setValue(float value);
	void showValue();

	int parameter_;
	float minimumValue_;
	float maximumValue_;
	float value_;
};

} /* namespace GS */

#endif /* PARAMETER_LINE_EDIT_H_ */
