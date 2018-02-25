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
// 2014-09
// This file was copied from Gnuspeech and modified by Marcelo Y. Matuda.

#include "EventList.h"

#include <cassert>
#include <cstring>
#include <limits> /* std::numeric_limits<double>::infinity() */
#include <sstream>
#include <vector>

#include "Log.h"

#define DEFAULT_CONTROL_PERIOD_MS 4



namespace GS {
namespace VTMControlModel {

const double Event::EMPTY_PARAMETER = std::numeric_limits<double>::infinity();



EventList::EventList(const char* configDirPath, Model& model)
		: model_{model}
		, controlPeriod_{DEFAULT_CONTROL_PERIOD_MS}
		, macroIntonation_{}
		, microIntonation_{}
		, intonationDrift_{}
		, smoothIntonation_{true}
		, initialPitch_{}
		, meanPitch_{}
		, globalTempo_{1.0}
		, intonationFactor_{1.0}
		, intonationRhythm_{configDirPath}
{
	setUp();

	list_.reserve(128);
}

EventList::~EventList()
{
}

void
EventList::setUp()
{
	zeroRef_ = 0;
	zeroIndex_ = 0;
	duration_ = 0;

	postureData_.clear();
	postureData_.push_back(PostureData{});
	currentPosture_ = 0;

	feet_.clear();
	feet_.push_back(Foot{});
	currentFoot_ = 0;

	toneGroups_.clear();
	toneGroups_.push_back(ToneGroup{});
	currentToneGroup_ = 0;

	ruleData_.clear();
	ruleData_.push_back(RuleData{});
	currentRule_ = 0;

	intonationPoints_.clear();

	list_.clear();
}

void
EventList::setUpDriftGenerator(double deviation, double sampleRate, double lowpassCutoff)
{
	driftGenerator_.setUp(deviation, sampleRate, lowpassCutoff);
}

const Posture*
EventList::getPostureAtIndex(unsigned int index) const
{
	if (index > currentPosture_) {
		return nullptr;
	} else {
		return postureData_[index].posture;
	}
}

const PostureData*
EventList::getPostureDataAtIndex(unsigned int index) const
{
	if (index > currentPosture_) {
		return nullptr;
	} else {
		return &postureData_[index];
	}
}

const RuleData*
EventList::getRuleDataAtIndex(unsigned int index) const
{
	if (static_cast<int>(index) > currentRule_) {
		return nullptr;
	} else {
		return &ruleData_[index];
	}
}

void
EventList::setFixedIntonationParameters(
		float notionalPitch,
		float pretonicPitchRange,
		float pretonicPerturbationRange,
		float tonicPitchRange,
		float tonicPerturbationRange)
{
	intonationRhythm_.setFixedIntonationParameter(IntonationRhythm::INTON_PRM_NOTIONAL_PITCH             , notionalPitch);
	intonationRhythm_.setFixedIntonationParameter(IntonationRhythm::INTON_PRM_PRETONIC_PITCH_RANGE       , pretonicPitchRange);
	intonationRhythm_.setFixedIntonationParameter(IntonationRhythm::INTON_PRM_PRETONIC_PERTURBATION_RANGE, pretonicPerturbationRange);
	intonationRhythm_.setFixedIntonationParameter(IntonationRhythm::INTON_PRM_TONIC_PITCH_RANGE          , tonicPitchRange);
	intonationRhythm_.setFixedIntonationParameter(IntonationRhythm::INTON_PRM_TONIC_PERTURBATION_RANGE   , tonicPerturbationRange);
}

void
EventList::addPostureIntonationPoint(int postureIndex, double position, double semitone)
{
	int ruleIndex = -1;
	for (int i = 0; i < currentRule_; ++i) {
		if ((postureIndex >= ruleData_[i].firstPosture) && (postureIndex < ruleData_[i].lastPosture)) {
			ruleIndex = i;
			break;
		}
	}
	if (ruleIndex < 0) return;

	// This offset will be added to the beat time.
	double offsetTime = 0.0;

	const RuleData& ruleData = ruleData_[ruleIndex];
	const int numRulePostures = ruleData.lastPosture - ruleData.firstPosture + 1;
	const double start = ruleData.start;
	const int postureLocalIndex = postureIndex - ruleData.firstPosture;
	switch (postureLocalIndex) {
	case 0:
		if (numRulePostures > 2) {
			offsetTime = start + ruleData.mark1    * position - ruleData.beat;
		} else {
			offsetTime = start + ruleData.duration * position - ruleData.beat;
		}
		break;
	case 1:
		if (numRulePostures > 3) {
			offsetTime = start + ruleData.mark1 + (ruleData.mark2    - ruleData.mark1) * position - ruleData.beat;
		} else {
			offsetTime = start + ruleData.mark1 + (ruleData.duration - ruleData.mark1) * position - ruleData.beat;
		}
		break;
	case 2:
		offsetTime = start + ruleData.mark1 + ruleData.mark2 + (ruleData.duration - ruleData.mark2) * position - ruleData.beat;
		break;
	}

	addIntonationPoint(semitone, offsetTime, 0.0, ruleIndex);
}

double
EventList::getBeatAtIndex(int ruleIndex) const
{
	if (ruleIndex > currentRule_) {
		return 0.0;
	} else {
		return ruleData_[ruleIndex].beat;
	}
}

unsigned int
EventList::newPostureWithObject(const Posture& p, bool marked)
{
	if (postureData_[currentPosture_].posture) {
		postureData_.push_back(PostureData{});
		currentPosture_++;
	}
	postureData_[currentPosture_].tempo = 1.0;
	postureData_[currentPosture_].ruleTempo = 1.0;
	postureData_[currentPosture_].posture = &p;
	postureData_[currentPosture_].marked = marked;

	return currentPosture_;
}

void
EventList::replaceCurrentPostureWith(const Posture& p, bool marked)
{
	if (postureData_[currentPosture_].posture) {
		postureData_[currentPosture_].posture = &p;
		postureData_[currentPosture_].marked = marked;
	} else {
		postureData_[currentPosture_ - 1].posture = &p;
		postureData_[currentPosture_ - 1].marked = marked;
	}
}

void
EventList::setCurrentToneGroupType(IntonationRhythm::ToneGroup type)
{
	toneGroups_[currentToneGroup_].type = type;
}

void
EventList::newFoot()
{
	if (currentPosture_ == 0) {
		return;
	}

	feet_[currentFoot_++].end = currentPosture_;
	newPosture();

	feet_.push_back(Foot());
	feet_[currentFoot_].start = currentPosture_;
	feet_[currentFoot_].end = -1;
	feet_[currentFoot_].tempo = 1.0;
}

void
EventList::setCurrentFootMarked()
{
	feet_[currentFoot_].marked = 1;
}

void
EventList::setCurrentFootLast()
{
	feet_[currentFoot_].last = 1;
}

void
EventList::setCurrentFootTempo(double tempo)
{
	feet_[currentFoot_].tempo = tempo;
}

void
EventList::setCurrentPostureTempo(double tempo)
{
	postureData_[currentPosture_].tempo = tempo;
}

void
EventList::setCurrentPostureRuleTempo(double tempo)
{
	postureData_[currentPosture_].ruleTempo = tempo;
}

void
EventList::newToneGroup()
{
	if (currentFoot_ == 0) {
		return;
	}

	toneGroups_[currentToneGroup_++].endFoot = currentFoot_;
	newFoot();

	toneGroups_.push_back(ToneGroup());
	toneGroups_[currentToneGroup_].startFoot = currentFoot_;
	toneGroups_[currentToneGroup_].endFoot = -1;
}

void
EventList::newPosture()
{
	if (postureData_[currentPosture_].posture) {
		postureData_.push_back(PostureData{});
		currentPosture_++;
	}
	postureData_[currentPosture_].tempo = 1.0;
}

void
EventList::setCurrentPostureSyllable()
{
	postureData_[currentPosture_].syllable = 1;
}

Event*
EventList::insertEvent(double time, int parameter, double value, bool special)
{
	if (time < 0.0) {
		return nullptr;
	}
	if (time > static_cast<double>(duration_ + controlPeriod_)) {
		return nullptr;
	}
	const unsigned int numParam = model_.parameterList().size();

	int tempTime = zeroRef_ + static_cast<int>(time);
	if (controlPeriod_ != 1) {
		tempTime -= tempTime % controlPeriod_;
	}

	if (list_.empty()) {
		auto tempEvent = std::make_unique<Event>(numParam);
		tempEvent->time = tempTime;
		if (parameter >= 0) {
			tempEvent->setParameter(parameter, value, special);
		}
		list_.push_back(std::move(tempEvent));
		return list_.back().get();
	}

	int i;
	for (i = list_.size() - 1; i >= zeroIndex_; i--) {
		if (list_[i]->time == tempTime) {
			if (parameter >= 0) {
				list_[i]->setParameter(parameter, value, special);
			}
			return list_[i].get();
		}
		if (list_[i]->time < tempTime) {
			auto tempEvent = std::make_unique<Event>(numParam);
			tempEvent->time = tempTime;
			if (parameter >= 0) {
				tempEvent->setParameter(parameter, value, special);
			}
			list_.insert(list_.begin() + (i + 1), std::move(tempEvent));
			return list_[i + 1].get();
		}
	}

	auto tempEvent = std::make_unique<Event>(numParam);
	tempEvent->time = tempTime;
	if (parameter >= 0) {
		tempEvent->setParameter(parameter, value, special);
	}
	list_.insert(list_.begin() + (i + 1), std::move(tempEvent));
	return list_[i + 1].get();
}

void
EventList::setZeroRef(int newValue)
{
	zeroRef_ = newValue;
	zeroIndex_ = 0;

	if (list_.empty()) {
		return;
	}

	for (int i = list_.size() - 1; i >= 0; i--) {
		if (list_[i]->time < newValue) {
			zeroIndex_ = i;
			return;
		}
	}
}

double
EventList::createSlopeRatioEvents(const Transition::SlopeRatio& slopeRatio,
		double baseline, double parameterDelta, double min, double max, int parameter, double timeMultiplier, bool lastGroup)
{
	const unsigned int numPoints = slopeRatio.pointList.size();
	const unsigned int numSlopes = slopeRatio.slopeList.size();
	if (numPoints == 0 || numSlopes == 0) {
		THROW_EXCEPTION(MissingValueException, "Empty slope ratio.");
	}
	if (numPoints != numSlopes + 1) {
		THROW_EXCEPTION(InvalidValueException, "The number of slope points is not equal to the number of slopes plus one.");
	}

	double pointTime, pointValue;

	Transition::getPointData(*slopeRatio.pointList.front(), model_, pointTime, pointValue);
	const double startValue = pointValue;

	Transition::getPointData(*slopeRatio.pointList.back(), model_, pointTime, pointValue);
	const double valueDelta = pointValue - startValue;

	std::vector<double> tempPointValues(numSlopes - 1);
	double sum = 0.0;
	for (unsigned int i = 1; i < numPoints; ++i) {
		const double deltaTime = Transition::getPointTime(*slopeRatio.pointList[i], model_)
						- Transition::getPointTime(*slopeRatio.pointList[i - 1], model_);
		const double value = slopeRatio.slopeList[i - 1]->slope * deltaTime;
		sum += value;
		if (i < numSlopes) {
			tempPointValues[i - 1] = value; // will be multiplied by "factor"
		}
	}
	const double factor = valueDelta / sum;

	double baseValue = startValue;
	double value = 0.0;
	for (unsigned int i = 0; i < numPoints; ++i) {
		const Transition::Point& point = *slopeRatio.pointList[i];

		if (i >= 1 && i < numPoints - 1) {
			pointTime = Transition::getPointTime(point, model_);
			pointValue = baseValue + tempPointValues[i - 1] * factor;
			baseValue = pointValue;
		} else { // the first and the last points
			Transition::getPointData(point, model_, pointTime, pointValue);
		}

		value = baseline + ((pointValue / 100.0) * parameterDelta);
		if (value < min) {
			value = min;
		} else if (value > max) {
			value = max;
		}
		if (!(lastGroup && (i == numPoints - 1))) { // not a "phantom" point
			insertEvent(pointTime * timeMultiplier, parameter, value, false);
		}
	}

	return value;
}

void
EventList::applyRule(const Rule& rule, const std::vector<RuleExpressionData>& ruleExpressionData, unsigned int basePostureIndex,
			const std::vector<double>& minParam, const std::vector<double>& maxParam)
{
	const unsigned int numPostures = ruleExpressionData.size();
	assert(numPostures >= 2 && numPostures <= 4);
	const unsigned int numParam = model_.parameterList().size();

	double ruleSymbols[Rule::NUM_SYMBOLS];
	rule.evaluateExpressionSymbols(ruleExpressionData, model_, ruleSymbols);

	const double timeMultiplier = 1.0 / postureData_[basePostureIndex].ruleTempo;
	if (timeMultiplier != 1.0) {
		for (int i = 0; i < Rule::NUM_SYMBOLS; ++i) {
			ruleSymbols[i] *= timeMultiplier;
		}
	}

	const Rule::Type ruleType = rule.type();

	ruleData_[currentRule_].firstPosture = basePostureIndex;
	ruleData_[currentRule_].lastPosture  = basePostureIndex + (static_cast<int>(ruleType) - 1);
	ruleData_[currentRule_].start    = zeroRef_;
	ruleData_[currentRule_].mark1    = ruleSymbols[Rule::SYMB_MARK1];
	ruleData_[currentRule_].mark2    = ruleSymbols[Rule::SYMB_MARK2];
	ruleData_[currentRule_].duration = ruleSymbols[Rule::SYMB_DURATION];
	ruleData_[currentRule_].beat     = ruleSymbols[Rule::SYMB_BEAT] + zeroRef_;

	setDuration(static_cast<int>(ruleData_[currentRule_].duration));

	ruleData_.push_back(RuleData{});
	++currentRule_;

	switch (ruleType) {
	case Rule::Type::tetraphone:
		if (numPostures == 4) {
			postureData_[basePostureIndex + 3].onset = zeroRef_ + ruleSymbols[Rule::SYMB_BEAT];
			Event* tempEvent = insertEvent(ruleSymbols[Rule::SYMB_MARK2], -1, 0.0, false);
			if (tempEvent) tempEvent->flag = 1;
		}
		// Falls through.
	case Rule::Type::triphone:
		if (numPostures >= 3) {
			postureData_[basePostureIndex + 2].onset = zeroRef_ + ruleSymbols[Rule::SYMB_BEAT];
			Event* tempEvent = insertEvent(ruleSymbols[Rule::SYMB_MARK1], -1, 0.0, false);
			if (tempEvent) tempEvent->flag = 1;
		}
		// Falls through.
	case Rule::Type::diphone:
		{
			postureData_[basePostureIndex + 1].onset = zeroRef_ + ruleSymbols[Rule::SYMB_BEAT];
			Event* tempEvent = insertEvent(0.0, -1, 0.0, false);
			if (tempEvent) tempEvent->flag = 1;
		}
		break;
	case Rule::Type::invalid:
		// Unreachable.
		break;
	}

	// Check time intervals.
	const int minTimeInterval = controlPeriod_ + 1;
	switch (ruleType) {
	case Rule::Type::tetraphone:
		if (static_cast<int>(ruleSymbols[Rule::SYMB_DURATION] - ruleSymbols[Rule::SYMB_MARK2]) < minTimeInterval ||
				static_cast<int>(ruleSymbols[Rule::SYMB_MARK2] - ruleSymbols[Rule::SYMB_MARK1]) < minTimeInterval ||
				static_cast<int>(ruleSymbols[Rule::SYMB_MARK1]) < minTimeInterval) {
			THROW_EXCEPTION(InvalidValueException, "The time interval between two postures is too small. Try reducing the tempo.");
		}
		break;
	case Rule::Type::triphone:
		if (static_cast<int>(ruleSymbols[Rule::SYMB_DURATION] - ruleSymbols[Rule::SYMB_MARK1]) < minTimeInterval ||
				static_cast<int>(ruleSymbols[Rule::SYMB_MARK1]) < minTimeInterval) {
			THROW_EXCEPTION(InvalidValueException, "The time interval between two postures is too small. Try reducing the tempo.");
		}
		break;
	case Rule::Type::diphone:
		if (static_cast<int>(ruleSymbols[Rule::SYMB_DURATION]) < minTimeInterval) {
			THROW_EXCEPTION(InvalidValueException, "The time interval between two postures is too small. Try reducing the tempo.");
		}
		break;
	case Rule::Type::invalid:
		// Unreachable.
		break;
	}

	/* Loop through the parameters */
	for (unsigned int i = 0; i < numParam; ++i) {
		double targets[4];

		/* Get actual parameter target values */
		targets[0] = postureData_[basePostureIndex].posture->getParameterTarget(i);
		targets[1] = postureData_[basePostureIndex + 1].posture->getParameterTarget(i);
		targets[2] = (numPostures >= 3) ? postureData_[basePostureIndex + 2].posture->getParameterTarget(i) : 0.0;
		targets[3] = (numPostures == 4) ? postureData_[basePostureIndex + 3].posture->getParameterTarget(i) : 0.0;

		/* Optimization, Don't calculate if no changes occur */
		bool equalTargets = false;
		switch (ruleType) {
		case Rule::Type::diphone:
			if (targets[0] == targets[1]) {
				equalTargets = true;
			}
			break;
		case Rule::Type::triphone:
			if ((targets[0] == targets[1]) && (targets[0] == targets[2])) {
				equalTargets = true;
			}
			break;
		case Rule::Type::tetraphone:
			if ((targets[0] == targets[1]) && (targets[0] == targets[2]) && (targets[0] == targets[3])) {
				equalTargets = true;
			}
			break;
		default:
			// Unreachable.
			break;
		}

		insertEvent(0.0, i, targets[0], false);

		if (!equalTargets) {
			auto currentType = Transition::Point::Type::diphone;
			double currentValueDelta = targets[1] - targets[0];
			double lastValue = targets[0];
			double value{};

			const std::shared_ptr<Transition> transition = rule.getParamProfileTransition(i);
			if (!transition) {
				THROW_EXCEPTION(UnavailableResourceException, "Rule transition not found (rule: "
						<< ruleData_[currentRule_ - 1].number << " parameter: " << i << ").");
			}

			// Generate events from the transition points.
			// The points must be sorted by type.
			const unsigned int numPointsOrSlopes = transition->pointOrSlopeList().size();
			for (unsigned int j = 0; j < numPointsOrSlopes; ++j) {
				const Transition::PointOrSlope& pointOrSlope = *transition->pointOrSlopeList()[j];
				const bool last = (j == numPointsOrSlopes - 1);
				if (pointOrSlope.isSlopeRatio()) {
					const auto& slopeRatio = dynamic_cast<const Transition::SlopeRatio&>(pointOrSlope);
					if (slopeRatio.pointList.empty()) {
						THROW_EXCEPTION(UnavailableResourceException, "Empty slope ratio in transition "
									<< transition->name() << '.');
					}
					if (slopeRatio.pointList[0]->type != currentType) {
						currentType = slopeRatio.pointList[0]->type;
						targets[static_cast<int>(currentType) - 2] = lastValue;
						currentValueDelta = targets[static_cast<int>(currentType) - 1] - lastValue;
					}
					value = createSlopeRatioEvents(
							slopeRatio, targets[static_cast<int>(currentType) - 2], currentValueDelta,
							minParam[i], maxParam[i], i, timeMultiplier, last);
				} else {
					const auto& point = dynamic_cast<const Transition::Point&>(pointOrSlope);
					if (point.type != currentType) {
						currentType = point.type;
						targets[static_cast<int>(currentType) - 2] = lastValue;
						currentValueDelta = targets[static_cast<int>(currentType) - 1] - lastValue;
					}
					double pointTime;
					Transition::getPointData(point, model_,
									targets[static_cast<int>(currentType) - 2], currentValueDelta, minParam[i], maxParam[i],
									pointTime, value);
					if (!last) { // not a "phantom" point
						insertEvent(pointTime * timeMultiplier, i, value, false);
					}
				}
				lastValue = value;
			}
		}
	}

	/* Special Event Profiles */
	for (unsigned int i = 0; i < numParam; ++i) {
		const std::shared_ptr<Transition> specialTransition = rule.getSpecialProfileTransition(i);
		if (specialTransition) {
			for (unsigned int j = 0, size = specialTransition->pointOrSlopeList().size(); j < size; ++j) {
				const Transition::PointOrSlope& pointOrSlope = *specialTransition->pointOrSlopeList()[j];
				const auto& point = dynamic_cast<const Transition::Point&>(pointOrSlope);

				/* calculate time of event */
				const double time = Transition::getPointTime(point, model_);

				/* Calculate value of event */
				const double value = ((point.value / 100.0) * (maxParam[i] - minParam[i]));

				/* insert event into event list */
				insertEvent(time * timeMultiplier, i, value, true);
			}
		}
	}

	setZeroRef(static_cast<int>(ruleSymbols[Rule::SYMB_DURATION]) + zeroRef_);
	Event* tempEvent = insertEvent(0.0, -1, 0.0, false); // insert at rule duration
	if (tempEvent) tempEvent->flag = 1;
}

void
EventList::generateEventList()
{
	const unsigned int numParam = model_.parameterList().size();
	std::vector<double> minParam(numParam);
	std::vector<double> maxParam(numParam);
	for (unsigned int i = 0; i < numParam; ++i) {
		const Parameter& param = model_.getParameter(i);
		minParam[i] = static_cast<double>(param.minimum());
		maxParam[i] = static_cast<double>(param.maximum());
	}

	applyRhythm();

	unsigned int basePostureIndex = 0;
	std::vector<RuleExpressionData> ruleExpressionData;
	while (basePostureIndex < currentPosture_) {
		ruleExpressionData.clear();
		for (unsigned int i = 0; i < 4; i++) {
			unsigned int postureIndex = basePostureIndex + i;
			if (postureIndex <= currentPosture_ && postureData_[postureIndex].posture) {
				ruleExpressionData.push_back(RuleExpressionData{
								postureData_[postureIndex].posture,
								postureData_[postureIndex].tempo,
								postureData_[postureIndex].marked});
			} else {
				break;
			}
		}
		if (ruleExpressionData.size() < 2) {
			break;
		}
		unsigned int ruleIndex = 0;
		const Rule* rule = model_.findFirstMatchingRule(ruleExpressionData, ruleIndex);
		if (!rule) {
			THROW_EXCEPTION(UnavailableResourceException, "Could not find a matching rule.");
		}

		ruleData_[currentRule_].number = ruleIndex + 1U;

		applyRule(*rule, ruleExpressionData, basePostureIndex, minParam, maxParam);

		basePostureIndex += rule->numberOfExpressions() - 1;
	}
}

void
EventList::setFullTimeScale()
{
	zeroRef_ = 0;
	zeroIndex_ = 0;
	duration_ = list_.back()->time + 100; // hardcoded
}

void
EventList::applyIntonation()
{
	if (list_.empty()) return;

	zeroRef_ = 0;
	zeroIndex_ = 0;
	duration_ = list_.back()->time + 100; // hardcoded

	std::shared_ptr<const Category> vocoidCategory = model_.findCategory("vocoid"); // hardcoded
	if (!vocoidCategory) {
		THROW_EXCEPTION(UnavailableResourceException, "Could not find the category \"vocoid\".");
	}

	const float* intonParms{};
	double offsetTime = 0.0;
	int ruleIndex = 0;
	for (int i = 0; i < currentToneGroup_; i++) {
		const int firstFoot = toneGroups_[i].startFoot;
		const int endFoot   = toneGroups_[i].endFoot;

		const double startTime = postureData_[feet_[firstFoot].start].onset;
		const double endTime = postureData_[feet_[endFoot].end].onset;

		intonParms = intonationRhythm_.intonationParameters(toneGroups_[i].type);

		const double pretonicDelta = intonParms[IntonationRhythm::INTON_PRM_NOTIONAL_PITCH] / (endTime - startTime);

		/* Set up intonation boundary variables */
		for (int j = firstFoot; j <= endFoot; j++) {
			int postureIndex = feet_[j].start;
			while (!postureData_[postureIndex].posture->isMemberOfCategory(*vocoidCategory)) {
				postureIndex++;
				if (postureIndex > feet_[j].end) {
					postureIndex = feet_[j].start;
					break;
				}
			}

			for (int k = 0; k < currentRule_; k++) {
				if ((postureIndex >= ruleData_[k].firstPosture) && (postureIndex <= ruleData_[k].lastPosture)) {
					ruleIndex = k;
					break;
				}
			}

			if (!feet_[j].marked) { // pretonic
				double semitone, slope;
				if (intonationRhythm_.randomIntonation()) {
					semitone = (intonationRhythm_.randomReal() - 0.5) * intonParms[IntonationRhythm::INTON_PRM_PRETONIC_PERTURBATION_RANGE];
					slope = intonationRhythm_.randomReal() * intonationRhythm_.pretonicSlopeRandomFactor()
							+ intonationRhythm_.pretonicBaseSlopeRandom();
				} else {
					semitone = 0.0;
					slope = intonationRhythm_.pretonicBaseSlope();
				}

				addIntonationPoint((postureData_[postureIndex].onset - startTime) * pretonicDelta
								+ intonParms[IntonationRhythm::INTON_PRM_NOTIONAL_PITCH]
								+ semitone,
							offsetTime, slope, ruleIndex);
			} else { // tonic
				double semitone, slope;
				if (toneGroups_[i].type == IntonationRhythm::ToneGroup::continuation) {
					slope = intonationRhythm_.tonicContinuationBaseSlope();
				} else {
					slope = intonationRhythm_.tonicBaseSlope();
				}
				if (intonationRhythm_.randomIntonation()) {
					semitone = (intonationRhythm_.randomReal() - 0.5) * intonParms[IntonationRhythm::INTON_PRM_TONIC_PERTURBATION_RANGE];
					slope += intonationRhythm_.randomReal() * intonationRhythm_.tonicSlopeRandomFactor();
				} else {
					semitone = 0.0;
					slope += intonationRhythm_.tonicSlopeOffset();
				}
				addIntonationPoint(intonParms[IntonationRhythm::INTON_PRM_PRETONIC_PITCH_RANGE]
								+ intonParms[IntonationRhythm::INTON_PRM_NOTIONAL_PITCH]
								+ semitone,
							offsetTime, slope, ruleIndex);

				postureIndex = feet_[j].end;
				for (int k = ruleIndex; k < currentRule_; k++) {
					if ((postureIndex >= ruleData_[k].firstPosture) && (postureIndex <= ruleData_[k].lastPosture)) {
						ruleIndex = k;
						break;
					}
				}

				addIntonationPoint(intonParms[IntonationRhythm::INTON_PRM_PRETONIC_PITCH_RANGE]
								+ intonParms[IntonationRhythm::INTON_PRM_NOTIONAL_PITCH]
								+ intonParms[IntonationRhythm::INTON_PRM_TONIC_PITCH_RANGE],
							0.0, 0.0, ruleIndex);
			}
			offsetTime = intonationRhythm_.intonationTimeOffset();
		}
	}
	if (intonParms) {
		addIntonationPoint(intonParms[IntonationRhythm::INTON_PRM_PRETONIC_PITCH_RANGE]
						+ intonParms[IntonationRhythm::INTON_PRM_NOTIONAL_PITCH]
						+ intonParms[IntonationRhythm::INTON_PRM_TONIC_PITCH_RANGE],
					0.0, 0.0, currentRule_ - 1);
	}

	if (macroIntonation_) prepareMacroIntonationInterpolation();
}

void
EventList::applyRhythm()
{
	/* Calculate Rhythm including regression */
	for (int i = 0; i < currentFoot_; i++) {
		const int numFootPostures = feet_[i].end - feet_[i].start + 1;
		/* Apply rhythm model */
		if (feet_[i].marked) {
			const double temp = intonationRhythm_.rhythmMarkedA() * numFootPostures + intonationRhythm_.rhythmMarkedB();
			feet_[i].tempo += temp / intonationRhythm_.rhythmMarkedDiv();
		} else {
			const double temp = intonationRhythm_.rhythmUnmarkedA() * numFootPostures + intonationRhythm_.rhythmUnmarkedB();
			feet_[i].tempo += temp / intonationRhythm_.rhythmUnmarkedDiv();
		}
		if (feet_[i].tempo < intonationRhythm_.rhythmMinTempo()) {
			feet_[i].tempo = intonationRhythm_.rhythmMinTempo();
		} else if (feet_[i].tempo > intonationRhythm_.rhythmMaxTempo()) {
			feet_[i].tempo = intonationRhythm_.rhythmMaxTempo();
		}
		feet_[i].tempo *= globalTempo_;
		for (int j = feet_[i].start; j < feet_[i].end + 1; j++) {
			postureData_[j].tempo *= feet_[i].tempo;
		}
	}
}

void
EventList::prepareMacroIntonationInterpolation()
{
	setFullTimeScale();

	if (intonationPoints_.empty()) return;

	Event* event1 = insertEvent(intonationPoints_[0].absoluteTime(), -1, 0.0, false);
	if (!event1) {
		THROW_EXCEPTION(MissingValueException, "Could not create event 1 for intonation.");
	}
	Event* event2{};

	const unsigned int numPoints = intonationPoints_.size();
	for (unsigned int j = 0; j < numPoints - 1; ++j) {
		const IntonationPoint& point1 = intonationPoints_[j];
		const IntonationPoint& point2 = intonationPoints_[j + 1];

		event2 = insertEvent(point2.absoluteTime(), -1, 0.0, false);
		if (!event2) {
			THROW_EXCEPTION(MissingValueException, "Could not create event 2 for intonation.");
		}

		const double x1 = event1->time;
		const double y1 = point1.semitone();
		const double x2 = event2->time;
		const double y2 = point2.semitone();
		const double dx = x2 - x1;
		auto interpData = std::make_unique<InterpolationData>();

		if (smoothIntonation_) { // cubic interpolation
			const double m1 = point1.slope();
			const double m2 = point2.slope();

			const double x12 = x1 * x1;
			const double x13 = x12 * x1;

			const double x22 = x2 * x2;
			const double x23 = x22 * x2;

			const double coef = 1.0 / (dx * dx * dx);

			const double d = ( -(y2 * x13) + 3.0 * y2 * x12 * x2 + m2 * x13 * x2 + m1 * x12 * x22 - m2 * x12 * x22 - 3.0 * x1 * y1 * x22 - m1 * x1 * x23 + y1 * x23 )
						* coef;
			const double c = ( -(m2 * x13) - 6.0 * y2 * x1 * x2 - 2.0 * m1 * x12 * x2 - m2 * x12 * x2 + 6.0 * x1 * y1 * x2 + m1 * x1 * x22 + 2.0 * m2 * x1 * x22 + m1 * x23 )
						* coef;
			const double b = ( 3.0 * y2 * x1 + m1 * x12 + 2.0 * m2 * x12 - 3.0 * x1 * y1 + 3.0 * x2 * y2 + m1 * x1 * x2 - m2 * x1 * x2 - 3.0 * y1 * x2 - 2.0 * m1 * x22 - m2 * x22 )
						* coef;
			const double a = ( -2.0 * y2 - m1 * x1 - m2 * x1 + 2.0 * y1 + m1 * x2 + m2 * x2)
						* coef;
			interpData->a = a;
			interpData->b = b;
			interpData->c = c;
			interpData->d = d;
		} else { // linear interpolation
			const double dy = y2 - y1;
			const double coef = dy / dx;

			const double b = y1 - x1 * coef;
			const double a = coef;
			interpData->a = a;
			interpData->b = b;
		}

		event1->interpData = std::move(interpData);

		event1 = event2;

		if (j == numPoints - 2) { // last iteration
			// After the last point: constant value.
			auto lastInterpData = std::make_unique<InterpolationData>();
			if (smoothIntonation_) {
				lastInterpData->d = point2.semitone();
			} else {
				lastInterpData->b = point2.semitone();
			}
			event2->interpData = std::move(lastInterpData);
		}
	}
}

void
EventList::addIntonationPoint(double semitone, double offsetTime, double slope, int ruleIndex)
{
	if (ruleIndex > currentRule_) {
		return;
	}

	IntonationPoint iPoint(this);
	iPoint.setRuleIndex(ruleIndex);
	iPoint.setOffsetTime(offsetTime);
	iPoint.setSemitone(semitone * intonationFactor_);
	iPoint.setSlope(slope * intonationFactor_);

	double time = iPoint.absoluteTime();
	double minTime;
	if (intonationPoints_.empty()) {
		minTime = 2.0 * controlPeriod_;
	} else {
		minTime = intonationPoints_.back().absoluteTime() + 2.0 * controlPeriod_;
	}
	if (time < minTime) {
		iPoint.setOffsetTime(offsetTime + (minTime - time));
	}

	intonationPoints_.push_back(iPoint);
}

void
EventList::generateOutput(std::vector<std::vector<float>>& vtmParamList)
{
	if (list_.size() < 2) {
		return;
	}

	const unsigned int numParam = model_.parameterList().size();
	const unsigned int numEvents = list_.size();
	std::vector<double> currentValues(numParam, 0.0);
	std::vector<double> currentDeltas(numParam, 0.0);
	std::vector<double> currentSpecialValues(numParam, 0.0);
	std::vector<double> currentSpecialDeltas(numParam, 0.0);

	// Set current values and deltas for normal parameters.
	for (unsigned int i = 0; i < numParam; ++i) {
		currentValues[i] = list_[0]->getParameter(i, false); // the first event contains the parameters of the first posture
		unsigned int j = 1;
		double value;
		while ((value = list_[j]->getParameter(i, false)) == Event::EMPTY_PARAMETER) {
			if (++j >= numEvents) break;
		}
		if (j < numEvents) {
			currentDeltas[i] = ((value - currentValues[i]) / list_[j]->time) * controlPeriod_;
		}
	}

	double pa{}, pb{}, pc{}, pd{}; // coefficients for polynomial interpolation (macro intonation)

	// Configure first segment of the macro intonation curve.
	if (macroIntonation_) {
		unsigned int j = 0;
		for ( ; j < numEvents; ++j) {
			if (list_[j]->interpData) break;
		}
		if (j < numEvents) {
			const double y1 = initialPitch_;
			const double x2 = list_[j]->time;
			const InterpolationData& data = *list_[j]->interpData;
			// Straight line.
			if (smoothIntonation_) {
				const double y2 = x2 * (x2 * (x2 * data.a + data.b) + data.c) + data.d;
				pc = (y2 - y1) / x2;
				pd = y1;
			} else {
				const double y2 = x2 * data.a + data.b;
				pa = (y2 - y1) / x2;
				pb = y1;
			}
		}
	}

	unsigned int targetIndex = 1; // stores the index of the next target event
	int targetTime = list_[targetIndex]->time;
	int currentTime = 0; // absolute time in ms
	std::vector<float> param(numParam, 0.0);
	// Loop for each control period.
	while (targetIndex < numEvents) {
		// Add normal parameters and special parameters.
		for (unsigned int j = 0; j < numParam; ++j) {
			param[j] = static_cast<float>(currentValues[j] + currentSpecialValues[j]);
		}

		// Intonation.
		// Hardcoded: parameter 0 is always the pitch.
		if (!microIntonation_) param[0] = 0.0;
		if (intonationDrift_)  param[0] += static_cast<float>(driftGenerator_.drift());
		if (macroIntonation_) {
			const double x = currentTime;
			double intonation;
			if (smoothIntonation_) {
				intonation = x * (x * (x * pa + pb) + pc) + pd;
			} else {
				intonation = x * pa + pb;
			}
			param[0] += static_cast<float>(intonation);
		}
		param[0] += static_cast<float>(meanPitch_);

		// Store the current parameter values.
		vtmParamList.push_back(param);

		//--------------------------------------------------------------
		// Prepare for the next iteration.

		// Linear interpolation of the parameters.
		for (unsigned int j = 0; j < numParam; ++j) {
			if (currentDeltas[j]) {
				currentValues[j] += currentDeltas[j];
			}
		}
		for (unsigned int j = 0; j < numParam; ++j) {
			if (currentSpecialDeltas[j]) {
				currentSpecialValues[j] += currentSpecialDeltas[j];
			}
		}

		currentTime += controlPeriod_;

		if (currentTime >= targetTime) {
			// Next interval.
			if (++targetIndex == numEvents) {
				break; // all events processed
			}
			targetTime = list_[targetIndex]->time;

			// Set current values and deltas for normal parameters.
			for (unsigned int j = 0; j < numParam; ++j) {
				// If the current event contains a value for the parameter:
				if (list_[targetIndex - 1]->getParameter(j, false) != Event::EMPTY_PARAMETER) {
					unsigned int k = targetIndex;
					double value;
					// Search for an event with value for this parameter.
					while ((value = list_[k]->getParameter(j, false)) == Event::EMPTY_PARAMETER) {
						if (++k >= numEvents) break;
					}
					if (value != Event::EMPTY_PARAMETER) { // found
						// Prepare linear interpolation.
						currentDeltas[j] = ((value - currentValues[j]) / (list_[k]->time - currentTime)) * controlPeriod_;
					} else { // there are no more events with value for this parameter
						currentDeltas[j] = 0.0;
					}
				}
			}
			// Set current values and deltas for special parameters.
			for (unsigned int j = 0; j < numParam; ++j) {
				// If the current event contains a value for the special parameter:
				if (list_[targetIndex - 1]->getParameter(j, true) != Event::EMPTY_PARAMETER) {
					unsigned int k = targetIndex;
					double value;
					// Search for an event with value for this special parameter.
					while ((value = list_[k]->getParameter(j, true)) == Event::EMPTY_PARAMETER) {
						if (++k >= numEvents) break;
					}
					if (value != Event::EMPTY_PARAMETER) { // found
						// Prepare linear interpolation.
						currentSpecialDeltas[j] = ((value - currentSpecialValues[j]) / (list_[k]->time - currentTime)) * controlPeriod_;
					} else { // there are no more events with value for this special parameter
						currentSpecialDeltas[j] = 0.0;
					}
				}
			}

			if (macroIntonation_) {
				// If the current event contains data for intonation interpolation,
				// setup the interpolation.
				if (list_[targetIndex - 1]->interpData) {
					const InterpolationData& data = *list_[targetIndex - 1]->interpData;
					pa = data.a;
					pb = data.b;
					if (smoothIntonation_) {
						pc = data.c;
						pd = data.d;
					}
				}
			}
		}
	}

	if (Log::debugEnabled) {
		printDataStructures();
	}
}

void
EventList::clearMacroIntonation()
{
	for (auto& event : list_) {
		event->interpData.reset();
	}
}

void
EventList::setControlPeriod(int value)
{
	if (value < 1 || value > 4) {
		THROW_EXCEPTION(InvalidValueException, "Invalid control period: " << value << '.');
	}
	controlPeriod_ = value;
}

void
EventList::printDataStructures()
{
	printf("Tone Groups %d\n", currentToneGroup_);
	for (int i = 0; i < currentToneGroup_; i++) {
		printf("%d  start: %d  end: %d  type: %d\n", i, toneGroups_[i].startFoot, toneGroups_[i].endFoot,
			static_cast<int>(toneGroups_[i].type));
	}

	printf("\nFeet %d\n", currentFoot_);
	for (int i = 0; i < currentFoot_; i++) {
		printf("%d  tempo: %f start: %d  end: %d  marked: %d last: %d onset1: %f onset2: %f\n", i, feet_[i].tempo,
			feet_[i].start, feet_[i].end, feet_[i].marked, feet_[i].last, feet_[i].onset1, feet_[i].onset2);
	}

	printf("\nPostures %d\n", currentPosture_);
	for (unsigned int i = 0; i < currentPosture_; i++) {
		printf("DEBUG_POSTURE %u  \"%s\" tempo: %f syllable: %d onset: %f ruleTempo: %f\n",
			i, postureData_[i].posture->name().c_str(), postureData_[i].tempo, postureData_[i].syllable,
			postureData_[i].onset, postureData_[i].ruleTempo);
	}

	printf("\nRules %d\n", currentRule_);
	for (int i = 0; i < currentRule_; i++) {
		printf("Number: %d  start: %d  end: %d  duration %f\n", ruleData_[i].number, ruleData_[i].firstPosture,
			ruleData_[i].lastPosture, ruleData_[i].duration);
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */
