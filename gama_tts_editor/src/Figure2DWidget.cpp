/***************************************************************************
 *  Copyright 2023 Marcelo Y. Matuda                                       *
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

#include "Figure2DWidget.h"

#include <algorithm> /* max_element, min_element */
#include <cmath> /* abs, ceil, floor, log10, max, min, pow */

#include <QCursor>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QRectF>

namespace {

constexpr int SPACING = 3;
constexpr int TEXT_SPACING = 3;
constexpr int TICK_SIZE = 3;
constexpr int MIN_AXIS_DIV = 4;
constexpr int MIN_X_TICKS_MARGIN = 6;
constexpr unsigned int MAX_X_LIST_SIZE_WITH_MARKER = 1000;
constexpr double WHEEL_ZOOM_FACTOR = 0.01;
constexpr double MOUSE_ZOOM_FACTOR = 0.002;
constexpr double DEFAULT_Y_DELTA = 1.0;
constexpr double MIN_ABS_VALUE_DELTA = 1.0e-9;
constexpr double MIN_VALUE_DELTA = 1.0e-5;
constexpr double MAX_VALUE_DELTA = 1.0e5;
constexpr double MARKER_SIZE = 2.0;
constexpr double POINT_MARKER_SIZE = 1.0;
constexpr double EPS = 1.0e-5;
constexpr double MAX_TICK_POW = 2.0;
constexpr double MIN_TICK_POW = -2.0;

}

namespace Lab {

Figure2DWidget::Figure2DWidget(QWidget* parent)
		: QWidget(parent)
		, figureChanged_()
		, drawCurveLines_(true)
		, drawPointMarker_()
		, expandAxisTicks_(true)
		, symmetricYRange_()
		, reduceYRange_(true)
		, leftMargin_()
		, rightMargin_()
		, topMargin_()
		, bottomMargin_()
		, xLabelWidth_()
		, yLabelWidth_()
		, textCapHeight_()
		, mainAreaWidth_()
		, mainAreaHeight_()
		, xScale_()
		, yScale_()
		, xBegin_()
		, xEnd_()
		, yBegin_()
		, yEnd_()
		, xBeginData_()
		, xEndData_()
		, yBeginData_()
		, yEndData_()
		, xTickCoef_(1.0)
		, yTickCoef_(1.0)
		, lastXBegin_()
		, lastXEnd_()
		, lastXScale_()
		, xLabel_("x")
		, yLabel_("y")
		, xCursorIndex_(-1)
		, maxXTickWidth_()
		, locale_(QLocale::system())
{
	setAutoFillBackground(true);
	setBackgroundRole(QPalette::Base);

	setFocusPolicy(Qt::StrongFocus); // enable key events
	setMouseTracking(true);
	setCursor(Qt::CrossCursor);
}

// Note: with no antialiasing, the coordinates in QPointF are rounded to the nearest integer.
void
Figure2DWidget::paintEvent(QPaintEvent* /*event*/)
{
	if (xList_.empty() || xTicks_.size() < 2 || yTicks_.size() < 2) return;
	if (xList_.size() > MAX_X_LIST_SIZE_WITH_MARKER) drawPointMarker_ = false;

	QPainter painter(this);
#ifdef _MSC_VER
	QFont monoFont("Consolas");
	monoFont.setStyleHint(QFont::Monospace);
#else
	QFont monoFont("monospace");
#endif
	painter.setFont(monoFont);

	auto formatTickValue = [&](double value, int precision) {
		return locale_.toString(value, 'f', precision);
	};
	auto tickPrecision = [](double step) {
		const int stepPow = static_cast<int>(std::floor(std::log10(step) + 1.0e-5));
		return (stepPow < 0) ? -stepPow : 0;
	};
	const int xPrec = tickPrecision(xTicks_[1] - xTicks_[0]);
	const int yPrec = tickPrecision(yTicks_[1] - yTicks_[0]);

	QFontMetrics fm = painter.fontMetrics();

	if (figureChanged_) {
		textCapHeight_ = fm.capHeight();

		yTicksWidth_.resize(yTicks_.size());
		int maxW = 0;
		for (unsigned int i = 0; i < yTicks_.size(); ++i) {
			yTicksWidth_[i] = fm.horizontalAdvance(formatTickValue(yTicks_[i], yPrec));
			if (yTicksWidth_[i] > maxW) maxW = yTicksWidth_[i];
		}
		maxW = std::max(maxW, fm.horizontalAdvance("00000"));
		leftMargin_ = SPACING + textCapHeight_ + 2 * TEXT_SPACING + maxW + TICK_SIZE;

		rightMargin_ = fm.horizontalAdvance("0") + SPACING;
		topMargin_ = textCapHeight_ + 2 * SPACING;
		bottomMargin_ = TICK_SIZE + 2 * TEXT_SPACING + 2 * textCapHeight_ + SPACING;
		xLabelWidth_ = fm.horizontalAdvance(xLabel_);
		yLabelWidth_ = fm.horizontalAdvance(yLabel_);

		maxXTickWidth_ = 0;
		for (unsigned int i = 0; i < xTicks_.size(); ++i) {
			maxXTickWidth_ = std::max(maxXTickWidth_, fm.horizontalAdvance(formatTickValue(xTicks_[i], xPrec)));
		}

		handleTransform();

		figureChanged_ = false;
	}

	const double uBegin = leftMargin_;
	const double vBegin = topMargin_ + mainAreaHeight_;
	const double uEnd = leftMargin_ + mainAreaWidth_;
	const double vEnd = topMargin_;

	// X axis.
	const double vTick = vBegin + TICK_SIZE;
	const double vText = vTick + TEXT_SPACING + textCapHeight_;
	double uOld = 0;
	for (unsigned int i = 0; i < xTicks_.size(); ++i) {
		const double u = uBegin + (xTicks_[i] * xTickCoef_ - xBegin_) * xScale_;
		if (i > 0 && u - uOld < maxXTickWidth_ + MIN_X_TICKS_MARGIN) continue;

		// Vertical line.
		painter.setPen(Qt::lightGray);
		painter.drawLine(QPointF(u, vBegin), QPointF(u, vEnd));
		painter.setPen(Qt::black);
		// Tick.
		painter.drawLine(QPointF(u, vBegin), QPointF(u, vTick));
		// Tick value.
		QString tickValue = formatTickValue(xTicks_[i], xPrec);
		if (i < xTicks_.size() - 1U || u + fm.horizontalAdvance(tickValue) < uEnd + rightMargin_) {
			painter.drawText(QPointF(u, vText), tickValue);
		}

		uOld = u;
	}
	// X label.
	const double vLabelPos = vText + TEXT_SPACING + textCapHeight_;
	//painter.drawText(QPointF(uBegin + mainAreaWidth_ / 2 - xLabelWidth_ / 2, vLabelPos), xLabel_);
	painter.drawText(QPointF(uBegin + mainAreaWidth_ - xLabelWidth_, vLabelPos), xLabel_);
	// X ticks transformation.
	QString xTickTransf;
	//if (xTickCoef_ != 1.0) xTickTransf += QString("x1e%1").arg(std::log10(xTickCoef_));
	if (xTickCoef_ != 1.0) xTickTransf += QString("x%L1").arg(xTickCoef_, 0, 'g', 10);
	if (!xTickTransf.isEmpty()) {
		painter.drawText(QPointF(uBegin, vLabelPos), xTickTransf);
	}

	// Y axis.
	const double uTick = uBegin - TICK_SIZE;
	for (unsigned int i = 0; i < yTicks_.size(); ++i) {
		const double uText = uTick - TEXT_SPACING - yTicksWidth_[i];
		const double v = vBegin + (yTicks_[i] * yTickCoef_ - yBegin_) * yScale_;
		// Horizontal line.
		painter.setPen(Qt::lightGray);
		painter.drawLine(QPointF(uBegin, v), QPointF(uEnd, v));
		painter.setPen(Qt::black);
		// Tick.
		painter.drawLine(QPointF(uBegin, v), QPointF(uTick, v));
		// Tick value.
		painter.drawText(QPointF(uText, v), formatTickValue(yTicks_[i], yPrec));
	}
	// Y label.
	painter.rotate(-90.0);
	//painter.drawText(QPointF(-vBegin + mainAreaHeight_ / 2 - yLabelWidth_ / 2, SPACING + textCapHeight_), yLabel_);
	painter.drawText(QPointF(-vBegin + mainAreaHeight_ - yLabelWidth_, SPACING + textCapHeight_), yLabel_);
	// Y ticks transformation.
	QString yTickTransf;
	//if (yTickCoef_ != 1.0) yTickTransf += QString("x1e%1").arg(std::log10(yTickCoef_));
	if (yTickCoef_ != 1.0) yTickTransf += QString("x%L1").arg(yTickCoef_, 0, 'g', 10);
	if (!yTickTransf.isEmpty()) {
		painter.drawText(QPointF(-vBegin, SPACING + textCapHeight_), yTickTransf);
	}
	painter.resetTransform();

	// Frame.
	painter.drawRect(QRectF(QPointF(uBegin, vBegin), QPointF(uEnd, vEnd)));

	QPoint pCursor = mapFromGlobal(QCursor::pos());
	if (rect().contains(pCursor)) {
		const double x = xBegin_ + (pCursor.x() - uBegin) / xScale_;
		const double y = yBegin_ + (pCursor.y() - vBegin) / yScale_;
		if (x >= xBegin_ && x <= xEnd_ && y >= yBegin_ && y <= yEnd_) {
			painter.drawText(QPointF(uBegin, vEnd - SPACING), QString("%L1 %L2").arg(x).arg(y));
		}
	}

	// Cursor.
	if (xCursorIndex_ >= 0 && static_cast<unsigned int>(xCursorIndex_) < xList_.size()) {
		QPen cursorPen;
		cursorPen.setColor(Qt::red);
		painter.setPen(cursorPen);
		const double x = uBegin + (xList_[xCursorIndex_] - xBegin_) * xScale_;
		if (x >= uBegin && x <= uEnd) {
			painter.drawLine(QPointF(x, vBegin), QPointF(x, vEnd));
		}
	}

	QPen pen2;
	pen2.setWidth(2);

	painter.setRenderHint(QPainter::Antialiasing);
	// When using antialiasing, 0.5 is added to (x, y) to match the case without antialiasing.

	// Draw curve.
	painter.setClipRect(uBegin, vEnd, mainAreaWidth_, mainAreaHeight_);
	painter.setPen(pen2);
	if (drawCurveLines_) {
		QPointF prevPoint(
				0.5 + uBegin + (xList_[0] - xBegin_) * xScale_,
				0.5 + vBegin + (yList_[0] - yBegin_) * yScale_);
		if (drawPointMarker_) {
			painter.drawEllipse(prevPoint, MARKER_SIZE, MARKER_SIZE);
		}
		for (unsigned int i = 1, size = xList_.size(); i < size; ++i) {
			QPointF currPoint(
				0.5 + uBegin + (xList_[i] - xBegin_) * xScale_,
				0.5 + vBegin + (yList_[i] - yBegin_) * yScale_);
			painter.drawLine(prevPoint, currPoint);
			if (drawPointMarker_) {
				painter.drawEllipse(currPoint, MARKER_SIZE, MARKER_SIZE);
			}
			prevPoint = currPoint;
		}
	} else {
		for (unsigned int i = 0, size = xList_.size(); i < size; ++i) {
			QPointF currPoint(
				0.5 + uBegin + (xList_[i] - xBegin_) * xScale_,
				0.5 + vBegin + (yList_[i] - yBegin_) * yScale_);
			painter.drawEllipse(currPoint, POINT_MARKER_SIZE, POINT_MARKER_SIZE);
		}
	}
	// Draw curve 2.
	if (!x2List_.empty()) {
		QPen pen3;
		pen3.setColor(Qt::blue);
		pen3.setWidth(2);
		painter.setPen(pen3);
		if (drawCurveLines_) {
			QPointF prevPoint(
					0.5 + uBegin + (x2List_[0] - xBegin_) * xScale_,
					0.5 + vBegin + (y2List_[0] - yBegin_) * yScale_);
			if (drawPointMarker_) {
				painter.drawEllipse(prevPoint, MARKER_SIZE, MARKER_SIZE);
			}
			for (unsigned int i = 1, size = x2List_.size(); i < size; ++i) {
				QPointF currPoint(
					0.5 + uBegin + (x2List_[i] - xBegin_) * xScale_,
					0.5 + vBegin + (y2List_[i] - yBegin_) * yScale_);
				painter.drawLine(prevPoint, currPoint);
				if (drawPointMarker_) {
					painter.drawEllipse(currPoint, MARKER_SIZE, MARKER_SIZE);
				}
				prevPoint = currPoint;
			}
		} else {
			for (unsigned int i = 0, size = x2List_.size(); i < size; ++i) {
				QPointF currPoint(
					0.5 + uBegin + (x2List_[i] - xBegin_) * xScale_,
					0.5 + vBegin + (y2List_[i] - yBegin_) * yScale_);
				painter.drawEllipse(currPoint, POINT_MARKER_SIZE, POINT_MARKER_SIZE);
			}
		}
	}

	painter.setClipping(false);
}

void
Figure2DWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (xList_.empty() || xTicks_.empty()) return;
	if (event->button() != Qt::LeftButton) return;

	resetFigure(true);
}

void
Figure2DWidget::mousePressEvent(QMouseEvent* event)
{
	if (xList_.empty() || xTicks_.empty()) return;

#ifdef USING_QT6
	lastMousePos_ = event->position();
#else
	lastMousePos_ = event->localPos();
#endif
	lastXBegin_ = xBegin_;
	lastXEnd_   = xEnd_;
	lastXScale_ = xScale_;
}

void
Figure2DWidget::mouseMoveEvent(QMouseEvent* event)
{
	// Note: For mouse move events, event->button() == Qt::NoButton.

	if (xList_.empty() || xTicks_.empty()) return;

#ifdef USING_QT6
	QPointF delta = event->position() - lastMousePos_;
#else
	QPointF delta = event->localPos() - lastMousePos_;
#endif
	if (event->buttons() & Qt::LeftButton) {
		const double dx = delta.x() / xScale_;
		const double dy = delta.y() / yScale_;
		const double newXBegin = xBegin_ - dx;
		const double newXEnd   = xEnd_   - dx;
		const double newYBegin = symmetricYRange_ ? yBegin_ : yBegin_ - dy;
		const double newYEnd   = symmetricYRange_ ? yEnd_   : yEnd_   - dy;
		// The new region must contain at least part of the data.
		if (newXBegin < xEndData_ && newXEnd > xBeginData_ &&
				newYBegin < yEndData_ && newYEnd > yBeginData_) {
			xBegin_ = newXBegin;
			xEnd_   = newXEnd;
			yBegin_ = newYBegin;
			yEnd_   = newYEnd;

			autoSetAxesTicks();
		}
#ifdef USING_QT6
		lastMousePos_ = event->position();
#else
		lastMousePos_ = event->localPos();
#endif
		return;
	} else if (event->buttons() & Qt::RightButton) {
		if (delta.x() == 0.0) return;
		const double factor = 1.0 + MOUSE_ZOOM_FACTOR * std::abs(delta.x());
		const double centerX = lastXBegin_ + (lastMousePos_.x() - leftMargin_) / lastXScale_;
		const double maxAbsXValue = std::max(std::abs(xBeginData_), std::abs(xEndData_));
		double newXBegin;
		double newXEnd;
		if (delta.x() > 0.0) {
			newXBegin = centerX + (lastXBegin_ - centerX) / factor;
			newXEnd   = centerX + (lastXEnd_   - centerX) / factor;
			const double xDeltaThreshold = std::max(MIN_ABS_VALUE_DELTA, MIN_VALUE_DELTA * maxAbsXValue);
			if (newXEnd - newXBegin < xDeltaThreshold) {
				qDebug("Zoom limit.");
				return;
			}
		} else {
			newXBegin = centerX + (lastXBegin_ - centerX) * factor;
			newXEnd   = centerX + (lastXEnd_   - centerX) * factor;
			const double xDeltaThreshold = MAX_VALUE_DELTA * maxAbsXValue;
			if (newXEnd - newXBegin > xDeltaThreshold) {
				qDebug("Zoom limit.");
				return;
			}
		}
		// The new region must contain at least part of the data.
		if (newXBegin < xEndData_ && newXEnd > xBeginData_) {
			xBegin_ = newXBegin;
			xEnd_   = newXEnd;
			autoSetAxesTicks();
		}
		return;
	}

#ifdef USING_QT6
	lastMousePos_ = event->position();
#else
	lastMousePos_ = event->localPos();
#endif
	update();
}

void
Figure2DWidget::wheelEvent(QWheelEvent* event)
{
	if (xList_.empty() || xTicks_.empty()) return;

	QPoint angle = event->angleDelta() / 8; // degrees
	if (angle.y() == 0) return;
	const double factor = 1.0 + WHEEL_ZOOM_FACTOR * std::abs(angle.y());
	const double centerY = symmetricYRange_ ? 0.0 : yEnd_ + (event->position().y() - topMargin_) / yScale_;
	const double maxAbsYValue = std::max(std::abs(yBeginData_), std::abs(yEndData_));
	double newYBegin;
	double newYEnd;
	if (angle.y() > 0) {
		newYBegin = centerY + (yBegin_ - centerY) / factor;
		newYEnd   = centerY + (yEnd_   - centerY) / factor;
		const double yDeltaThreshold = std::max(MIN_ABS_VALUE_DELTA, MIN_VALUE_DELTA * maxAbsYValue);
		if (newYEnd - newYBegin < yDeltaThreshold) {
			qDebug("Zoom limit.");
			return;
		}
	} else {
		newYBegin = centerY + (yBegin_ - centerY) * factor;
		newYEnd   = centerY + (yEnd_   - centerY) * factor;
		const double yDeltaThreshold = MAX_VALUE_DELTA * maxAbsYValue;
		if (newYEnd - newYBegin > yDeltaThreshold) {
			qDebug("Zoom limit.");
			return;
		}
	}
	// The new region must contain at least part of the data.
	if (newYBegin < yEndData_ && newYEnd > yBeginData_) {
		yBegin_ = newYBegin;
		yEnd_   = newYEnd;
		autoSetAxesTicks();
	}
}

void
Figure2DWidget::resizeEvent(QResizeEvent* /*event*/)
{
	if (xList_.empty() || xTicks_.empty()) return;

	handleTransform();
}

void
Figure2DWidget::keyPressEvent(QKeyEvent* event)
{
	if (xList_.empty() || xTicks_.empty()) {
		QWidget::keyPressEvent(event);
		return;
	}

	if (event->key() == Qt::Key_Space) {
		drawPointMarker_ = !drawPointMarker_;
	} else {
		QWidget::keyPressEvent(event);
		return;
	}

	update();
}

void
Figure2DWidget::handleTransform()
{
	mainAreaWidth_  =  width() - leftMargin_ -  rightMargin_;
	mainAreaHeight_ = height() -  topMargin_ - bottomMargin_;

	xScale_ =   mainAreaWidth_ / (xEnd_ - xBegin_);
	yScale_ = -mainAreaHeight_ / (yEnd_ - yBegin_);
}

void
Figure2DWidget::autoSetAxisTicks(double minValue, double maxValue,
					std::vector<double>& ticks, double& coef,
					bool expand, bool symmetric)
{
	ticks.clear();

	if (symmetric) {
		const double maxAbs = std::max(std::abs(minValue), std::abs(maxValue));
		minValue = -maxAbs;
		maxValue =  maxAbs;
	}

	// Calculate step.
	const double range = maxValue - minValue;

	const double maxStep = range / MIN_AXIS_DIV;
	const double truncMaxStep = std::pow(10.0, std::floor(std::log10(maxStep)));
	double step = truncMaxStep;
	double aux;
	if ((aux = truncMaxStep * 5.0) < maxStep) {
		step = aux;
	} else if ((aux = truncMaxStep * 2.0) < maxStep) {
		step = aux;
	}
	//qDebug("maxStep: %f truncMaxStep: %f step: %f", maxStep, truncMaxStep, step);

	// Calculate tick values.
	double firstTick, lastTick;
	if (expand) {
		firstTick = std::floor(minValue / step) * step;
		lastTick  =  std::ceil(maxValue / step) * step;
	} else {
		firstTick =  std::ceil(minValue / step) * step;
		lastTick  = std::floor(maxValue / step) * step;
	}
	ticks.push_back(firstTick);
	for (unsigned int i = 1; ; ++i) {
		const double tick = firstTick + i * step;
		if (tick > lastTick + step * EPS) break;
		ticks.push_back(tick);
	}

	// Calculate coefficient.
	const double stepPow = std::log10(step);
	if (stepPow > MAX_TICK_POW) {
		coef = std::pow(10.0, std::ceil(stepPow) - MAX_TICK_POW);
		for (double& t : ticks) t /= coef;
	} else if (stepPow < MIN_TICK_POW) {
		coef = std::pow(10.0, std::floor(stepPow) - MIN_TICK_POW);
		for (double& t : ticks) t /= coef;
	} else {
		coef = 1.0;
	}
}

void
Figure2DWidget::autoSetAxesTicks(bool expand)
{
	autoSetAxisTicks(xBegin_, xEnd_, xTicks_, xTickCoef_, expand, false);
	autoSetAxisTicks(yBegin_, yEnd_, yTicks_, yTickCoef_, expand, symmetricYRange_);

	figureChanged_ = true;
	update();
}

bool
Figure2DWidget::resetFigure(bool reduceYRange)
{
	// The values may not be sorted.
	xEndData_   = *std::max_element(xList_.begin(), xList_.end());
	xBeginData_ = *std::min_element(xList_.begin(), xList_.end());
	yEndData_   = *std::max_element(yList_.begin(), yList_.end());
	yBeginData_ = *std::min_element(yList_.begin(), yList_.end());
	if (!x2List_.empty()) {
		xEndData_   = std::max(static_cast<float>(xEndData_  ), *std::max_element(x2List_.begin(), x2List_.end()));
		xBeginData_ = std::min(static_cast<float>(xBeginData_), *std::min_element(x2List_.begin(), x2List_.end()));
		yEndData_   = std::max(static_cast<float>(yEndData_  ), *std::max_element(y2List_.begin(), y2List_.end()));
		yBeginData_ = std::min(static_cast<float>(yBeginData_), *std::min_element(y2List_.begin(), y2List_.end()));
	}

	const double xValueDelta = xEndData_ - xBeginData_;
	const double maxAbsXValue = std::max(std::abs(xBeginData_), std::abs(xEndData_));
	const double xDeltaThreshold = std::max(MIN_ABS_VALUE_DELTA, MIN_VALUE_DELTA * maxAbsXValue);
	if (xValueDelta < xDeltaThreshold) {
		std::cerr << "[Figure2DWidget::resetFigure] Invalid x range (< " << xDeltaThreshold << ")." << std::endl;
		return false;
	}
	xBegin_ = xBeginData_;
	xEnd_   = xEndData_;

	const double maxAbsYValue = std::max(std::abs(yBeginData_), std::abs(yEndData_));
	const double newYBegin = symmetricYRange_ ? -maxAbsYValue : yBeginData_;
	const double newYEnd   = symmetricYRange_ ?  maxAbsYValue : yEndData_;
	if (reduceYRange) {
		yBegin_ = newYBegin;
		yEnd_   = newYEnd;
	} else {
		yBegin_ = std::min(yBegin_, newYBegin);
		yEnd_   = std::max(yEnd_  , newYEnd  );
	}
	const double yValueDelta = yEnd_ - yBegin_;
	const double yDeltaThreshold = std::max(MIN_ABS_VALUE_DELTA, MIN_VALUE_DELTA * maxAbsYValue);
	if (yValueDelta < yDeltaThreshold) {
		yBegin_ -= DEFAULT_Y_DELTA;
		yEnd_   += DEFAULT_Y_DELTA;
	}

	autoSetAxesTicks(symmetricYRange_ ? false : expandAxisTicks_);

	xBegin_ = std::min(xBegin_, xTicks_.front() * xTickCoef_);
	xEnd_   = std::max(xEnd_  , xTicks_.back()  * xTickCoef_);
	yBegin_ = std::min(yBegin_, yTicks_.front() * yTickCoef_);
	yEnd_   = std::max(yEnd_  , yTicks_.back()  * yTickCoef_);

	figureChanged_ = true;
	update();
	return true;
}

void
Figure2DWidget::setXLabel(QString label)
{
	xLabel_ = label;
	figureChanged_ = true;
	update();
}

void
Figure2DWidget::setYLabel(QString label)
{
	yLabel_ = label;
	figureChanged_ = true;
	update();
}

} // namespace Lab
