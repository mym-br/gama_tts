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

#include "IntonationWidget.h"

#include <algorithm> /* max, min */
#include <array>
#include <cmath> /* abs */

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>

#include "EventList.h"
#include "Model.h"

#define MARGIN 10.0
#define TRACK_HEIGHT 20.0
#define GRAPH_HEIGHT 300.0
#define MININUM_WIDTH 1024
#define MININUM_HEIGHT 390
#define TEXT_MARGIN 10.0
#define DEFAULT_TIME_SCALE 0.7
#define SELECTION_SIZE 6.0
#define MIN_VALUE (-20.0)
#define MAX_VALUE 10.0
#define MAX_SLOPE 0.2
#define VALUE_STEP 2.0
#define MARKER_SIZE 3.5
#define SMOOTH_POINTS_X_INCREMENT 5
#define TIME_DIVISION_MARK_SIZE 5



namespace {

constexpr std::array<const char*, 16> yLabels = {
	" 10",
	"  8",
	"  6",
	"  4",
	"  2",
	"  0",
	" -2",
	" -4",
	" -6",
	" -8",
	"-10",
	"-12",
	"-14",
	"-16",
	"-18",
	"-20"
};

} // namespace

namespace GS {

IntonationWidget::IntonationWidget(QWidget* parent)
		: QWidget(parent)
		, eventList_(nullptr)
		, timeScale_(DEFAULT_TIME_SCALE)
		, modelUpdated_(false)
		, textYOffset_(0.0)
		, leftMargin_(0.0)
		, yStep_(TRACK_HEIGHT)
		, maxTime_(100.0)
		, graphWidth_(maxTime_ * timeScale_)
		, totalWidth_(MININUM_WIDTH)
		, totalHeight_(MININUM_HEIGHT)
		, selectedPoint_(-1)
{
	setMinimumWidth(totalWidth_);
	setMinimumHeight(totalHeight_);

	setFocusPolicy(Qt::StrongFocus); // enable key events
}



// Note: with no antialiasing, the coordinates in QPointF are rounded to the nearest integer.
void
IntonationWidget::paintEvent(QPaintEvent*)
{
	if (eventList_ == nullptr || eventList_->list().empty()) {
		return;
	}

	QPainter painter(this);
	painter.setFont(QFont("monospace"));

	if (modelUpdated_) {
		QFontMetrics fm = painter.fontMetrics();
		textYOffset_ = 0.5 * fm.xHeight() + 1.0;
		leftMargin_ = MARGIN + TEXT_MARGIN + fm.horizontalAdvance(yLabels[0]);

		modelUpdated_ = false;
	}

	totalWidth_ = std::ceil(leftMargin_ + graphWidth_ + 6.0 * MARGIN);
	if (totalWidth_ < MININUM_WIDTH) {
		totalWidth_ = MININUM_WIDTH;
	}
	totalHeight_ = std::ceil(2.0 * MARGIN + 3.0 * TRACK_HEIGHT + 15.0 * yStep_);
	if (totalHeight_ < MININUM_HEIGHT) {
		totalHeight_ = MININUM_HEIGHT;
	}
	setMinimumWidth(totalWidth_);
	setMinimumHeight(totalHeight_);

	double xStart = timeToX(0.0);
	double yStart = valueToY(MIN_VALUE);
	double xEnd = timeToX(maxTime_);
	double yEnd = valueToY(MAX_VALUE);

	// Y labels.
	for (unsigned int i = 0; i < yLabels.size(); ++i) {
		double y = MARGIN + (i + 3) * yStep_ + textYOffset_;
		painter.drawText(QPointF(MARGIN, y), yLabels[i]);
	}
	// Horizontal lines.
	painter.setPen(Qt::lightGray);
	for (int i = 1; i < 15; ++i) {
		double y = MARGIN + (i + 3) * yStep_;
		painter.drawLine(QPointF(xStart, y), QPointF(xEnd, y));
	}
	painter.setPen(Qt::black);

	postureTimeList_.clear();

	double yPosture = MARGIN + 3.0 * TRACK_HEIGHT - 0.5 * (TRACK_HEIGHT - 1.0) + textYOffset_;
	unsigned int postureIndex = 0;
	for (const VTMControlModel::Event_ptr& ev : eventList_->list()) {
		double x = timeToX(ev->time);
		if (ev->flag) {
			postureTimeList_.push_back(ev->time);
			const VTMControlModel::PostureData* postureData = eventList_->getPostureDataAtIndex(postureIndex++);
			if (postureData) {
				painter.setPen(Qt::black);
				// Posture name.
				if (postureData->marked) {
					painter.drawText(QPointF(x, yPosture), QString(postureData->posture->name().c_str()) + '\'');
				} else {
					painter.drawText(QPointF(x, yPosture), postureData->posture->name().c_str());
				}
			}
			painter.setPen(Qt::lightGray);
			// Event vertical line.
			painter.drawLine(QPointF(x, yStart), QPointF(x, yEnd));
		}
	}
	painter.setPen(Qt::black);

	// Frame.
	painter.drawRect(QRectF(QPointF(xStart, yStart), QPointF(xEnd, yEnd)));

	double yRuleText  = MARGIN + TRACK_HEIGHT - 0.5 * (TRACK_HEIGHT - 1.0) + textYOffset_;
	double yRuleText2 = yRuleText + TRACK_HEIGHT;

	for (int i = 0; i < eventList_->numberOfRules(); ++i) {
		auto* ruleData = eventList_->getRuleDataAtIndex(i);
		if (ruleData) {
			unsigned int firstPosture = ruleData->firstPosture;
			unsigned int lastPosture = ruleData->lastPosture;

			int postureTime1, postureTime2;
			if (firstPosture < postureTimeList_.size()) {
				postureTime1 = postureTimeList_[firstPosture];
			} else {
				postureTime1 = 0; // invalid
			}
			if (lastPosture < postureTimeList_.size()) {
				postureTime2 = postureTimeList_[lastPosture];
			} else {
				postureTime2 = postureTime1 + ruleData->duration;
			}

			// Rule frame.
			painter.drawRect(QRectF(
					QPointF(timeToX(postureTime1), MARGIN),
					QPointF(timeToX(postureTime2), MARGIN + 2.0 * TRACK_HEIGHT)));
			// Rule number.
			painter.drawText(QPointF(timeToX(postureTime1) + TEXT_MARGIN, yRuleText), QString::number(ruleData->number));

			// Rule duration.
			painter.drawText(QPointF(timeToX(postureTime1) + TEXT_MARGIN, yRuleText2), QString::number(ruleData->duration, 'f', 0));
		}
	}

	QPen pen;
	QPen pen2;
	pen2.setWidth(2);

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBrush(QBrush(Qt::black));

	// Lines between intonation points.
	QPointF prevPoint(0.5 + xStart, 0.5 + valueToY(eventList_->initialPitch()));
	for (unsigned int i = 0, size = intonationPointList_.size(); i < size; ++i) {
		QPointF currPoint(
			0.5 + timeToX(intonationPointList_[i].absoluteTime()),
			0.5 + valueToY(intonationPointList_[i].semitone()));

		painter.setPen(pen2);
		painter.drawLine(prevPoint, currPoint);

		painter.setPen(pen);
		drawPointMarker(painter, 0.5 + currPoint.x(), 0.5 + currPoint.y());

		prevPoint = currPoint;
	}
	painter.setPen(pen2);
	if (!intonationPointList_.empty()) {
		QPointF lastPoint(0.5 + xEnd, prevPoint.y());
		painter.drawLine(prevPoint, lastPoint);
	}

	painter.setBrush(QBrush{});

	smoothPoints(painter);
	painter.setPen(pen);

	painter.setRenderHint(QPainter::Antialiasing, false);

	// Point selection.
	if (selectedPoint_ >= 0) {
		double x = timeToX(intonationPointList_[selectedPoint_].absoluteTime());
		double y = valueToY(intonationPointList_[selectedPoint_].semitone());
		painter.drawRect(QRectF(
			 QPointF(x - SELECTION_SIZE, y - SELECTION_SIZE),
			 QPointF(x + SELECTION_SIZE, y + SELECTION_SIZE)));
	}

	// Beat lines.
	for (unsigned int i = 0, size = intonationPointList_.size(); i < size; ++i) {
		double x = timeToX(intonationPointList_[i].beatTime());
		painter.drawLine(QPointF(x, yStart), QPointF(x, yEnd));
	}

	// Time scale.
	double yTick1 = yStart + TIME_DIVISION_MARK_SIZE;
	double yTick2 = yStart + 2.0 * TIME_DIVISION_MARK_SIZE;
	double yTimeText = yTick2 + 0.5 * TRACK_HEIGHT + textYOffset_;
	for (int time = 0, end = static_cast<int>(maxTime_); time <= end; time += 10) {
		double x = timeToX(time);
		if (time % 100 == 0) {
			painter.drawLine(QPointF(x, yStart), QPointF(x, yTick2));
			painter.drawText(QPointF(x, yTimeText), QString::number(time));
		} else {
			painter.drawLine(QPointF(x, yStart), QPointF(x, yTick1));
		}
	}
}

void
IntonationWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	qDebug("IntonationWidget::mouseDoubleClickEvent");

	if (eventList_ == nullptr || eventList_->list().empty() || intonationPointList_.empty()) {
		return;
	}
	if (event->button() != Qt::LeftButton) return;

#ifdef USING_QT6
	QPointF clickPoint = event->position();
#else
	QPointF clickPoint = event->localPos();
#endif
	double clickTime = xToTime(clickPoint.x());
	unsigned int ruleIndex = 0;
	double minDist = 1.0e10;
	for (unsigned int i = 0, size = eventList_->numberOfRules(); i < size; ++i) {
		auto* ruleData = eventList_->getRuleDataAtIndex(i);
		double distance = std::abs(clickTime - ruleData->beat);
		if (distance <= minDist) {
			minDist = distance;
			ruleIndex = i;
		} else {
			break;
		}
	}

	auto* ruleData = eventList_->getRuleDataAtIndex(ruleIndex);

	VTMControlModel::IntonationPoint newPoint(eventList_);
	newPoint.setRuleIndex(ruleIndex);
	newPoint.setOffsetTime(clickTime - ruleData->beat);
	newPoint.setSemitone(yToValue(clickPoint.y()));

	selectedPoint_ = addIntonationPoint(newPoint);
	qDebug("selectedPoint_ = %d", selectedPoint_);

	sendSelectedPointData();
	update();
}

void
IntonationWidget::mousePressEvent(QMouseEvent* event)
{
	if (eventList_ == nullptr || eventList_->list().empty() || intonationPointList_.empty()) {
		return;
	}
	if (event->button() != Qt::LeftButton) return;

#ifdef USING_QT6
	QPointF clickPoint = event->position();
#else
	QPointF clickPoint = event->localPos();
#endif
	double minDist = 1.0e10;
	for (unsigned int i = 0, size = intonationPointList_.size(); i < size; ++i) {
		QPointF point(
			timeToX(intonationPointList_[i].absoluteTime()),
			valueToY(intonationPointList_[i].semitone()));

		// Simplified "distance".
		double dist = std::abs(point.x() - clickPoint.x()) + std::abs(point.y() - clickPoint.y());
		if (dist < minDist) {
			minDist = dist;
			selectedPoint_ = i;
		}
	}
	sendSelectedPointData();

	update();
}

void
IntonationWidget::keyPressEvent(QKeyEvent* event)
{
	if (eventList_ == nullptr || eventList_->list().empty() ||
			intonationPointList_.empty() || selectedPoint_ < 0) {
		QWidget::keyPressEvent(event);
		return;
	}

	switch (event->key()) {
	case Qt::Key_Delete:
		intonationPointList_.erase(intonationPointList_.begin() + selectedPoint_);
		selectedPoint_ = -1;
		break;
	case Qt::Key_Up:
		intonationPointList_[selectedPoint_].setSemitone(
			std::min(
				MAX_VALUE,
				intonationPointList_[selectedPoint_].semitone() + 1.0));
		break;
	case Qt::Key_Down:
		intonationPointList_[selectedPoint_].setSemitone(
			std::max(
				MIN_VALUE,
				intonationPointList_[selectedPoint_].semitone() - 1.0));
		break;
	case Qt::Key_Left:
		if (intonationPointList_[selectedPoint_].ruleIndex() == 0) {
			return;
		}
		{
			auto intonationPoint = intonationPointList_[selectedPoint_];
			intonationPointList_.erase(intonationPointList_.begin() + selectedPoint_);
			intonationPoint.setRuleIndex(intonationPoint.ruleIndex() - 1);
			selectedPoint_ = addIntonationPoint(intonationPoint);
		}
		break;
	case Qt::Key_Right:
		if (intonationPointList_[selectedPoint_].ruleIndex() == eventList_->numberOfRules() - 1) {
			return;
		}
		{
			auto intonationPoint = intonationPointList_[selectedPoint_];
			intonationPointList_.erase(intonationPointList_.begin() + selectedPoint_);
			intonationPoint.setRuleIndex(intonationPoint.ruleIndex() + 1);
			selectedPoint_ = addIntonationPoint(intonationPoint);
		}
		break;
	default:
		QWidget::keyPressEvent(event);
		return;
	}

	sendSelectedPointData();
	update();
}

QSize
IntonationWidget::sizeHint() const
{
	return QSize(totalWidth_, totalHeight_);
}

void
IntonationWidget::updateData(VTMControlModel::EventList* eventList)
{
	eventList_ = eventList;

	modelUpdated_ = true;

	update();
}

void
IntonationWidget::setSelectedPointValue(double value)
{
	if (eventList_ == nullptr || intonationPointList_.empty() || selectedPoint_ < 0) return;

	bool changedValue = false;
	if (value > MAX_VALUE) {
		value = MAX_VALUE;
		changedValue = true;
	} else if (value < MIN_VALUE) {
		value = MIN_VALUE;
		changedValue = true;
	}

	intonationPointList_[selectedPoint_].setSemitone(value);
	if (changedValue) {
		sendSelectedPointData();
	}
	update();
}

void
IntonationWidget::setSelectedPointSlope(double slope)
{
	if (eventList_ == nullptr || intonationPointList_.empty() || selectedPoint_ < 0) return;

	bool changedValue = false;
	if (slope > MAX_SLOPE) {
		slope = MAX_SLOPE;
		changedValue = true;
	} else if (slope < -MAX_SLOPE) {
		slope = -MAX_SLOPE;
		changedValue = true;
	}

	intonationPointList_[selectedPoint_].setSlope(slope);
	if (changedValue) {
		sendSelectedPointData();
	}
	update();
}

void
IntonationWidget::setSelectedPointBeatOffset(double beatOffset)
{
	if (eventList_ == nullptr || intonationPointList_.empty() || selectedPoint_ < 0) return;

	double beat = intonationPointList_[selectedPoint_].beatTime();
	double time = beat + beatOffset;

	if (time > maxTime_) {
		beatOffset = maxTime_ - beat;
	} else if (time < 0.0) {
		beatOffset = -beat;
	}

	intonationPointList_[selectedPoint_].setOffsetTime(beatOffset);
	sendSelectedPointData();
	update();
}

bool
IntonationWidget::saveIntonationToEventList()
{
	if (eventList_ == nullptr || intonationPointList_.empty()) return false;

	eventList_->intonationPoints() = intonationPointList_;
	return true;
}

// Slot.
void
IntonationWidget::loadIntonationFromEventList()
{
	if (eventList_ == nullptr || eventList_->list().empty()) return;

	qDebug("IntonationWidget::loadIntonationFromEventList");

	maxTime_ = eventList_->list().back()->time;
	graphWidth_ = maxTime_ * timeScale_;

	intonationPointList_ = eventList_->intonationPoints();
	if (selectedPoint_ >= static_cast<int>(intonationPointList_.size())) {
		selectedPoint_ = -1;
	}
	sendSelectedPointData();

	update();
}

double
IntonationWidget::valueToY(double value)
{
	return MARGIN + 3.0 * TRACK_HEIGHT + (5.0 - value / VALUE_STEP) * yStep_;
}

double
IntonationWidget::timeToX(double time)
{
	return leftMargin_ + graphWidth_ * time / maxTime_;
}

double
IntonationWidget::yToValue(double y)
{
	return ((y - MARGIN - 3.0 * TRACK_HEIGHT) / yStep_ - 5.0) * -VALUE_STEP;
}

double
IntonationWidget::xToTime(double x)
{
	return (x - leftMargin_) * maxTime_ / graphWidth_;
}

void
IntonationWidget::drawPointMarker(QPainter& painter, double x, double y)
{
	painter.drawEllipse(QPointF(x, y), MARKER_SIZE, MARKER_SIZE);
}

void
IntonationWidget::smoothPoints(QPainter& painter)
{
	if (intonationPointList_.size() < 2U) return;

	for (unsigned int i = 0, end = intonationPointList_.size() - 1; i < end; ++i) {
		const auto& point1 = intonationPointList_[i];
		const auto& point2 = intonationPointList_[i + 1];

		double x1 = point1.absoluteTime();
		double y1 = point1.semitone();
		double m1 = point1.slope();

		double x2 = point2.absoluteTime();
		double y2 = point2.semitone();
		double m2 = point2.slope();

		double x12 = x1  * x1;
		double x13 = x12 * x1;

		double x22 = x2  * x2;
		double x23 = x22 * x2;

		double denominator = x2 - x1;
		denominator = denominator * denominator * denominator;

		double d = (-(y2 * x13) + 3.0 * y2 * x12 * x2 + m2 * x13 * x2 + m1 * x12 * x22 - m2 * x12 * x22 - 3.0 * x1 * y1 * x22 - m1 * x1 * x23 + y1 * x23)
			/ denominator;
		double c = (-(m2 * x13) - 6.0 * y2 * x1 * x2 - 2.0 * m1 * x12 * x2 - m2 * x12 * x2 + 6.0 * x1 * y1 * x2 + m1 * x1 * x22 + 2.0 * m2 * x1 * x22 + m1 * x23)
			/ denominator;
		double b = (3.0 * y2 * x1 + m1 * x12 + 2.0 * m2 * x12 - 3.0 * x1 * y1 + 3.0 * x2 * y2 + m1 * x1 * x2 - m2 * x1 * x2 - 3.0 * y1 * x2 - 2.0 * m1 * x22 - m2 * x22)
			/ denominator;
		double a = (-2.0 * y2 - m1 * x1 - m2 * x1 + 2.0 * y1 + m1 * x2 + m2 * x2) / denominator;

		QPointF prevPoint(0.5 + timeToX(x1), 0.5 + valueToY(y1));

		for (unsigned int j = static_cast<unsigned int>(x1); j <= static_cast<unsigned int>(x2); j += SMOOTH_POINTS_X_INCREMENT) {
			double x = j;
			double y = x * (x * (x * a + b) + c) + d;

			QPointF currPoint(0.5 + timeToX(x), 0.5 + valueToY(y));

			painter.drawLine(prevPoint, currPoint);

			prevPoint = currPoint;
		}

		QPointF lastPoint(0.5 + timeToX(x2), 0.5 + valueToY(y2));
		painter.drawLine(prevPoint, lastPoint);
	}
}

// Returns the insertion index.
int
IntonationWidget::addIntonationPoint(VTMControlModel::IntonationPoint& newPoint)
{
	qDebug("eventList_->numberOfRules() = %d ruleIndex = %d", eventList_->numberOfRules(), newPoint.ruleIndex());

	if (newPoint.ruleIndex() >= eventList_->numberOfRules()) {
		qWarning("[IntonationWidget::addIntonationPoint] newPoint.ruleIndex() >= eventList_->numberOfRules()");
		return -1;
	}

	double time = newPoint.absoluteTime();
	for (unsigned int i = 0, size = intonationPointList_.size(); i < size; ++i) {
		if (time < intonationPointList_[i].absoluteTime()) {
			intonationPointList_.insert(intonationPointList_.begin() + i, newPoint);
			return i;
		}
	}
	intonationPointList_.push_back(newPoint);
	return intonationPointList_.size() - 1;
}

void
IntonationWidget::sendSelectedPointData()
{
	if (eventList_ == nullptr) return;

	if (intonationPointList_.empty() || selectedPoint_ < 0) {
		emit pointSelected(0.0, 0.0, 0.0, 0.0, 0.0);
		return;
	}

	const auto& point = intonationPointList_[selectedPoint_];
	emit pointSelected(point.semitone(), point.slope(), point.beatTime(), point.offsetTime(), point.absoluteTime());
}

} // namespace GS
