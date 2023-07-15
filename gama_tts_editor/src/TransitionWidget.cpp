/***************************************************************************
 *  Copyright 2014, 2015, 2017 Marcelo Y. Matuda                           *
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
// This file was created by Marcelo Y. Matuda, and code/information
// from Gnuspeech was added to it later.

#include "TransitionWidget.h"

#include <array>
#include <cmath> /* fabs */

#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>

#define MARGIN                       (20.0)
#define MARKER_SIZE                  (3.5)
#define TEXT_MARGIN                  (10.0)
#define SELECTION_SIZE               (6.0)
#define SLOPE_TEXT_MARGIN            (4.0)
#define TETRAPHONE_MARK1             (100.0)
#define TETRAPHONE_MARK2             (200.0)
#define TETRAPHONE_DURATION          (300.0)
#define POINT_SELECTION_MAX_DISTANCE (25.0)



namespace {

constexpr std::array<const char*, 14> transitionYLabels = {
	"   %",
	" 110",
	" 100",
	"  90",
	"  80",
	"  70",
	"  60",
	"  50",
	"  40",
	"  30",
	"  20",
	"  10",
	"   0",
	" -10"
};
constexpr std::array<const char*, 14> specialTransitionYLabels = {
	"   %",
	" 120",
	" 100",
	"  80",
	"  60",
	"  40",
	"  20",
	"   0",
	" -20",
	" -40",
	" -60",
	" -80",
	"-100",
	"-120"
};

} // namespace

namespace GS {

TransitionWidget::TransitionWidget(QWidget* parent)
		: QWidget(parent)
		, special_()
		, dataUpdated_()
		, textYOffset_()
		, slopeTextYOffset_()
		, leftMargin_()
		, yStep_()
		, graphWidth_()
		, transitionType_(VTMControlModel::Transition::Type::invalid)
		, pointList_()
		, ruleDuration_()
		, mark1_()
		, mark2_()
		, mark3_()
		, selectedPointIndex_(-1)
{
	QPalette pal;
	pal.setColor(QPalette::Window, Qt::white);
	setPalette(pal);
	setAutoFillBackground(true);
}

void
TransitionWidget::clear()
{
	transitionType_ = VTMControlModel::Transition::Type::invalid;
	pointList_ = nullptr;
	mark1_ = 0.0;
	mark2_ = 0.0;
	mark3_ = 0.0;
	ruleDuration_ = 0.0;
	selectedPointIndex_ = -1;

	update();
}

void
TransitionWidget::updateData(VTMControlModel::Transition::Type transitionType,
		std::vector<TransitionPoint>* pointList,
		float ruleDuration,
		float mark1,
		float mark2,
		float mark3)
{
	transitionType_ = transitionType;
	pointList_      = pointList;
	ruleDuration_   = ruleDuration;
	mark1_          = mark1;
	mark2_          = mark2;
	mark3_          = mark3;

	dataUpdated_ = true;
	update();
}

void
TransitionWidget::setSelectedPointIndex(int index)
{
	selectedPointIndex_ = index;
	update();
}

// Note: with no antialiasing, the coordinates in QPointF are rounded to the nearest integer.
void
TransitionWidget::paintEvent(QPaintEvent*)
{
	if (pointList_ == nullptr) {
		return;
	}

	QPainter painter(this);
	QFont normalFont{"monospace"};
	QFont slopeFont{"monospace", 9};
	painter.setFont(normalFont);

	if (dataUpdated_) {
		QFontMetrics fm = painter.fontMetrics();
		textYOffset_ = 0.5 * fm.ascent();
		if (special_) {
			leftMargin_ = MARGIN + TEXT_MARGIN + fm.horizontalAdvance(specialTransitionYLabels[0]);
		} else {
			leftMargin_ = MARGIN + TEXT_MARGIN + fm.horizontalAdvance(transitionYLabels[0]);
		}

		painter.setFont(slopeFont);
		fm = painter.fontMetrics();
		slopeTextYOffset_ = 0.5 * fm.ascent();
		painter.setFont(normalFont);

		updateScales();

		dataUpdated_ = false;
	}

	if (special_) {
		// Gray area.
		painter.fillRect(QRectF(QPointF(leftMargin_, MARGIN + 7.0 * yStep_), QPointF(leftMargin_ + graphWidth_, MARGIN + 14.0 * yStep_)), Qt::lightGray);

		// Y labels.
		for (unsigned int i = 0; i < specialTransitionYLabels.size(); ++i) {
			double y = MARGIN + i * yStep_ + textYOffset_;
			painter.drawText(QPointF(MARGIN, y), specialTransitionYLabels[i]);
			painter.drawText(QPointF(leftMargin_ + graphWidth_ + TEXT_MARGIN, y), specialTransitionYLabels[i]);
		}
	} else {
		// Gray areas.
		painter.fillRect(QRectF(QPointF(leftMargin_, MARGIN), QPointF(leftMargin_ + graphWidth_, MARGIN + 2.0 * yStep_)), Qt::lightGray);
		painter.fillRect(QRectF(QPointF(leftMargin_, MARGIN + 12.0 * yStep_), QPointF(leftMargin_ + graphWidth_, MARGIN + 14.0 * yStep_)), Qt::lightGray);

		// Y labels.
		for (unsigned int i = 0; i < transitionYLabels.size(); ++i) {
			double y = MARGIN + i * yStep_ + textYOffset_;
			painter.drawText(QPointF(MARGIN, y), transitionYLabels[i]);
			painter.drawText(QPointF(leftMargin_ + graphWidth_ + TEXT_MARGIN, y), transitionYLabels[i]);
		}
	}

	// Disabled area.
	if (transitionType_ != VTMControlModel::Transition::Type::tetraphone) {
		const double xStartDisabled = timeToX(ruleDuration_);
		painter.fillRect(QRectF(QPointF(xStartDisabled, MARGIN), QPointF(leftMargin_ + graphWidth_, MARGIN + 14.0 * yStep_)), Qt::lightGray);
	}

	// Horizontal lines.
	for (int i = 0; i < 15; ++i) {
		double y = MARGIN + i * yStep_;
		painter.drawLine(QPointF(leftMargin_, y), QPointF(leftMargin_ + graphWidth_, y));
	}

	QPen pen;
	QPen pen2;
	pen2.setWidth(2);

	painter.setRenderHint(QPainter::Antialiasing);

	// Lines.
	if (!pointList_->empty()) {
		painter.setPen(pen2);

		QPointF prevP(0.5 + timeToX(0.0), 0.5 + valueToY(0.0));

		for (unsigned int i = 0, size = pointList_->size(); i < size; ++i) {
			const auto& point = (*pointList_)[i];
			if (static_cast<int>(point.type) > static_cast<int>(transitionType_)) continue;

			QPointF p(0.5 + timeToX(point.time), 0.5 + valueToY(point.value));
			painter.drawLine(prevP, p);

			prevP = p;
			if (!special_) {
				if (i != size - 1) {
					if (point.type != (*pointList_)[i + 1].type) {
						prevP.setY(0.5 + valueToY(0.0));
					}
				}
			}
		}

		QPointF p(0.5 + timeToX(ruleDuration_), 0.5 + valueToY(special_ ? 0.0 : 100.0));
		painter.drawLine(prevP, p);

		painter.setPen(pen);
	}

	painter.setBrush(QBrush(Qt::black));

	// Points.
	if (!pointList_->empty()) {
		for (auto& point : *pointList_) {
			double x = 0.5 + timeToX(point.time);
			double y = 0.5 + valueToY(point.value);
			switch (point.type) {
			case VTMControlModel::Transition::Point::Type::diphone:
				drawDiPoint(painter, x, y);
				break;
			case VTMControlModel::Transition::Point::Type::triphone:
				if (transitionType_ == VTMControlModel::Transition::Type::diphone) continue;
				drawTriPoint(painter, x, y);
				break;
			case VTMControlModel::Transition::Point::Type::tetraphone:
				if (transitionType_ == VTMControlModel::Transition::Type::diphone
					|| transitionType_ == VTMControlModel::Transition::Type::triphone) continue;
				drawTetraPoint(painter, x, y);
				break;
			default:
				break;
			}
		}
	}

	// Posture markers.
	double yMarker = 0.5 + 0.5 * yStep_;
	drawDiPoint(   painter, 0.5 + timeToX(TETRAPHONE_MARK1)   , yMarker);
	drawTriPoint(  painter, 0.5 + timeToX(TETRAPHONE_MARK2)   , yMarker);
	drawTetraPoint(painter, 0.5 + timeToX(TETRAPHONE_DURATION), yMarker);

	painter.setBrush(QBrush());
	painter.setRenderHint(QPainter::Antialiasing, false);

	// Posture vertical lines.
	double yPosture1 = special_ ? valueToY(-140.0) : valueToY(-20.0);
	double yPosture2 = special_ ? valueToY( 140.0) : valueToY(120.0);
	double x = timeToX(0.0);
	painter.drawLine(QPointF(x, yPosture1), QPointF(x, yPosture2));
	x = timeToX(TETRAPHONE_MARK1);
	painter.drawLine(QPointF(x, yPosture1), QPointF(x, yPosture2));
	x = timeToX(TETRAPHONE_MARK2);
	painter.drawLine(QPointF(x, yPosture1), QPointF(x, yPosture2));
	x = timeToX(TETRAPHONE_DURATION);
	painter.drawLine(QPointF(x, yPosture1), QPointF(x, yPosture2));

	if (!pointList_->empty()) {
		painter.setFont(slopeFont);

		if (!special_) {
			// Slopes.
			double y1 = MARGIN + 14.0 * yStep_;
			double y2 = y1 + yStep_;
			double yText = y2 - 0.5 * yStep_ + slopeTextYOffset_;
			for (unsigned int i = 0, end = pointList_->size() - 1; i < end; ++i) {
				const auto& point1 = (*pointList_)[i];
				if (point1.hasSlope) {
					const auto& point2 = (*pointList_)[i + 1];
					painter.drawRect(QRectF(QPointF(timeToX(point1.time), y1), QPointF(timeToX(point2.time), y2)));
					painter.drawText(
						QPointF(timeToX(point1.time) + SLOPE_TEXT_MARGIN, yText),
						QString::number(point1.slope, 'f', 1));
				}
			}
		}

		// Selected point.
		if (selectedPointIndex_ >= 0 && static_cast<std::size_t>(selectedPointIndex_) < pointList_->size()) {
			int i = 0;
			for (const auto& point : *pointList_) {
				if (static_cast<int>(point.type) > static_cast<int>(transitionType_)) continue;
				if (i == selectedPointIndex_) {
					double px = timeToX(point.time);
					double py = valueToY(point.value);
					painter.drawRect(QRectF(
						 QPointF(px - SELECTION_SIZE, py - SELECTION_SIZE),
						 QPointF(px + SELECTION_SIZE, py + SELECTION_SIZE)));
				}
				++i;
			}
		}
	}
}

void
TransitionWidget::resizeEvent(QResizeEvent* /*event*/)
{
	updateScales();
}

void
TransitionWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (pointList_ == nullptr) {
		return;
	}

#ifdef USING_QT6
	QPointF pos = event->position();
#else
	QPointF pos = event->localPos();
#endif
	double time = xToTime(pos.x());
	double value = yToValue(pos.y());
	const double minValue = special_ ? -140.0 : -20.0;
	const double maxValue = special_ ?  140.0 : 120.0;
	if (time >= 0.0 && time <= ruleDuration_ && value >= minValue && value <= maxValue) {
		unsigned int pointType;
		switch (transitionType_) {
		case VTMControlModel::Transition::Type::diphone:
			pointType = 2;
			break;
		case VTMControlModel::Transition::Type::triphone:
			if (time <= mark1_) {
				pointType = 2;
			} else {
				pointType = 3;
			}
			break;
		case VTMControlModel::Transition::Type::tetraphone:
			if (time <= mark1_) {
				pointType = 2;
			} else if (time <= mark2_) {
				pointType = 3;
			} else {
				pointType = 4;
			}
			break;
		default:
			return;
		}

		qDebug("mouseDoubleClickEvent type=%u time=%f value=%f", pointType, time, value);

		emit pointCreationRequested(pointType, time, value);
	}
}

void
TransitionWidget::mousePressEvent(QMouseEvent* event)
{
	if (pointList_ == nullptr) return;
	if (pointList_->empty()) return;

	qDebug("TransitionWidget::mousePressEvent");

	unsigned int i = 0;
	for (const auto& point : *pointList_) {
		if (static_cast<int>(point.type) > static_cast<int>(transitionType_)) continue;

		const double x = timeToX(point.time);
		const double y = valueToY(point.value);
#ifdef USING_QT6
		const double absDx = std::fabs(x - event->position().x());
		const double absDy = std::fabs(y - event->position().y());
#else
		const double absDx = std::fabs(x - event->x());
		const double absDy = std::fabs(y - event->y());
#endif
		if (absDx <= POINT_SELECTION_MAX_DISTANCE && absDy <= POINT_SELECTION_MAX_DISTANCE) {
			emit pointSelected(i);
			break;
		}

		++i;
	}
}

void
TransitionWidget::updateScales()
{
	if (pointList_ == nullptr) {
		return;
	}

	yStep_ = (height() - (2.0 * MARGIN)) / 15.0;
	graphWidth_ = width() - 2.0 * leftMargin_;
}

double
TransitionWidget::valueToY(double value)
{
	if (special_) {
		return MARGIN + (7.0 - 0.05 * value) * yStep_;
	} else {
		return MARGIN + (12.0 - 0.1 * value) * yStep_;
	}
}

double
TransitionWidget::timeToX(double time)
{
	return leftMargin_ + graphWidth_ * time / TETRAPHONE_DURATION;
}

double
TransitionWidget::yToValue(double y)
{
	if (special_) {
		return ((y - MARGIN) / yStep_ - 7.0) * -20.0;
	} else {
		return ((y - MARGIN) / yStep_ - 12.0) * -10.0;
	}
}

double
TransitionWidget::xToTime(double x)
{
	return (x - leftMargin_) * TETRAPHONE_DURATION / graphWidth_;
}

void
TransitionWidget::drawDiPoint(QPainter& painter, double x, double y)
{
	painter.drawEllipse(QPointF(x, y), MARKER_SIZE, MARKER_SIZE);
}

void
TransitionWidget::drawTriPoint(QPainter& painter, double x, double y)
{
	QPointF points[3] = {
		QPointF(x, y - 1.35 * 1.15 * MARKER_SIZE),
		QPointF(x + 1.35 * MARKER_SIZE, y + 1.35 * 0.577 * MARKER_SIZE),
		QPointF(x - 1.35 * MARKER_SIZE, y + 1.35 * 0.577 * MARKER_SIZE)
	};
	painter.drawConvexPolygon(points, 3);
}

void
TransitionWidget::drawTetraPoint(QPainter& painter, double x, double y)
{
	painter.drawRect(QRectF(
			QPointF(x - 0.89 * MARKER_SIZE, y - 0.89 * MARKER_SIZE),
			QPointF(x + 0.89 * MARKER_SIZE, y + 0.89 * MARKER_SIZE)));
}

} // namespace GS
