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

#ifndef VTM_CONTROL_MODEL_RULE_H_
#define VTM_CONTROL_MODEL_RULE_H_

#include <memory>
#include <ostream>
#include <string>
#include <utility> /* move */
#include <vector>

#include "Exception.h"



namespace GS {
namespace VTMControlModel {

class Category;
class Equation;
class Model;
class Posture;
class Transition;

// Data for the calculation of the value of expression symbols and boolean expressions.
struct RuleExpressionData {
	const Posture* posture;
	double tempo;
	bool marked;
};

class RuleBooleanNode {
public:
	RuleBooleanNode() = default;
	virtual ~RuleBooleanNode() = default;

	virtual bool eval(const RuleExpressionData& expressionData) const = 0;
	virtual void print(std::ostream& out, int level = 0) const = 0;
private:
	RuleBooleanNode(const RuleBooleanNode&) = delete;
	RuleBooleanNode& operator=(const RuleBooleanNode&) = delete;
	RuleBooleanNode(RuleBooleanNode&&) = delete;
	RuleBooleanNode& operator=(RuleBooleanNode&&) = delete;
};

typedef std::unique_ptr<RuleBooleanNode> RuleBooleanNode_ptr;
// Type of container that manages the RuleBooleanNode instances.
typedef std::vector<RuleBooleanNode_ptr> RuleBooleanNodeList;

class RuleBooleanAndExpression : public RuleBooleanNode {
public:
	RuleBooleanAndExpression(RuleBooleanNode_ptr c1, RuleBooleanNode_ptr c2)
			: RuleBooleanNode(), child1_(std::move(c1)), child2_(std::move(c2)) {}
	virtual ~RuleBooleanAndExpression() = default;

	virtual bool eval(const RuleExpressionData& expressionData) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	RuleBooleanAndExpression(const RuleBooleanAndExpression&) = delete;
	RuleBooleanAndExpression& operator=(const RuleBooleanAndExpression&) = delete;
	RuleBooleanAndExpression(RuleBooleanAndExpression&&) = delete;
	RuleBooleanAndExpression& operator=(RuleBooleanAndExpression&&) = delete;

	RuleBooleanNode_ptr child1_;
	RuleBooleanNode_ptr child2_;
};

class RuleBooleanOrExpression : public RuleBooleanNode {
public:
	RuleBooleanOrExpression(RuleBooleanNode_ptr c1, RuleBooleanNode_ptr c2)
			: RuleBooleanNode(), child1_(std::move(c1)), child2_(std::move(c2)) {}
	virtual ~RuleBooleanOrExpression() = default;

	virtual bool eval(const RuleExpressionData& expressionData) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	RuleBooleanOrExpression(const RuleBooleanOrExpression&) = delete;
	RuleBooleanOrExpression& operator=(const RuleBooleanOrExpression&) = delete;
	RuleBooleanOrExpression(RuleBooleanOrExpression&&) = delete;
	RuleBooleanOrExpression& operator=(RuleBooleanOrExpression&&) = delete;

	RuleBooleanNode_ptr child1_;
	RuleBooleanNode_ptr child2_;
};

class RuleBooleanXorExpression : public RuleBooleanNode {
public:
	RuleBooleanXorExpression(RuleBooleanNode_ptr c1, RuleBooleanNode_ptr c2)
			: RuleBooleanNode(), child1_(std::move(c1)), child2_(std::move(c2)) {}
	virtual ~RuleBooleanXorExpression() = default;

	virtual bool eval(const RuleExpressionData& expressionData) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	RuleBooleanXorExpression(const RuleBooleanXorExpression&) = delete;
	RuleBooleanXorExpression& operator=(const RuleBooleanXorExpression&) = delete;
	RuleBooleanXorExpression(RuleBooleanXorExpression&&) = delete;
	RuleBooleanXorExpression& operator=(RuleBooleanXorExpression&&) = delete;

	RuleBooleanNode_ptr child1_;
	RuleBooleanNode_ptr child2_;
};

class RuleBooleanNotExpression : public RuleBooleanNode {
public:
	explicit RuleBooleanNotExpression(RuleBooleanNode_ptr c)
			: RuleBooleanNode(), child_(std::move(c)) {}
	virtual ~RuleBooleanNotExpression() = default;

	virtual bool eval(const RuleExpressionData& expressionData) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	RuleBooleanNotExpression(const RuleBooleanNotExpression&) = delete;
	RuleBooleanNotExpression& operator=(const RuleBooleanNotExpression&) = delete;
	RuleBooleanNotExpression(RuleBooleanNotExpression&&) = delete;
	RuleBooleanNotExpression& operator=(RuleBooleanNotExpression&&) = delete;

	RuleBooleanNode_ptr child_;
};

class RuleBooleanMarkedExpression : public RuleBooleanNode {
public:
	explicit RuleBooleanMarkedExpression(RuleBooleanNode_ptr c)
			: RuleBooleanNode(), child_(std::move(c)) {}
	virtual ~RuleBooleanMarkedExpression() = default;

	virtual bool eval(const RuleExpressionData& expressionData) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	RuleBooleanMarkedExpression(const RuleBooleanMarkedExpression&) = delete;
	RuleBooleanMarkedExpression& operator=(const RuleBooleanMarkedExpression&) = delete;
	RuleBooleanMarkedExpression(RuleBooleanMarkedExpression&&) = delete;
	RuleBooleanMarkedExpression& operator=(RuleBooleanMarkedExpression&&) = delete;

	RuleBooleanNode_ptr child_;
};

class RuleBooleanTerminal : public RuleBooleanNode {
public:
	explicit RuleBooleanTerminal(const std::shared_ptr<Category>& category)
			: RuleBooleanNode(), category_(category) {}
	virtual ~RuleBooleanTerminal() = default;

	virtual bool eval(const RuleExpressionData& expressionData) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	RuleBooleanTerminal(const RuleBooleanTerminal&) = delete;
	RuleBooleanTerminal& operator=(const RuleBooleanTerminal&) = delete;
	RuleBooleanTerminal(RuleBooleanTerminal&&) = delete;
	RuleBooleanTerminal& operator=(RuleBooleanTerminal&&) = delete;

	const std::shared_ptr<Category> category_;
};

class Rule {
public:
	enum class Type {
		invalid    = 0,
		diphone    = 2,
		triphone   = 3,
		tetraphone = 4
	};
	enum Symbol {
		SYMB_DURATION,
		SYMB_BEAT,
		SYMB_MARK1,
		SYMB_MARK2,
		SYMB_MARK3,
		NUM_SYMBOLS
	};
	struct ExpressionSymbolEquations {
		std::shared_ptr<Equation> duration;
		std::shared_ptr<Equation> beat;
		std::shared_ptr<Equation> mark1;
		std::shared_ptr<Equation> mark2;
		std::shared_ptr<Equation> mark3;
	};

	explicit Rule(unsigned int numParameters);
	~Rule();

	std::size_t numberOfExpressions() const;
	bool evalBooleanExpression(const std::vector<RuleExpressionData>& expressionData) const;
	bool evalBooleanExpression(const RuleExpressionData& expressionData, unsigned int expressionIndex) const;
	void printBooleanNodeTree() const;

	ExpressionSymbolEquations& exprSymbolEquations() { return exprSymbolEquations_; }
	const ExpressionSymbolEquations& exprSymbolEquations() const { return exprSymbolEquations_; }

	const std::shared_ptr<Transition>& getParamProfileTransition(unsigned int parameterIndex) const {
		if (parameterIndex >= paramProfileTransitionList_.size()) {
			THROW_EXCEPTION(InvalidParameterException, "Invalid parameter index: " << parameterIndex << '.');
		}

		return paramProfileTransitionList_[parameterIndex];
	}
	void setParamProfileTransition(unsigned int parameterIndex, const std::shared_ptr<Transition>& transition) {
		if (parameterIndex >= paramProfileTransitionList_.size()) {
			THROW_EXCEPTION(InvalidParameterException, "Invalid parameter index: " << parameterIndex << '.');
		}

		paramProfileTransitionList_[parameterIndex] = transition;
	}

	const std::shared_ptr<Transition>& getSpecialProfileTransition(unsigned int parameterIndex) const {
		if (parameterIndex >= specialProfileTransitionList_.size()) {
			THROW_EXCEPTION(InvalidParameterException, "Invalid parameter index: " << parameterIndex << '.');
		}

		return specialProfileTransitionList_[parameterIndex]; // may return an empty string
	}
	void setSpecialProfileTransition(unsigned int parameterIndex, const std::shared_ptr<Transition>& transition) {
		if (parameterIndex >= specialProfileTransitionList_.size()) {
			THROW_EXCEPTION(InvalidParameterException, "Invalid parameter index: " << parameterIndex << '.');
		}

		specialProfileTransitionList_[parameterIndex] = transition;
	}

	void evaluateExpressionSymbols(const std::vector<RuleExpressionData>& expressionData, Model& model, double* ruleSymbols) const;

	const std::vector<std::string>& booleanExpressionList() const { return booleanExpressionList_; }
	void setBooleanExpressionList(const std::vector<std::string>& exprList, const Model& model);

	const std::vector<std::shared_ptr<Transition>>& paramProfileTransitionList() const { return paramProfileTransitionList_; }
	std::vector<std::shared_ptr<Transition>>& paramProfileTransitionList() { return paramProfileTransitionList_; }

	const std::vector<std::shared_ptr<Transition>>& specialProfileTransitionList() const { return specialProfileTransitionList_; }
	std::vector<std::shared_ptr<Transition>>& specialProfileTransitionList() { return specialProfileTransitionList_; }

	const std::string& comment() const { return comment_; }
	void setComment(const std::string& comment) { comment_ = comment; }

	Type type() const { return type_; }

	void validate(const Model& model) const;
private:
	Rule(const Rule&) = delete;
	Rule& operator=(const Rule&) = delete;
	Rule(Rule&&) = delete;
	Rule& operator=(Rule&&) = delete;

	std::vector<std::string> booleanExpressionList_;
	std::vector<std::shared_ptr<Transition>> paramProfileTransitionList_;
	std::vector<std::shared_ptr<Transition>> specialProfileTransitionList_;
	ExpressionSymbolEquations exprSymbolEquations_;
	std::string comment_;
	RuleBooleanNodeList booleanNodeList_;
	Type type_;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_RULE_H_ */
