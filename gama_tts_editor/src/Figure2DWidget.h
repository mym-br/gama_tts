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

#ifndef FIGURE_2D_WIDGET_H
#define FIGURE_2D_WIDGET_H

#include <cstddef> /* std::size_t */
#include <iostream>
#include <vector>

#include <QLocale>
#include <QPointF>
#include <QString>
#include <QWidget>

namespace Lab {

class Figure2DWidget : public QWidget {
	Q_OBJECT
public:
	explicit Figure2DWidget(QWidget* parent=0);

	// If maxSize >= 2, use only maxSize elements.
	template<typename T> void updateData(const std::vector<T>& x, const std::vector<T>& y, std::size_t maxSize=0);
	template<typename T> void updateData2(const std::vector<T>& x, const std::vector<T>& y, std::size_t maxSize=0);
	void setXLabel(QString label);
	void setYLabel(QString label);
	void setDrawCurveLines(bool enable) { drawCurveLines_ = enable; }
	void enablePointMarker(bool enable) { drawPointMarker_ = enable; }
	void setExpandAxisTicks(bool value) { expandAxisTicks_ = value; }
	void setSymmetricYRange(bool value) { symmetricYRange_ = value; }
	void setReduceYRange(bool value) { reduceYRange_ = value; }
	void setXCursorIndex(int value) { xCursorIndex_ = value; }
	void resetYRange() { yBegin_ = 0.0; yEnd_ = 0.0; }
	void clear() {
		xList_.clear();
		yList_.clear();
		x2List_.clear();
		y2List_.clear();
		update();
	}
protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void wheelEvent(QWheelEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
private:
	void handleTransform();
	void autoSetAxesTicks(bool expand=false);
	bool resetFigure(bool reduceYRange=true);

	static void autoSetAxisTicks(double minValue, double maxValue,
					std::vector<double>& ticks, double& coef,
					bool expand, bool symmetric);

	bool figureChanged_;
	bool drawCurveLines_;
	bool drawPointMarker_;
	bool expandAxisTicks_;
	bool symmetricYRange_;
	bool reduceYRange_; // if false, updateData() will only extend the range
	int leftMargin_;
	int rightMargin_;
	int topMargin_;
	int bottomMargin_;
	int xLabelWidth_;
	int yLabelWidth_;
	int textCapHeight_;
	int mainAreaWidth_;
	int mainAreaHeight_;
	double xScale_;
	double yScale_;
	double xBegin_;
	double xEnd_;
	double yBegin_;
	double yEnd_;
	double xBeginData_;
	double xEndData_;
	double yBeginData_;
	double yEndData_;
	double xTickCoef_;
	double yTickCoef_;
	double lastXBegin_;
	double lastXEnd_;
	double lastXScale_;
	std::vector<float> xList_;
	std::vector<float> yList_;
	std::vector<float> x2List_;
	std::vector<float> y2List_;
	std::vector<double> xTicks_;
	std::vector<double> yTicks_;
	std::vector<int> yTicksWidth_;
	QString xLabel_;
	QString yLabel_;
	QPointF lastMousePos_;
	int xCursorIndex_;
	int maxXTickWidth_;
	QLocale locale_;
};

template<typename T>
void
Figure2DWidget::updateData(const std::vector<T>& x, const std::vector<T>& y, std::size_t maxSize)
{
	xList_.clear();
	yList_.clear();
	update();
	if (x.size() != y.size()) {
		std::cerr << "[Figure2DWidget::updateData] Arrays x and y with different sizes." << std::endl;
		return;
	}
	if (x.size() < 2U) {
		std::cerr << "[Figure2DWidget::updateData] Arrays x and y are too small." << std::endl;
		return;
	}

	if (maxSize < 2U || maxSize > x.size()) {
		maxSize = x.size();
	}
	xList_.resize(maxSize);
	yList_.resize(maxSize);
	for (unsigned int i = 0; i < maxSize; ++i) {
		xList_[i] = x[i];
		yList_[i] = y[i];
	}

	if (!resetFigure(reduceYRange_)) {
		xList_.clear();
		yList_.clear();
		update();
	}
}

template<typename T>
void
Figure2DWidget::updateData2(const std::vector<T>& x, const std::vector<T>& y, std::size_t maxSize)
{
	x2List_.clear();
	y2List_.clear();
	update();
	if (x.size() != y.size()) {
		std::cerr << "[Figure2DWidget::updateData2] Arrays x and y with different sizes." << std::endl;
		return;
	}
	if (x.size() < 2U) {
		std::cerr << "[Figure2DWidget::updateData2] Arrays x and y are too small." << std::endl;
		return;
	}

	if (maxSize < 2U || maxSize > x.size()) {
		maxSize = x.size();
	}
	x2List_.resize(maxSize);
	y2List_.resize(maxSize);
	for (unsigned int i = 0; i < maxSize; ++i) {
		x2List_[i] = x[i];
		y2List_[i] = y[i];
	}

	if (!resetFigure(reduceYRange_)) {
		x2List_.clear();
		y2List_.clear();
		update();
	}
}

} // namespace Lab

#endif // FIGURE_2D_WIDGET_H
