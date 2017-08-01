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

#ifndef VTM_CONTROL_MODEL_EVENT_LIST_H_
#define VTM_CONTROL_MODEL_EVENT_LIST_H_

#include <cassert>
#include <cstddef> /* std::size_t */
#include <memory>
#include <ostream>
#include <random>
#include <vector>

#include "DriftGenerator.h"
#include "IntonationPoint.h"
#include "Model.h"

#define TONE_GROUP_TYPE_STATEMENT    0
#define TONE_GROUP_TYPE_EXCLAMATION  1
#define TONE_GROUP_TYPE_QUESTION     2
#define TONE_GROUP_TYPE_CONTINUATION 3
#define TONE_GROUP_TYPE_SEMICOLON    4



namespace GS {
namespace VTMControlModel {

struct PostureData {
	const Posture* posture;
	int    syllable;
	double onset;
	float  ruleTempo;
	PostureData()
		: posture(nullptr)
		, syllable(0)
		, onset(0.0)
		, ruleTempo(0.0) {}
};

struct Foot {
	double onset1;
	double onset2;
	double tempo;
	int start;
	int end;
	int marked;
	int last;
	Foot()
		: onset1(0.0)
		, onset2(0.0)
		, tempo(1.0)
		, start(0)
		, end(0)
		, marked(0)
		, last(0) {}
};

struct ToneGroup {
	int startFoot;
	int endFoot;
	int type;
	ToneGroup()
		: startFoot(0)
		, endFoot(0)
		, type(0) {}
};

struct RuleData {
	int    number;
	int    firstPosture;
	int    lastPosture;
	double duration;
	double beat;
	RuleData()
		: number(0)
		, firstPosture(0)
		, lastPosture(0)
		, duration(0.0)
		, beat(0.0) {}
};

struct InterpolationData {
	double a;
	double b;
	double c;
	double d;
};

struct Event {
	static const double EMPTY_PARAMETER;

	Event(unsigned int numParameters)
			: parameters(numParameters, EMPTY_PARAMETER)
			, specialParameters(numParameters, EMPTY_PARAMETER)
			, time{}
			, flag{}
			, interpData{}
	{
	}

	void setParameter(int index, double value, bool special) {
		if (index < 0 || static_cast<std::size_t>(index) >= parameters.size()) {
			THROW_EXCEPTION(InvalidValueException, "Invalid parameter index: " << index << " (special: " << special << ").");
		}
		if (special) {
			specialParameters[index] = value;
		} else {
			parameters[index] = value;
		}
	}
	double getParameter(int index, bool special) const {
		if (index < 0 || static_cast<std::size_t>(index) >= specialParameters.size()) {
			THROW_EXCEPTION(InvalidValueException, "Invalid parameter index: " << index << " (special: " << special << ").");
		}
		if (special) {
			return specialParameters[index];
		} else {
			return parameters[index];
		}
	}

	std::vector<double> parameters;
	std::vector<double> specialParameters;
	int time;
	int flag; // 1 when it is the first or the last event of the rule (this flag is used only for debug)
	std::unique_ptr<InterpolationData> interpData;
};
typedef std::unique_ptr<Event> Event_ptr;



class EventList {
public:
	EventList(const char* configDirPath, Model& model);
	~EventList();

	const std::vector<Event_ptr>& list() const { return list_; }
	std::vector<IntonationPoint>& intonationPoints() { return intonationPoints_; }

	void setPitchMean(double newMean) { pitchMean_ = newMean; }
	double pitchMean() const { return pitchMean_; }

	void setGlobalTempo(double newTempo) { globalTempo_ = newTempo; }
	double globalTempo() const { return globalTempo_; }

	void setMacroIntonation(bool value) { macroIntonation_ = value; }
	bool macroIntonation() const { return macroIntonation_; }

	void setMicroIntonation(bool value) { microIntonation_ = value; }
	bool microIntonation() const { return microIntonation_; }

	void setIntonationDrift(bool value) { intonationDrift_ = value; }
	bool intonationDrift() const { return intonationDrift_; }

	void setSmoothIntonation(bool value) { smoothIntonation_ = value; }
	bool smoothIntonation() const { return smoothIntonation_; }

	void setDuration(int newValue) { duration_ = newValue; }

	void setTgUseRandom(bool tgUseRandom) { tgUseRandom_ = tgUseRandom; }
	bool tgUseRandom() const { return tgUseRandom_; }

	void setIntonationFactor(float value) { intonationFactor_ = value; }

	void setCurrentPostureSyllable();
	void setUp();
	double getBeatAtIndex(int ruleIndex) const;
	void newPostureWithObject(const Posture& p);
	void replaceCurrentPostureWith(const Posture& p);
	void setCurrentToneGroupType(int type);
	void newFoot();
	void setCurrentFootMarked();
	void setCurrentFootLast();
	void setCurrentFootTempo(double tempo);
	void setCurrentPostureTempo(double tempo);
	void setCurrentPostureRuleTempo(float tempo);
	void newToneGroup();
	void generateEventList();
	void applyIntonation();
	void prepareMacroIntonationInterpolation();
	void generateOutput(std::ostream& vtmParamStream);
	void clearMacroIntonation();
	void setControlPeriod(int value);

	void setUpDriftGenerator(double deviation, double sampleRate, double lowpassCutoff);

	const Posture* getPostureAtIndex(unsigned int index) const;
	const PostureData* getPostureDataAtIndex(unsigned int index) const;
	int numberOfRules() const { return currentRule_; }
	const RuleData* getRuleAtIndex(unsigned int index) const;

	void setUseFixedIntonationParameters(bool value) { useFixedIntonationParameters_ = value; }
	void setFixedIntonationParameters(float notionalPitch, float pretonicRange, float pretonicLift, float tonicRange, float tonicMovement);
private:
	EventList(const EventList&) = delete;
	EventList& operator=(const EventList&) = delete;

	void parseGroups(int index, int number, FILE* fp);
	void initToneGroups(const char* configDirPath);
	void printToneGroups();
	void addIntonationPoint(double semitone, double offsetTime, double slope, int ruleIndex);
	void setFullTimeScale();
	void newPosture();
	Event* insertEvent(double time, int parameter, double value, bool special);
	void setZeroRef(int newValue);
	void applyRule(const Rule& rule, const std::vector<const Posture*>& postureList, const double* tempos, int postureIndex,
			const std::vector<double>& minParam, const std::vector<double>& maxParam);
	void printDataStructures();
	double createSlopeRatioEvents(const Transition::SlopeRatio& slopeRatio,
			double baseline, double parameterDelta, double min, double max, int eventIndex, double timeMultiplier);

	Model& model_;

	int zeroRef_;
	int zeroIndex_;
	int duration_;
	int controlPeriod_;
	bool macroIntonation_;
	bool microIntonation_;
	bool intonationDrift_;
	bool smoothIntonation_;

	double pitchMean_;
	double globalTempo_;
	float* intonParms_;

	/* NOTE postureData and postureTempo are separate for optimization reasons */
	std::vector<PostureData> postureData_;
	std::vector<double> postureTempo_;
	unsigned int currentPosture_;

	std::vector<Foot> feet_;
	int currentFoot_;

	std::vector<ToneGroup> toneGroups_;
	int currentToneGroup_;

	std::vector<RuleData> ruleData_;
	int currentRule_;

	std::vector<IntonationPoint> intonationPoints_;
	std::vector<Event_ptr> list_;
	DriftGenerator driftGenerator_;

	bool tgUseRandom_;
	float intonationRandom_;
	std::vector<std::vector<float>> tgParameters_;
	int tgCount_[5];

	bool useFixedIntonationParameters_;
	float fixedIntonationParameters_[10];

	float intonationFactor_;

	std::random_device randDev_;
	std::mt19937 randSrc_;
	std::uniform_real_distribution<> randDist_;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_EVENT_LIST_H_ */
