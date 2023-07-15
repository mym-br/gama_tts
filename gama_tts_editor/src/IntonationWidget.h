/***************************************************************************
 *  Copyright 1991, 1992, 1993, 1994, 1995, 1996, 2001, 2002               *
 *    David R. Hill, Leonard Manzara, Craig Schock                         *
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
// 2015-01
// This file was copied from Gnuspeech and modified by Marcelo Y. Matuda.

#ifndef INTONATION_WIDGET_H
#define INTONATION_WIDGET_H

#include <vector>

#include <QWidget>

#include "IntonationPoint.h"



namespace GS {

namespace VTMControlModel {
class EventList;
}

class IntonationWidget : public QWidget {
	Q_OBJECT
public:
	explicit IntonationWidget(QWidget* parent=nullptr);
	virtual ~IntonationWidget() = default;

	virtual QSize sizeHint() const;
	void updateData(VTMControlModel::EventList* eventList);
	void setSelectedPointValue(double value);
	void setSelectedPointSlope(double slope);
	void setSelectedPointBeatOffset(double beatOffset);
	bool saveIntonationToEventList();
	void sendSelectedPointData();
signals:
	void pointSelected(
		double value,
		double slope,
		double beat,
		double beatOffset,
		double absoluteTime);
public slots:
	void loadIntonationFromEventList();
protected:
	virtual void paintEvent(QPaintEvent*);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
private:
	IntonationWidget(const IntonationWidget&) = delete;
	IntonationWidget& operator=(const IntonationWidget&) = delete;
	IntonationWidget(IntonationWidget&&) = delete;
	IntonationWidget& operator=(IntonationWidget&&) = delete;

	double valueToY(double value);
	double timeToX(double time);
	double yToValue(double y);
	double xToTime(double x);
	void drawPointMarker(QPainter& painter, double x, double y);
	void smoothPoints(QPainter& painter);
	int addIntonationPoint(VTMControlModel::IntonationPoint& newPoint);

	VTMControlModel::EventList* eventList_;
	double timeScale_;
	bool modelUpdated_;
	double textYOffset_;
	double leftMargin_;
	double yStep_;
	double maxTime_;
	double graphWidth_;
	int totalWidth_;
	int totalHeight_;
	int selectedPoint_;
	std::vector<VTMControlModel::IntonationPoint> intonationPointList_;
	std::vector<int> postureTimeList_;
};

} // namespace GS

#endif // INTONATION_WIDGET_H
