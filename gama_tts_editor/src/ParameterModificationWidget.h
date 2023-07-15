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

#ifndef PARAMETER_MODIFICATION_WIDGET_H
#define PARAMETER_MODIFICATION_WIDGET_H

#include <QWidget>

namespace GS {

class ParameterModificationWidget : public QWidget {
	Q_OBJECT
public:
	explicit ParameterModificationWidget(QWidget* parent=nullptr);
	virtual ~ParameterModificationWidget() = default;

	void stop();
signals:
	void modificationStarted();
	void offsetChanged(double offset);
protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent *event);
private:
	enum class State {
		stopped,
		running
	};

	ParameterModificationWidget(const ParameterModificationWidget&) = delete;
	ParameterModificationWidget& operator=(const ParameterModificationWidget&) = delete;
	ParameterModificationWidget(ParameterModificationWidget&&) = delete;
	ParameterModificationWidget& operator=(ParameterModificationWidget&&) = delete;

	double offset(int xMouse);

	State state_;
	int mouseX_;
};

} // namespace GS

#endif // PARAMETER_MODIFICATION_WIDGET_H
