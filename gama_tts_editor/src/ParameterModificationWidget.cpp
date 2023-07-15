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

#include "ParameterModificationWidget.h"

#include <QPainter>
#include <QMouseEvent>



namespace GS {

ParameterModificationWidget::ParameterModificationWidget(QWidget* parent)
		: QWidget(parent)
		, state_(State::stopped)
		, mouseX_()
{
	setMouseTracking(true);
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
}

void
ParameterModificationWidget::stop()
{
	state_ = State::stopped;
	update();
}

void
ParameterModificationWidget::paintEvent(QPaintEvent* /*event*/)
{
	QPainter painter(this);

	const int xCenter = width() / 2;
	const int xEnd = width() - 1;
	const int yEnd = height() - 1;
	painter.drawLine(0, 0, 0, yEnd);
	painter.drawLine(0, 0, xEnd, 0);
	painter.drawLine(xEnd, 0, xEnd, yEnd);
	painter.drawLine(0, yEnd, xEnd, yEnd);
	painter.drawLine(xCenter, 0, xCenter, yEnd);

	if (state_ == State::running) {
		const int yCenter = height() / 2;
		painter.drawLine(xCenter, 0, mouseX_, yCenter);
		painter.drawLine(xCenter, yEnd, mouseX_, yCenter);
	}
}

void
ParameterModificationWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (state_ == State::running) {
#ifdef USING_QT6
		mouseX_ = event->position().x(); // truncate
#else
		mouseX_ = event->x();
#endif
		emit offsetChanged(offset(mouseX_));
		update();
	}
}

void
ParameterModificationWidget::mousePressEvent(QMouseEvent* event)
{
	emit modificationStarted();
#ifdef USING_QT6
	mouseX_ = event->position().x(); // truncate
#else
	mouseX_ = event->x();
#endif
	emit offsetChanged(offset(mouseX_));
	state_ = State::running;
	update();
}

double
ParameterModificationWidget::offset(int xMouse)
{
	const double xCenter = static_cast<double>(width() / 2);
	return (xMouse - xCenter) / xCenter;
}

} // namespace GS
