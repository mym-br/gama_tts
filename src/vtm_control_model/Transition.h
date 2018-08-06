/***************************************************************************
 *  Copyright 2014 Marcelo Y. Matuda                                       *
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
// This file was created by Marcelo Y. Matuda, and code/information
// from Gnuspeech was added to it later.

#ifndef VTM_CONTROL_MODEL_TRANSITION_H_
#define VTM_CONTROL_MODEL_TRANSITION_H_

#include <string>
#include <memory>
#include <vector>

#include "Equation.h"
#include "Exception.h"



namespace GS {
namespace VTMControlModel {

class Model;

class Transition {
public:
	enum class Type {
		invalid    = 0,
		diphone    = 2,
		triphone   = 3,
		tetraphone = 4
	};

	struct PointOrSlope {
		virtual ~PointOrSlope() {}
		virtual bool isSlopeRatio() const = 0;
	};
	typedef std::unique_ptr<PointOrSlope> PointOrSlope_ptr;

	struct Point : PointOrSlope {
		enum class Type {
			invalid    = 0,
			diphone    = 2, // Posture 1 --> Posture 2
			triphone   = 3, // Posture 2 --> Posture 3
			tetraphone = 4  // Posture 3 --> Posture 4
		};
		Type type;
		float value;

		// If timeExpression is not empty, time = timeExpression, otherwise time = freeTime.
		std::shared_ptr<Equation> timeExpression;
		float freeTime; // milliseconds

		Point() : type(Type::invalid), value(), timeExpression(), freeTime() {}
		virtual ~Point() {}

		virtual bool isSlopeRatio() const { return false; }

		static Type getTypeFromName(const std::string& typeName) {
			if (typeName == "diphone") {
				return Transition::Point::Type::diphone;
			} else if (typeName == "triphone") {
				return Transition::Point::Type::triphone;
			} else if (typeName == "tetraphone") {
				return Transition::Point::Type::tetraphone;
			} else {
				THROW_EXCEPTION(VTMControlModelException, "Invalid transition point type: " << typeName << '.');
			}
		}
		static std::string getNameFromType(Point::Type type) {
			switch (type) {
			case Transition::Point::Type::invalid:
				return "invalid";
			case Transition::Point::Type::diphone:
				return "diphone";
			case Transition::Point::Type::triphone:
				return "triphone";
			case Transition::Point::Type::tetraphone:
				return "tetraphone";
			default:
				return "---";
			}
		}
	private:
		Point(const Point&) = delete;
		Point& operator=(const Point&) = delete;
	};
	typedef std::unique_ptr<Point> Point_ptr;

	struct Slope {
		float slope;

		Slope() : slope() {}
	private:
		Slope(const Slope&) = delete;
		Slope& operator=(const Slope&) = delete;
	};
	typedef std::unique_ptr<Slope> Slope_ptr;

	struct SlopeRatio : PointOrSlope {
		std::vector<Point_ptr> pointList;
		std::vector<Slope_ptr> slopeList;

		SlopeRatio() {}
		virtual ~SlopeRatio() {}

		virtual bool isSlopeRatio() const { return true; }
	private:
		SlopeRatio(const SlopeRatio&) = delete;
		SlopeRatio& operator=(const SlopeRatio&) = delete;
	};

	Transition(const std::string& name, Type type, bool special)
			: name_(name), type_(type), special_(special)
	{
	}

	const std::string& name() const { return name_; }
	void setName(const std::string& name) { name_ = name; }

	Type type() const { return type_; }
	void setType(Type type) { type_ = type; }

	bool special() const { return special_; }

	const std::string& comment() const { return comment_; }
	void setComment(const std::string& comment) { comment_ = comment; }

	std::vector<PointOrSlope_ptr>& pointOrSlopeList() { return pointOrSlopeList_; }
	const std::vector<PointOrSlope_ptr>& pointOrSlopeList() const { return pointOrSlopeList_; }

	static double getPointTime(const Transition::Point& point, const Model& model);
	static void getPointData(const Transition::Point& point, const Model& model,
					double& time, double& value);
	static void getPointData(const Transition::Point& point, const Model& model,
					double baseline, double delta, double min, double max,
					double& time, double& value);
	static Type getTypeFromName(const std::string& typeName) {
		if (typeName == "diphone") {
			return Type::diphone;
		} else if (typeName == "triphone") {
			return Type::triphone;
		} else if (typeName == "tetraphone") {
			return Type::tetraphone;
		} else {
			THROW_EXCEPTION(VTMControlModelException, "Invalid transition type: " << typeName << '.');
		}
	}
	static std::string getNameFromType(Type type) {
		switch (type) {
		case Type::invalid:
			return "invalid";
		case Type::diphone:
			return "diphone";
		case Type::triphone:
			return "triphone";
		case Type::tetraphone:
			return "tetraphone";
		default:
			return "---";
		}
	}
private:
	std::string name_;
	Type type_;
	bool special_;
	std::vector<PointOrSlope_ptr> pointOrSlopeList_;
	std::string comment_;
};

struct TransitionGroup {
	std::string name;
	std::vector<std::shared_ptr<Transition>> transitionList;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_TRANSITION_H_ */
