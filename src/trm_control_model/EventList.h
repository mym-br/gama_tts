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

#ifndef TRM_CONTROL_MODEL_EVENT_LIST_H_
#define TRM_CONTROL_MODEL_EVENT_LIST_H_

#include <memory>
#include <vector>

#include "DriftGenerator.h"
#include "IntonationPoint.h"
#include "Model.h"

#define TONE_GROUP_TYPE_STATEMENT    0
#define TONE_GROUP_TYPE_EXCLAIMATION 1
#define TONE_GROUP_TYPE_QUESTION     2
#define TONE_GROUP_TYPE_CONTINUATION 3
#define TONE_GROUP_TYPE_SEMICOLON    4



namespace TRMControlModel {

struct PhoneData {
	const Phone* phone;
	int    syllable;
	double onset;
	float  ruleTempo;
	PhoneData()
		: phone(nullptr)
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
	int    firstPhone;
	int    lastPhone;
	double duration;
	double beat;
	RuleData()
		: number(0)
		, firstPhone(0)
		, lastPhone(0)
		, duration(0.0)
		, beat(0.0) {}
};

struct Event {
	enum {
		EVENTS_SIZE = 36
	};
	Event();

	void setValue(double value, int index) {
		if (index < 0) return;
		events[index] = value;
	}
	double getValue(int index) const {
		return events[index];
	}

	int time;
	int flag;
	double events[EVENTS_SIZE];
};
typedef std::unique_ptr<Event> Event_ptr;



class EventList {
public:
	EventList(const char* configDirPath, Model& model);
	~EventList();

	void setPitchMean(double newMean) { pitchMean_ = newMean; }
	double pitchMean() const { return pitchMean_; }

	void setGlobalTempo(double newTempo) { globalTempo_ = newTempo; }
	double globalTempo() const { return globalTempo_; }

	void setMacroIntonation(int newValue) { macroFlag_ = newValue; }
	int macroIntonation() const { return macroFlag_; }

	void setMicroIntonation(int newValue) { microFlag_ = newValue; }
	int microIntonation() const { return microFlag_; }

	void setDrift(int newValue) { driftFlag_ = newValue; }
	int drift() const { return driftFlag_; }

	void setSmoothIntonation(int newValue) { smoothIntonation_ = newValue; }
	int smoothIntonation() const { return smoothIntonation_; }

	void setDuration(int newValue) { duration_ = newValue; }

	void setTgUseRandom(bool tgUseRandom) { tgUseRandom_ = tgUseRandom; }
	bool tgUseRandom() const { return tgUseRandom_; }
	void setCurrentPhoneSyllable();
	void setUp();
	double getBeatAtIndex(int ruleIndex) const;
	void newPhoneWithObject(const Phone& p);
	void replaceCurrentPhoneWith(const Phone& p);
	void setCurrentToneGroupType(int type);
	void newFoot();
	void setCurrentFootMarked();
	void setCurrentFootLast();
	void setCurrentFootTempo(double tempo);
	void setCurrentPhoneTempo(double tempo);
	void setCurrentPhoneRuleTempo(float tempo);
	void newToneGroup();
	void generateEventList();
	void applyIntonation();
	void applyIntonationSmooth();
	void generateOutput(const char* trmParamFile);

private:
	EventList(const EventList&);
	EventList& operator=(const EventList&);

	void parseGroups(int index, int number, FILE* fp);
	void initToneGroups(const char* configDirPath);
	void printToneGroups();
	void addIntonationPoint(double semitone, double offsetTime, double slope, int ruleIndex);
	void setFullTimeScale();
	void newPhone();
	Event* insertEvent(int number, double time, double value);
	void setZeroRef(int newValue);
	void applyRule(const Rule& rule, const PhoneSequence& phoneList, const double* tempos, int phoneIndex);
	void printDataStructures();
	double createSlopeRatioEvents(const Transition::SlopeRatio& slopeRatio,
			double baseline, double parameterDelta, double min, double max, int eventIndex);

	Model& model_;

	int zeroRef_;
	int zeroIndex_;
	int duration_;
	int timeQuantization_;
	int macroFlag_;
	int microFlag_;
	int driftFlag_;
	int smoothIntonation_;

	double radiusMultiply_;
	double pitchMean_;
	double globalTempo_;
	double multiplier_;
	float* intonParms_;

	/* NOTE phoneData and phoneTempo are separate for optimization reasons */
	std::vector<PhoneData> phoneData_;
	std::vector<double> phoneTempo_;
	int currentPhone_;

	std::vector<Foot> feet_;
	int currentFoot_;

	std::vector<ToneGroup> toneGroups_;
	int currentToneGroup_;

	std::vector<RuleData> ruleData_;
	int currentRule_;

	double min_[16];
	double max_[16];

	std::vector<IntonationPoint_ptr> intonationPoints_;
	std::vector<Event_ptr> list_;
	DriftGenerator driftGenerator_;

	bool tgUseRandom_;
	float intonationRandom_;
	std::vector<std::vector<float>> tgParameters_;
	int tgCount_[5];
};

} /* namespace TRMControlModel */

#endif /* TRM_CONTROL_MODEL_EVENT_LIST_H_ */
