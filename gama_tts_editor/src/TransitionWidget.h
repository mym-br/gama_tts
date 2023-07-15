/***************************************************************************
 *  Copyright 2014, 2015 Marcelo Y. Matuda                                 *
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

#ifndef TRANSITION_WIDGET_H
#define TRANSITION_WIDGET_H

#include <vector>

#include <QWidget>

#include "Transition.h"
#include "TransitionPoint.h"



namespace GS {

class TransitionWidget : public QWidget {
	Q_OBJECT
public:
	explicit TransitionWidget(QWidget* parent=nullptr);
	virtual ~TransitionWidget() = default;

	void setSpecial() { special_ = true; }
	void clear();
	void updateData(VTMControlModel::Transition::Type transitionType,
			std::vector<TransitionPoint>* pointList,
			float ruleDuration,
			float mark1,
			float mark2,
			float mark3);

	void setSelectedPointIndex(int index);
signals:
	void pointCreationRequested(unsigned int pointType, float time, float value);
	void pointSelected(unsigned int pointIndex);
protected:
	virtual void paintEvent(QPaintEvent*);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
private:
	TransitionWidget(const TransitionWidget&) = delete;
	TransitionWidget& operator=(const TransitionWidget&) = delete;
	TransitionWidget(TransitionWidget&&) = delete;
	TransitionWidget& operator=(TransitionWidget&&) = delete;

	void updateScales();
	double valueToY(double value);
	double timeToX(double time);
	double yToValue(double y);
	double xToTime(double x);
	static void drawDiPoint(QPainter& painter, double x, double y);
	static void drawTriPoint(QPainter& painter, double x, double y);
	static void drawTetraPoint(QPainter& painter, double x, double y);

	bool special_;
	bool dataUpdated_;
	double textYOffset_;
	double slopeTextYOffset_;
	double leftMargin_;
	double yStep_;
	double graphWidth_;
	VTMControlModel::Transition::Type transitionType_;
	const std::vector<TransitionPoint>* pointList_;
	float ruleDuration_;
	float mark1_;
	float mark2_;
	float mark3_;
	int selectedPointIndex_;
};

} // namespace GS

#endif // TRANSITION_WIDGET_H
