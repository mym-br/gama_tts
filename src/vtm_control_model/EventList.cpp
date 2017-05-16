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

#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>

#include "Log.h"

#define DIPHONE 2
#define TRIPHONE 3
#define TETRAPHONE 4

#define INTONATION_CONFIG_FILE_NAME "/intonation.txt"



namespace GS {
namespace VTMControlModel {

EventList::EventList(const char* configDirPath, Model& model)
		: model_ {model}
		, macroIntonation_ {}
		, microIntonation_ {}
		, intonationDrift_ {}
		, smoothIntonation_ {true}
		, globalTempo_ {1.0}
		, tgParameters_(5)
		, useFixedIntonationParameters_ {false}
		, randSrc_ {randDev_()}
{
	setUp();

	list_.reserve(128);

	initToneGroups(configDirPath);

	for (int i = 0; i < 10; ++i) {
		fixedIntonationParameters_[i] = 0.0;
	}
}

EventList::~EventList()
{
}

void
EventList::setUp()
{
	list_.clear();

	zeroRef_ = 0;
	zeroIndex_ = 0;
	duration_ = 0;
	timeQuantization_ = 4;

	intonParms_ = nullptr;

	postureData_.clear();
	postureData_.push_back(PostureData());
	postureTempo_.clear();
	postureTempo_.push_back(1.0);
	currentPosture_ = 0;

	feet_.clear();
	feet_.push_back(Foot());
	currentFoot_ = 0;

	toneGroups_.clear();
	toneGroups_.push_back(ToneGroup());
	currentToneGroup_ = 0;

	ruleData_.clear();
	ruleData_.push_back(RuleData());
	currentRule_ = 0;
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
EventList::getRuleAtIndex(unsigned int index) const
{
	if (static_cast<int>(index) > currentRule_) {
		return nullptr;
	} else {
		return &ruleData_[index];
	}
}

void
EventList::setFixedIntonationParameters(float notionalPitch, float pretonicRange, float pretonicLift, float tonicRange, float tonicMovement)
{
	fixedIntonationParameters_[1] = notionalPitch;
	fixedIntonationParameters_[2] = pretonicRange;
	fixedIntonationParameters_[3] = pretonicLift;
	fixedIntonationParameters_[5] = tonicRange;
	fixedIntonationParameters_[6] = tonicMovement;
}

void
EventList::parseGroups(int index, int number, FILE* fp)
{
	char line[256];
	tgParameters_[index].resize(10 * number);
	for (int i = 0; i < number; ++i) {
		fgets(line, 256, fp);
		float* temp = &tgParameters_[index][i * 10];
		sscanf(line, " %f %f %f %f %f %f %f %f %f %f",
			&temp[0], &temp[1], &temp[2], &temp[3], &temp[4],
			&temp[5], &temp[6], &temp[7], &temp[8], &temp[9]);
	}
}

void
EventList::initToneGroups(const char* configDirPath)
{
	FILE* fp;
	char line[256];
	int count = 0;

	std::ostringstream path;
	path << configDirPath << INTONATION_CONFIG_FILE_NAME;
	fp = fopen(path.str().c_str(), "rb");
	if (fp == NULL) {
		THROW_EXCEPTION(IOException, "Could not open the file " << path.str().c_str() << '.');
	}
	while (fgets(line, 256, fp) != NULL) {
		if ((line[0] == '#') || (line[0] == ' ')) {
			// Skip.
		} else if (strncmp(line, "TG", 2) == 0) {
			sscanf(&line[2], " %d", &tgCount_[count]);
			parseGroups(count, tgCount_[count], fp);
			count++;
		} else if (strncmp(line, "RANDOM", 6) == 0) {
			sscanf(&line[6], " %f", &intonationRandom_);
		}
	}
	fclose(fp);

	if (Log::debugEnabled) {
		printToneGroups();
	}
}

void
EventList::printToneGroups()
{
	printf("===== Intonation configuration:\n");
	printf("Intonation random = %f\n", intonationRandom_);
	printf("Tone groups: %d %d %d %d %d\n", tgCount_[0], tgCount_[1], tgCount_[2], tgCount_[3], tgCount_[4]);

	for (int i = 0; i < 5; i++) {
		float* temp = &tgParameters_[i][0];
		printf("Temp [%d] = %p\n", i, temp);
		int j = 0;
		for (int k = 0; k < tgCount_[i]; k++) {
			printf("%f %f %f %f %f %f %f %f %f %f\n",
				temp[j]  , temp[j+1], temp[j+2], temp[j+3], temp[j+4],
				temp[j+5], temp[j+6], temp[j+7], temp[j+8], temp[j+9]);
			j += 10;
		}
	}
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

void
EventList::newPostureWithObject(const Posture& p)
{
	if (postureData_[currentPosture_].posture) {
		postureData_.push_back(PostureData());
		postureTempo_.push_back(1.0);
		currentPosture_++;
	}
	postureTempo_[currentPosture_] = 1.0;
	postureData_[currentPosture_].ruleTempo = 1.0;
	postureData_[currentPosture_].posture = &p;
}

void
EventList::replaceCurrentPostureWith(const Posture& p)
{
	if (postureData_[currentPosture_].posture) {
		postureData_[currentPosture_].posture = &p;
	} else {
		postureData_[currentPosture_ - 1].posture = &p;
	}
}

void
EventList::setCurrentToneGroupType(int type)
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
	postureTempo_[currentPosture_] = tempo;
}

void
EventList::setCurrentPostureRuleTempo(float tempo)
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
		postureData_.push_back(PostureData());
		postureTempo_.push_back(1.0);
		currentPosture_++;
	}
	postureTempo_[currentPosture_] = 1.0;
}

void
EventList::setCurrentPostureSyllable()
{
	postureData_[currentPosture_].syllable = 1;
}

Event*
EventList::insertEvent(double time, int parameter, double value)
{
	if (time < 0.0) {
		return nullptr;
	}
	if (time > (double) (duration_ + timeQuantization_)) {
		return nullptr;
	}

	int tempTime = zeroRef_ + (int) time;
	tempTime = (tempTime >> 2) << 2;
	//if ((tempTime % timeQuantization) != 0) {
	//	tempTime++;
	//}

	if (list_.empty()) {
		auto tempEvent = std::make_unique<Event>();
		tempEvent->time = tempTime;
		if (parameter >= 0) {
			tempEvent->setParameter(parameter, value);
		}
		list_.push_back(std::move(tempEvent));
		return list_.back().get();
	}

	int i;
	for (i = list_.size() - 1; i >= zeroIndex_; i--) {
		if (list_[i]->time == tempTime) {
			if (parameter >= 0) {
				list_[i]->setParameter(parameter, value);
			}
			return list_[i].get();
		}
		if (list_[i]->time < tempTime) {
			auto tempEvent = std::make_unique<Event>();
			tempEvent->time = tempTime;
			if (parameter >= 0) {
				tempEvent->setParameter(parameter, value);
			}
			list_.insert(list_.begin() + (i + 1), std::move(tempEvent));
			return list_[i + 1].get();
		}
	}

	auto tempEvent = std::make_unique<Event>();
	tempEvent->time = tempTime;
	if (parameter >= 0) {
		tempEvent->setParameter(parameter, value);
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
EventList::createSlopeRatioEvents(
		const Transition::SlopeRatio& slopeRatio,
		double baseline, double parameterDelta, double min, double max, int eventIndex, double timeMultiplier)
{
	double temp = 0.0, temp1 = 0.0, intervalTime = 0.0, sum = 0.0, factor = 0.0;
	double baseTime = 0.0, endTime = 0.0, totalTime = 0.0, delta = 0.0;
	double startValue;
	double pointTime, pointValue;

	Transition::getPointData(*slopeRatio.pointList.front(), model_, pointTime, pointValue);
	baseTime = pointTime;
	startValue = pointValue;

	Transition::getPointData(*slopeRatio.pointList.back(), model_, pointTime, pointValue);
	endTime = pointTime;
	delta = pointValue - startValue;

	temp = slopeRatio.totalSlopeUnits();
	totalTime = endTime - baseTime;

	int numSlopes = slopeRatio.slopeList.size();
	std::vector<double> newPointValues(numSlopes - 1);
	for (int i = 1; i < numSlopes + 1; i++) {
		temp1 = slopeRatio.slopeList[i - 1]->slope / temp; /* Calculate normal slope */

		/* Calculate time interval */
		intervalTime = Transition::getPointTime(*slopeRatio.pointList[i], model_)
				- Transition::getPointTime(*slopeRatio.pointList[i - 1], model_);

		/* Apply interval percentage to slope */
		temp1 = temp1 * (intervalTime / totalTime);

		/* Multiply by delta and add to last point */
		temp1 = temp1 * delta;
		sum += temp1;

		if (i < numSlopes) {
			newPointValues[i - 1] = temp1;
		}
	}
	factor = delta / sum;
	temp = startValue;

	double value = 0.0;
	for (unsigned int i = 0, size = slopeRatio.pointList.size(); i < size; i++) {
		const Transition::Point& point = *slopeRatio.pointList[i];

		if (i >= 1 && i < slopeRatio.pointList.size() - 1) {
			pointTime = Transition::getPointTime(point, model_);

			pointValue = newPointValues[i - 1];
			pointValue *= factor;
			pointValue += temp;
			temp = pointValue;
		} else {
			Transition::getPointData(point, model_, pointTime, pointValue);
		}

		value = baseline + ((pointValue / 100.0) * parameterDelta);
		if (value < min) {
			value = min;
		} else if (value > max) {
			value = max;
		}
		if (!point.isPhantom) {
			insertEvent(pointTime * timeMultiplier, eventIndex, value);
		}
	}

	return value;
}

// It is assumed that postureList.size() >= 2.
void
EventList::applyRule(const Rule& rule, const std::vector<const Posture*>& postureList, const double* tempos, int postureIndex)
{
	int cont;
	int currentType;
	double currentValueDelta, value, lastValue;
	double ruleSymbols[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
	double tempTime;
	double targets[4];
	Event* tempEvent = nullptr;

	rule.evaluateExpressionSymbols(tempos, postureList, model_, ruleSymbols);

	const double timeMultiplier = 1.0 / postureData_[postureIndex].ruleTempo;

	int type = rule.numberOfExpressions();
	setDuration((int) (ruleSymbols[0] * timeMultiplier));

	ruleData_[currentRule_].firstPosture = postureIndex;
	ruleData_[currentRule_].lastPosture = postureIndex + (type - 1);
	ruleData_[currentRule_].beat = (ruleSymbols[1] * timeMultiplier) + (double) zeroRef_;
	ruleData_[currentRule_++].duration = ruleSymbols[0] * timeMultiplier;
	ruleData_.push_back(RuleData());

	switch (type) {
	/* Note: Case 4 should execute all of the below, case 3 the last two */
	case 4:
		if (postureList.size() == 4) {
			postureData_[postureIndex + 3].onset = (double) zeroRef_ + ruleSymbols[1];
			tempEvent = insertEvent(ruleSymbols[3] * timeMultiplier, -1, 0.0);
			if (tempEvent) tempEvent->flag = 1;
		}
	case 3: // falls through
		if (postureList.size() >= 3) {
			postureData_[postureIndex + 2].onset = (double) zeroRef_ + ruleSymbols[1];
			tempEvent = insertEvent(ruleSymbols[2] * timeMultiplier, -1, 0.0);
			if (tempEvent) tempEvent->flag = 1;
		}
	case 2: // falls through
		postureData_[postureIndex + 1].onset = (double) zeroRef_ + ruleSymbols[1];
		tempEvent = insertEvent(0.0, -1, 0.0);
		if (tempEvent) tempEvent->flag = 1;
		break;
	}

	//tempTargets = (List *) [rule parameterList];

	/* Loop through the parameters */
	for (unsigned int i = 0, size = model_.parameterList().size(); i < size; ++i) {
		/* Get actual parameter target values */
		targets[0] = postureList[0]->getParameterTarget(i);
		targets[1] = postureList[1]->getParameterTarget(i);
		targets[2] = (postureList.size() >= 3) ? postureList[2]->getParameterTarget(i) : 0.0;
		targets[3] = (postureList.size() == 4) ? postureList[3]->getParameterTarget(i) : 0.0;

		/* Optimization, Don't calculate if no changes occur */
		cont = 1;
		switch (type) {
		case DIPHONE:
			if (targets[0] == targets[1]) {
				cont = 0;
			}
			break;
		case TRIPHONE:
			if ((targets[0] == targets[1]) && (targets[0] == targets[2])) {
				cont = 0;
			}
			break;
		case TETRAPHONE:
			if ((targets[0] == targets[1]) && (targets[0] == targets[2]) && (targets[0] == targets[3])) {
				cont = 0;
			}
			break;
		}

		insertEvent(0.0, i, targets[0]);

		if (cont) {
			currentType = DIPHONE;
			currentValueDelta = targets[1] - targets[0];
			lastValue = targets[0];
			//lastValue = 0.0;

			const std::shared_ptr<Transition> transition = rule.getParamProfileTransition(i);
			if (!transition) {
				THROW_EXCEPTION(UnavailableResourceException, "Rule transition not found: " << i << '.');
			}

			/* Apply lists to parameter */
			for (unsigned int j = 0; j < transition->pointOrSlopeList().size(); ++j) {
				const Transition::PointOrSlope& pointOrSlope = *transition->pointOrSlopeList()[j];
				if (pointOrSlope.isSlopeRatio()) {
					const auto& slopeRatio = dynamic_cast<const Transition::SlopeRatio&>(pointOrSlope);

					if (slopeRatio.pointList[0]->type != currentType) { //TODO: check pointList.size() > 0
						currentType = slopeRatio.pointList[0]->type;
						targets[currentType - 2] = lastValue;
						currentValueDelta = targets[currentType - 1] - lastValue;
					}
					value = createSlopeRatioEvents(
							slopeRatio, targets[currentType - 2], currentValueDelta,
							min_[i], max_[i], i, timeMultiplier);
				} else {
					const auto& point = dynamic_cast<const Transition::Point&>(pointOrSlope);

					if (point.type != currentType) {
						currentType = point.type;
						targets[currentType - 2] = lastValue;
						currentValueDelta = targets[currentType - 1] - lastValue;
					}
					double pointTime;
					Transition::getPointData(point, model_,
									targets[currentType - 2], currentValueDelta, min_[i], max_[i],
									pointTime, value);
					if (!point.isPhantom) {
						insertEvent(pointTime * timeMultiplier, i, value);
					}
				}
				lastValue = value;
			}
		}
		//else {
		//	insertEvent(i, 0.0, targets[0]);
		//}
	}

	/* Special Event Profiles */
	for (unsigned int i = 0, size = model_.parameterList().size(); i < size; ++i) {
		const std::shared_ptr<Transition> specialTransition = rule.getSpecialProfileTransition(i);
		if (specialTransition) {
			for (unsigned int j = 0; j < specialTransition->pointOrSlopeList().size(); ++j) {
				const Transition::PointOrSlope& pointOrSlope = *specialTransition->pointOrSlopeList()[j];
				const auto& point = dynamic_cast<const Transition::Point&>(pointOrSlope);

				/* calculate time of event */
				tempTime = Transition::getPointTime(point, model_);

				/* Calculate value of event */
				value = ((point.value / 100.0) * (max_[i] - min_[i]));
				//maxValue = value;

				/* insert event into event list */
				insertEvent(tempTime * timeMultiplier, i + 16U, value);
			}
		}
	}

	setZeroRef((int) (ruleSymbols[0] * timeMultiplier) + zeroRef_);
	tempEvent = insertEvent(0.0, -1, 0.0);
	if (tempEvent) tempEvent->flag = 1;
}

void
EventList::generateEventList()
{
	for (unsigned int i = 0; i < 16; i++) { //TODO: replace hard-coded value
		const Parameter& param = model_.getParameter(i);
		min_[i] = (double) param.minimum();
		max_[i] = (double) param.maximum();
	}

	/* Calculate Rhythm including regression */
	for (int i = 0; i < currentFoot_; i++) {
		int rus = feet_[i].end - feet_[i].start + 1;
		/* Apply rhythm model */
		double footTempo;
		if (feet_[i].marked) {
			double tempTempo = 117.7 - (19.36 * (double) rus);
			feet_[i].tempo -= tempTempo / 180.0;
			footTempo = globalTempo_ * feet_[i].tempo;
		} else {
			double tempTempo = 18.5 - (2.08 * (double) rus);
			feet_[i].tempo -= tempTempo / 140.0;
			footTempo = globalTempo_ * feet_[i].tempo;
		}
		for (int j = feet_[i].start; j < feet_[i].end + 1; j++) {
			postureTempo_[j] *= footTempo;
			if (postureTempo_[j] < 0.2) {
				postureTempo_[j] = 0.2;
			} else if (postureTempo_[j] > 2.0) {
				postureTempo_[j] = 2.0;
			}
		}
	}

	unsigned int basePostureIndex = 0;
	std::vector<const Posture*> tempPostureList;
	while (basePostureIndex < currentPosture_) {
		tempPostureList.clear();
		for (unsigned int i = 0; i < 4; i++) {
			unsigned int postureIndex = basePostureIndex + i;
			if (postureIndex <= currentPosture_ && postureData_[postureIndex].posture) {
				tempPostureList.push_back(postureData_[postureIndex].posture);
			} else {
				break;
			}
		}
		if (tempPostureList.size() < 2) {
			break;
		}
		unsigned int ruleIndex = 0;
		const Rule* tempRule = model_.findFirstMatchingRule(tempPostureList, ruleIndex);
		if (tempRule == nullptr) {
			THROW_EXCEPTION(UnavailableResourceException, "Could not find a matching rule.");
		}

		ruleData_[currentRule_].number = ruleIndex + 1U;

		applyRule(*tempRule, tempPostureList, &postureTempo_[basePostureIndex], basePostureIndex);

		basePostureIndex += tempRule->numberOfExpressions() - 1;
	}

	//[dataPtr[numElements-1] setFlag:1];
}

void
EventList::setFullTimeScale()
{
	zeroRef_ = 0;
	zeroIndex_ = 0;
	duration_ = list_.back()->time + 100;
}

void
EventList::applyIntonation()
{
	if (list_.empty()) return;

	int tgRandom;
	int firstFoot, endFoot;
	int ruleIndex = 0, postureIndex;
	int i, j, k;
	double startTime, endTime, pretonicDelta, offsetTime = 0.0;
	double randomSemitone, randomSlope;

	zeroRef_ = 0;
	zeroIndex_ = 0;
	duration_ = list_.back()->time + 100;

	intonationPoints_.clear();

	std::shared_ptr<const Category> vocoidCategory = model_.findCategory("vocoid");
	if (!vocoidCategory) {
		THROW_EXCEPTION(UnavailableResourceException, "Could not find the category \"vocoid\".");
	}

	std::uniform_int_distribution<> intRandDist0(0, tgCount_[0] > 0 ? tgCount_[0] - 1 : 0);
	std::uniform_int_distribution<> intRandDist1(0, tgCount_[1] > 0 ? tgCount_[1] - 1 : 0);
	std::uniform_int_distribution<> intRandDist2(0, tgCount_[2] > 0 ? tgCount_[2] - 1 : 0);
	std::uniform_int_distribution<> intRandDist3(0, tgCount_[3] > 0 ? tgCount_[3] - 1 : 0);

	for (i = 0; i < currentToneGroup_; i++) {
		firstFoot = toneGroups_[i].startFoot;
		endFoot = toneGroups_[i].endFoot;

		startTime = postureData_[feet_[firstFoot].start].onset;
		endTime = postureData_[feet_[endFoot].end].onset;

		//printf("Tg: %d First: %d  end: %d  StartTime: %f  endTime: %f\n", i, firstFoot, endFoot, startTime, endTime);

		if (useFixedIntonationParameters_) {
			intonParms_ = fixedIntonationParameters_;
		} else {
			switch (toneGroups_[i].type) {
			default:
			case TONE_GROUP_TYPE_STATEMENT:
				if (tgUseRandom_) {
					tgRandom = intRandDist0(randSrc_);
				} else {
					tgRandom = 0;
				}
				intonParms_ = &tgParameters_[0][tgRandom * 10];
				break;
			case TONE_GROUP_TYPE_EXCLAMATION:
				if (tgUseRandom_) {
					tgRandom = intRandDist0(randSrc_);
				} else {
					tgRandom = 0;
				}
				intonParms_ = &tgParameters_[0][tgRandom * 10];
				break;
			case TONE_GROUP_TYPE_QUESTION:
				if (tgUseRandom_) {
					tgRandom = intRandDist1(randSrc_);
				} else {
					tgRandom = 0;
				}
				intonParms_ = &tgParameters_[1][tgRandom * 10];
				break;
			case TONE_GROUP_TYPE_CONTINUATION:
				if (tgUseRandom_) {
					tgRandom = intRandDist2(randSrc_);
				} else {
					tgRandom = 0;
				}
				intonParms_ = &tgParameters_[2][tgRandom * 10];
				break;
			case TONE_GROUP_TYPE_SEMICOLON:
				if (tgUseRandom_) {
					tgRandom = intRandDist3(randSrc_);
				} else {
					tgRandom = 0;
				}
				intonParms_ = &tgParameters_[3][tgRandom * 10];
				break;
			}
		}

		//printf("Intonation Parameters: Type : %d  random: %d\n", toneGroups[i].type, tgRandom);
		//for (j = 0; j<6; j++)
		//	printf("%f ", intonParms[j]);
		//printf("\n");

		pretonicDelta = (intonParms_[1]) / (endTime - startTime);
		//printf("Pretonic Delta = %f time = %f\n", pretonicDelta, (endTime - startTime));

		/* Set up intonation boundary variables */
		for (j = firstFoot; j <= endFoot; j++) {
			postureIndex = feet_[j].start;
			while (!postureData_[postureIndex].posture->isMemberOfCategory(*vocoidCategory)) {
				postureIndex++;
				//printf("Checking posture %s for vocoid\n", [posture[postureIndex].posture symbol]);
				if (postureIndex > feet_[j].end) {
					postureIndex = feet_[j].start;
					break;
				}
			}

			if (!feet_[j].marked) {
				for (k = 0; k < currentRule_; k++) {
					if ((postureIndex >= ruleData_[k].firstPosture) && (postureIndex <= ruleData_[k].lastPosture)) {
						ruleIndex = k;
						break;
					}
				}

				if (tgUseRandom_) {
					randomSemitone = randDist_(randSrc_) * intonParms_[3] - intonParms_[3] / 2.0;
					randomSlope = randDist_(randSrc_) * 0.015 + 0.01;
				} else {
					randomSemitone = 0.0;
					randomSlope = 0.02;
				}

				//printf("postureIndex = %d onsetTime : %f Delta: %f\n", postureIndex,
				//	postures[postureIndex].onset-startTime,
				//	((postures[postureIndex].onset-startTime)*pretonicDelta) + intonParms[1] + randomSemitone);

				addIntonationPoint((postureData_[postureIndex].onset - startTime) * pretonicDelta + intonParms_[1] + randomSemitone,
							offsetTime, randomSlope, ruleIndex);
			} else { /* Tonic */
				if (toneGroups_[i].type == 3) {
					randomSlope = 0.01;
				} else {
					randomSlope = 0.02;
				}

				for (k = 0; k < currentRule_; k++) {
					if ((postureIndex >= ruleData_[k].firstPosture) && (postureIndex <= ruleData_[k].lastPosture)) {
						ruleIndex = k;
						break;
					}
				}

				if (tgUseRandom_) {
					randomSemitone = randDist_(randSrc_) * intonParms_[6] - intonParms_[6] / 2.0;
					randomSlope += randDist_(randSrc_) * 0.03;
				} else {
					randomSemitone = 0.0;
					randomSlope += 0.03;
				}
				addIntonationPoint(intonParms_[2] + intonParms_[1] + randomSemitone,
							offsetTime, randomSlope, ruleIndex);

				postureIndex = feet_[j].end;
				for (k = ruleIndex; k < currentRule_; k++) {
					if ((postureIndex >= ruleData_[k].firstPosture) && (postureIndex <= ruleData_[k].lastPosture)) {
						ruleIndex = k;
						break;
					}
				}

				addIntonationPoint(intonParms_[2] + intonParms_[1] + intonParms_[5],
							0.0, 0.0, ruleIndex);
			}
			offsetTime = -40.0;
		}
	}
	if (intonParms_) {
		addIntonationPoint(intonParms_[2] + intonParms_[1] + intonParms_[5],
					0.0, 0.0, currentRule_ - 1);
	}

	if (macroIntonation_) prepareMacroIntonationInterpolation();
}

void
EventList::prepareMacroIntonationInterpolation()
{
	setFullTimeScale();

	if (intonationPoints_.empty()) return;

	Event* event1 = insertEvent(intonationPoints_[0].absoluteTime(), -1, 0.0);
	if (!event1) return;
	Event* event2 {};

	for (unsigned int j = 0; j < intonationPoints_.size() - 1; j++) {
		const IntonationPoint& point1 = intonationPoints_[j];
		const IntonationPoint& point2 = intonationPoints_[j + 1];

		event2 = insertEvent(point2.absoluteTime(), -1, 0.0);
		if (!event2) break;

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
			interpData->c = 0.0;
			interpData->d = 0.0;
		}

		event1->interpData = std::move(interpData);

		event1 = event2;
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
	iPoint.setSemitone(semitone);
	iPoint.setSlope(slope);

	double time = iPoint.absoluteTime();
	for (std::size_t i = 0; i < intonationPoints_.size(); i++) {
		if (time < intonationPoints_[i].absoluteTime()) {
			intonationPoints_.insert(intonationPoints_.begin() + i, iPoint);
			return;
		}
	}

	intonationPoints_.push_back(iPoint);
}

void
EventList::generateOutput(std::ostream& vtmParamStream)
{
	if (list_.size() < 2) {
		return;
	}

	double currentValues[32]; // hardcoded
	double currentDeltas[32]; // hardcoded
	double temp;
	float table[16]; // hardcoded
	double pa {}, pb {}, pc {}, pd {}; // coefficients for polynomial interpolation

	for (int i = 0; i < 16; i++) {
		currentValues[i] = list_[0]->getParameter(i);
		unsigned int j = 1;
		while ((temp = list_[j]->getParameter(i)) == Event::EMPTY_PARAMETER) {
			if (++j >= list_.size()) break;
		}
		if (j < list_.size()) {
			currentDeltas[i] = ((temp - currentValues[i]) / (double) (list_[j]->time)) * 4.0;
		} else {
			currentDeltas[i] = 0.0;
		}
	}
	for (int i = 16; i < 32; i++) {
		currentValues[i] = currentDeltas[i] = 0.0;
	}

	if (macroIntonation_) {
		unsigned int j = 0, size = list_.size();
		for ( ; j < size; ++j) {
			if (list_[j]->interpData) break;
		}
		if (j < size) {
			const double y1 = -20.0; // hardcoded initial pitch
			const double x2 = list_[j]->time;
			const InterpolationData& data = *list_[j]->interpData;
			// Straight line.
			if (smoothIntonation_) {
				const double y2 = x2 * (x2 * (x2 * data.a + data.b) + data.c) + data.d;
				pa = 0.0;
				pb = 0.0;
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
	int currentTime = 0; // absolute time in ms, multiple of 4
	int nextTime = list_[1]->time;
	while (targetIndex < list_.size()) {
		// Add normal parameters and special parameters.
		for (int j = 0; j < 16; j++) {
			table[j] = (float) currentValues[j] + (float) currentValues[j + 16];
		}

		// Intonation.
		if (!microIntonation_) table[0] = 0.0;
		if (intonationDrift_)  table[0] += static_cast<float>(driftGenerator_.drift());
		if (macroIntonation_) {
			const double x = currentTime;
			float intonation;
			if (smoothIntonation_) {
				intonation = x * (x * (x * pa + pb) + pc) + pd;
			} else {
				intonation = x * pa + pb;
			}
			table[0] += intonation;
		}
		table[0] += static_cast<float>(pitchMean_);

		// Send the parameter values to the output stream.
		vtmParamStream << table[0];
		for (int k = 1; k < 16; ++k) {
			vtmParamStream << ' ' << table[k];
		}
		vtmParamStream << '\n';

		// Linear interpolation of the parameters.
		for (int j = 0; j < 32; j++) {
			if (currentDeltas[j]) {
				currentValues[j] += currentDeltas[j];
			}
		}

		currentTime += 4; // hardcoded - 250 Hz

		if (currentTime >= nextTime) {
			// Next interval.
			++targetIndex;
			if (targetIndex == list_.size()) {
				break;
			}
			nextTime = list_[targetIndex]->time;
			for (int j = 0; j < 32; j++) {
				if (list_[targetIndex - 1]->getParameter(j) != Event::EMPTY_PARAMETER) {
					unsigned int k = targetIndex;
					while ((temp = list_[k]->getParameter(j)) == Event::EMPTY_PARAMETER) {
						if (++k >= list_.size()) break;
					}
					if (temp != Event::EMPTY_PARAMETER) {
						// Prepare linear interpolation.
						currentDeltas[j] = (temp - currentValues[j]) / (list_[k]->time - currentTime);
						currentDeltas[j] *= 4.0; // hardcoded - 250 Hz
					} else {
						currentDeltas[j] = 0.0;
					}
				}
			}

			if (macroIntonation_) {
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
EventList::printDataStructures()
{
	printf("Tone Groups %d\n", currentToneGroup_);
	for (int i = 0; i < currentToneGroup_; i++) {
		printf("%d  start: %d  end: %d  type: %d\n", i, toneGroups_[i].startFoot, toneGroups_[i].endFoot,
			toneGroups_[i].type);
	}

	printf("\nFeet %d\n", currentFoot_);
	for (int i = 0; i < currentFoot_; i++) {
		printf("%d  tempo: %f start: %d  end: %d  marked: %d last: %d onset1: %f onset2: %f\n", i, feet_[i].tempo,
			feet_[i].start, feet_[i].end, feet_[i].marked, feet_[i].last, feet_[i].onset1, feet_[i].onset2);
	}

	printf("\nPostures %d\n", currentPosture_);
	for (unsigned int i = 0; i < currentPosture_; i++) {
		printf("DEBUG_POSTURE %u  \"%s\" tempo: %f syllable: %d onset: %f ruleTempo: %f\n",
			 i, postureData_[i].posture->name().c_str(), postureTempo_[i], postureData_[i].syllable, postureData_[i].onset, postureData_[i].ruleTempo);
	}

	printf("\nRules %d\n", currentRule_);
	for (int i = 0; i < currentRule_; i++) {
		printf("Number: %d  start: %d  end: %d  duration %f\n", ruleData_[i].number, ruleData_[i].firstPosture,
			ruleData_[i].lastPosture, ruleData_[i].duration);
	}
#if 0
	printf("\nEvents %lu\n", list_.size());
	for (unsigned int i = 0; i < list_.size(); i++) {
		const Event& event = *list_[i];
		printf("  Event: time=%d flag=%d\n    Values: ", event.time, event.flag);

		for (int j = 0; j < 16; j++) {
			printf("%.3f ", event.getValue(j));
		}
		printf("\n            ");
		for (int j = 16; j < 32; j++) {
			printf("%.3f ", event.getValue(j));
		}
		printf("\n            ");
		for (int j = 32; j < Event::EVENTS_SIZE; j++) {
			printf("%.3f ", event.getValue(j));
		}
		printf("\n");
	}
#endif
}

} /* namespace VTMControlModel */
} /* namespace GS */
