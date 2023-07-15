/***************************************************************************
 *  Copyright 2014, 2015 Marcelo Y. Matuda                                 *
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

#include "TransitionPoint.h"

#include <algorithm> /* sort */
#include <utility> /* move, swap */

#include "Exception.h"
#include "Model.h"



namespace GS {

void
TransitionPoint::copyPointsFromTransition(
		const VTMControlModel::Transition& transition,
		std::vector<TransitionPoint>& pointList)
{
	pointList.clear();

	for (auto& pointOrSlope : transition.pointOrSlopeList()) {
		if (pointOrSlope->isSlopeRatio()) {
			const auto& slopeRatio = dynamic_cast<const VTMControlModel::Transition::SlopeRatio&>(*pointOrSlope);

			for (unsigned int i = 0, size = slopeRatio.pointList.size(); i < size; ++i) {
				const auto& point = *slopeRatio.pointList[i];

				TransitionPoint tp;
				tp.type = point.type;
				tp.value = point.value;
				tp.timeExpression = point.timeExpression;
				tp.freeTime = point.freeTime;
				if (i != size - 1) {
					tp.hasSlope = true;
					tp.slope = slopeRatio.slopeList[i]->slope;
				}

				pointList.push_back(std::move(tp));
			}
		} else {
			const auto& point = dynamic_cast<const VTMControlModel::Transition::Point&>(*pointOrSlope);

			TransitionPoint tp;
			tp.type = point.type;
			tp.value = point.value;
			tp.timeExpression = point.timeExpression;
			tp.freeTime = point.freeTime;

			pointList.push_back(std::move(tp));
		}
	}
}

void
TransitionPoint::sortPointListByTypeAndTime(std::vector<TransitionPoint>& pointList)
{
	std::sort(pointList.begin(), pointList.end(), [](const TransitionPoint& p1, const TransitionPoint& p2) -> bool {
		return p1.type < p2.type || (p1.type == p2.type && p1.time < p2.time);
	});
}

void
TransitionPoint::sortPointListByTime(std::vector<TransitionPoint>& pointList)
{
	std::sort(pointList.begin(), pointList.end(), [](const TransitionPoint& p1, const TransitionPoint& p2) -> bool {
		return p1.time < p2.time;
	});
}

void
TransitionPoint::calculateTimes(const VTMControlModel::Model& model, std::vector<TransitionPoint>& pointList)
{
	for (auto& point : pointList) {
		const auto timeExpression = point.timeExpression.lock();
		if (!timeExpression) {
			point.time = point.freeTime;
		} else {
			point.time = model.evalEquationFormula(*timeExpression);
		}
	}
}

void
TransitionPoint::adjustValuesInSlopeRatios(std::vector<TransitionPoint>& pointList)
{
	if (pointList.empty()) {
		return;
	}

	for (unsigned int i = 0, end = pointList.size() - 1; i < end; ++i) {
		if (pointList[i].hasSlope) {
			// Find the last point in the slope ratio group.
			unsigned int j = i + 1;
			while (j < end && pointList[i].type == pointList[j].type && pointList[j].hasSlope) {
				++j;
			}
			if (pointList[i].type != pointList[j].type) {
				--j;
			}

			if (j - i < 2U) { // not enough points in the group
				i = j;
				continue;
			}
			pointList[j].hasSlope = false;
			pointList[j].slope = 0.0;

			float valueDeltaSum = 0.0;
			for (unsigned int k = i; k < j; ++k) {
				const float timeDelta = pointList[k + 1].time - pointList[k].time;
				const float valueDelta = pointList[k].slope * timeDelta;
				valueDeltaSum += valueDelta;
				if (k < j - 1) {
					pointList[k + 1].value = valueDelta; // temporary value
				}
			}
			if (valueDeltaSum == 0.0) { // all slopes are zero
				i = j;
				continue;
			}
			const float factor = (pointList[j].value - pointList[i].value) / valueDeltaSum;
			float previousValue = pointList[i].value;
			for (unsigned int k = i + 1; k < j; ++k) {
				pointList[k].value *= factor;
				pointList[k].value += previousValue;
				previousValue = pointList[k].value;
			}
			i = j;
		}
	}
}

// May throw GS::Exception.
void
TransitionPoint::copyPointsToTransition(VTMControlModel::Transition::Type type, const std::vector<TransitionPoint>& pointList, VTMControlModel::Transition& transition)
{
	if (pointList.empty()) {
		THROW_EXCEPTION(InvalidParameterException, "Empty point list.");
	}

	VTMControlModel::Transition newTransition(transition.name(), type, transition.special());
	newTransition.setComment(transition.comment());

	for (unsigned int i = 0, size = pointList.size(); i < size; ++i) {
		if (static_cast<int>(pointList[i].type) > static_cast<int>(type)) continue;
		if (i != size - 1 && pointList[i].hasSlope) {
			// Find the last point in the slope ratio group.
			unsigned int j = i + 1;
			while (j < size - 1 && pointList[i].type == pointList[j].type && pointList[j].hasSlope) {
				++j;
			}
			if (pointList[i].type != pointList[j].type) {
				--j;
			}

			if (j - i < 2U) { // not enough points in the group
				for (unsigned int k = i; k <= j; ++k) { // add as normal points
					newTransition.pointOrSlopeList().push_back(makeNewPoint(pointList[k]));
				}
				i = j;
				continue;
			}

			// Create slope ratio group.
			auto newSlopeRatio = std::make_unique<VTMControlModel::Transition::SlopeRatio>();
			for (unsigned int k = i; k <= j; ++k) {
				newSlopeRatio->pointList.push_back(makeNewPoint(pointList[k]));
			}
			float slopeSum = 0.0;
			for (unsigned int k = i; k < j; ++k) {
				auto newSlope = std::make_unique<VTMControlModel::Transition::Slope>();
				newSlope->slope = pointList[k].slope;
				slopeSum += newSlope->slope;
				newSlopeRatio->slopeList.push_back(std::move(newSlope));
			}
			if (slopeSum == 0.0) {
				THROW_EXCEPTION(InvalidParameterException, "Slope ratio group with only zero slopes.");
			}
			newTransition.pointOrSlopeList().push_back(std::move(newSlopeRatio));

			i = j;
		} else { // normal points
			newTransition.pointOrSlopeList().push_back(makeNewPoint(pointList[i]));
		}
	}

	std::swap(transition, newTransition);
}

std::unique_ptr<VTMControlModel::Transition::Point>
TransitionPoint::makeNewPoint(const TransitionPoint& sourcePoint)
{
	auto newPoint = std::make_unique<VTMControlModel::Transition::Point>();
	newPoint->type = sourcePoint.type;
	newPoint->value = sourcePoint.value;

	const auto timeExpression = sourcePoint.timeExpression.lock();
	if (timeExpression) {
		newPoint->timeExpression = timeExpression;
	} else {
		newPoint->freeTime = sourcePoint.freeTime;
	}
	return newPoint;
}

} // namespace GS
