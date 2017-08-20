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

#include <cstddef> /* std::size_t */
#include <memory>
#include <vector>

#include "DriftGenerator.h"
#include "IntonationPoint.h"
#include "IntonationRhythm.h"
#include "Model.h"



namespace GS {
namespace VTMControlModel {

struct PostureData {
	const Posture* posture;
	double         tempo;
	double         ruleTempo;
	double         onset;
	int            syllable;
	bool           marked;
	PostureData()
		: posture{}
		, tempo{1.0}
		, ruleTempo{1.0}
		, onset{}
		, syllable{}
		, marked{} {}
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
		: onset1{}
		, onset2{}
		, tempo{1.0}
		, start{}
		, end{}
		, marked{}
		, last{} {}
};

struct ToneGroup {
	int startFoot;
	int endFoot;
	IntonationRhythm::ToneGroup type;
	ToneGroup()
		: startFoot{}
		, endFoot{}
		, type{IntonationRhythm::ToneGroup::none} {}
};

struct RuleData {
	int    number;
	int    firstPosture;
	int    lastPosture;
	int    start;
	double mark1;
	double mark2;
	double duration;
	double beat;
	RuleData()
		: number{}
		, firstPosture{}
		, lastPosture{}
		, start{}
		, mark1{}
		, mark2{}
		, duration{}
		, beat{} {}
};

struct InterpolationData {
	double a;
	double b;
	double c;
	double d;
	InterpolationData()
		: a{}
		, b{}
		, c{}
		, d{} {}
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

	void setInitialPitch(double value) { initialPitch_ = value; }
	double initialPitch() const { return initialPitch_; }

	void setMeanPitch(double value) { meanPitch_ = value; }
	double meanPitch() const { return meanPitch_; }

	void setGlobalTempo(double value) { globalTempo_ = value; }
	double globalTempo() const { return globalTempo_; }

	void setMacroIntonation(bool value) { macroIntonation_ = value; }
	bool macroIntonation() const { return macroIntonation_; }

	void setMicroIntonation(bool value) { microIntonation_ = value; }
	bool microIntonation() const { return microIntonation_; }

	void setIntonationDrift(bool value) { intonationDrift_ = value; }
	bool intonationDrift() const { return intonationDrift_; }

	void setSmoothIntonation(bool value) { smoothIntonation_ = value; }
	bool smoothIntonation() const { return smoothIntonation_; }

	void setDuration(int value) { duration_ = value; }

	void setRandomIntonation(bool value) {
		intonationRhythm_.setRandomIntonation(value);
	}

	void setIntonationFactor(float value) { intonationFactor_ = value; }

	void setCurrentPostureSyllable();
	void setUp();
	double getBeatAtIndex(int ruleIndex) const;
	unsigned int newPostureWithObject(const Posture& p, bool marked=false);
	void replaceCurrentPostureWith(const Posture& p, bool marked=false);
	void setCurrentToneGroupType(IntonationRhythm::ToneGroup type);
	void newFoot();
	void setCurrentFootMarked();
	void setCurrentFootLast();
	void setCurrentFootTempo(double tempo);
	void setCurrentPostureTempo(double tempo);
	void setCurrentPostureRuleTempo(double tempo);
	void newToneGroup();
	void generateEventList();
	void applyIntonation();
	void applyRhythm();
	void prepareMacroIntonationInterpolation();
	void generateOutput(std::vector<std::vector<float>>& vtmParamList);
	void clearMacroIntonation();
	void setControlPeriod(int value);

	void setUpDriftGenerator(double deviation, double sampleRate, double lowpassCutoff);

	const Posture* getPostureAtIndex(unsigned int index) const;
	const PostureData* getPostureDataAtIndex(unsigned int index) const;
	int numberOfRules() const { return currentRule_; }
	const RuleData* getRuleDataAtIndex(unsigned int index) const;

	void setUseFixedIntonationParameters(bool value) { intonationRhythm_.setUseFixedIntonationParameters(value); }
	void setFixedIntonationParameters(
			float notionalPitch,
			float pretonicPitchRange,
			float pretonicPerturbationRange,
			float tonicPitchRange,
			float tonicPerturbationRange);

	void addPostureIntonationPoint(int postureIndex, double position, double semitone);
private:
	EventList(const EventList&) = delete;
	EventList& operator=(const EventList&) = delete;

	void addIntonationPoint(double semitone, double offsetTime, double slope, int ruleIndex);
	void setFullTimeScale();
	void newPosture();
	Event* insertEvent(double time, int parameter, double value, bool special);
	void setZeroRef(int newValue);
	void applyRule(const Rule& rule, const std::vector<RuleExpressionData>& ruleExpressionData, unsigned int basePostureIndex,
			const std::vector<double>& minParam, const std::vector<double>& maxParam);
	void printDataStructures();
	double createSlopeRatioEvents(const Transition::SlopeRatio& slopeRatio,
			double baseline, double parameterDelta, double min, double max, int parameter, double timeMultiplier, bool lastGroup);

	Model& model_;

	int zeroRef_;
	int zeroIndex_;
	int duration_;
	int controlPeriod_;
	bool macroIntonation_;
	bool microIntonation_;
	bool intonationDrift_;
	bool smoothIntonation_;

	double initialPitch_;
	double meanPitch_;
	double globalTempo_;

	std::vector<PostureData> postureData_;
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
	float intonationFactor_;
	IntonationRhythm intonationRhythm_;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_EVENT_LIST_H_ */
