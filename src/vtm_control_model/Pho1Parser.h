/***************************************************************************
 *  Copyright 2017 Marcelo Y. Matuda                                       *
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

#ifndef VTM_CONTROL_MODEL_PHO1_PARSER_H_
#define VTM_CONTROL_MODEL_PHO1_PARSER_H_

#include <list>
#include <string>
#include <vector>

#include "StringMap.h"



namespace GS {
namespace VTMControlModel {

class EventList;
class Model;

struct Pho1IntonationPoint {
	Pho1IntonationPoint(float pos, float freq) : position(pos), frequency(freq) {}
	float position;
	float frequency;
};

struct Pho1Data {
	Pho1Data() : duration(1.0), postureIndex() {}
	std::string phoneme;
	float duration;
	unsigned int postureIndex;
	std::vector<Pho1IntonationPoint> intonationPoints;
};

// Parser for MBROLA file format.
// Only phoneme commands are parsed.
// It doesn't parse intonation in the format (NN,NN).
class Pho1Parser {
public:
	Pho1Parser(const char* configDirPath, const Model& model, EventList& eventList, const char* phonemeMapFile);
	~Pho1Parser();

	void parse(const std::string& pho);
private:
	Pho1Parser(const Pho1Parser&) = delete;
	Pho1Parser& operator=(const Pho1Parser&) = delete;

	void loadInputData(const std::string& pho);
	void replacePhonemes();
	void fillPostureList();
	void addIntonation();
	void printInputData();

	const Model& model_;
	EventList& eventList_;
	StringMap phonemeMap_;
	std::list<Pho1Data> inputData_;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_PHO1_PARSER_H_ */
