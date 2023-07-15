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

#include "Rule.h"

#include <cassert>
#include <cctype> /* isspace */
#include <iostream>
#include <string_view>

#include "Category.h"
#include "Equation.h"
#include "Model.h"
#include "Posture.h"
#include "Text.h"
#include "Transition.h"



namespace {

using namespace GS::VTMControlModel;

constexpr char rightParenChar = ')';
constexpr char  leftParenChar = '(';
constexpr std::string_view     orOpSymb = "or";
constexpr std::string_view    notOpSymb = "not";
constexpr std::string_view    xorOpSymb = "xor";
constexpr std::string_view    andOpSymb = "and";
constexpr std::string_view markedOpSymb = "marked";

class Parser {
public:
	Parser(const std::string& s, const Model& model)
				: model_(model)
				, s_(GS::Text::trim(s))
				, pos_(0)
				, symbolType_(SymbolType::invalid) {
		if (s_.empty()) {
			THROW_EXCEPTION(GS::VTMControlModelException, "Boolean expression parser error: Empty string.");
		}
		nextSymbol();
	}

	RuleBooleanNode_ptr parse();
private:
	enum class SymbolType {
		invalid,
		orOp,
		notOp,
		xorOp,
		andOp,
		markedOp,
		rightParen,
		leftParen,
		string
	};

	[[noreturn]] void throwException(const char* errorDescription) const;
	template<typename T> [[noreturn]] void throwException(const char* errorDescription, const T& complement) const;
	bool finished() const {
		return pos_ >= s_.size();
	}
	void nextSymbol();
	RuleBooleanNode_ptr getBooleanNode();
	void skipSpaces();

	static bool isSeparator(char c) {
		switch (c) {
		case rightParenChar: return true;
		case leftParenChar:  return true;
		default:             return std::isspace(c) != 0;
		}
	}

	const Model& model_;
	const std::string s_;
	std::string::size_type pos_;
	std::string symbol_;
	SymbolType symbolType_;
};

void
Parser::throwException(const char* errorDescription) const
{
	THROW_EXCEPTION(GS::VTMControlModelException, "Boolean expression parser error: "
				<< errorDescription
				<< " at position " << (pos_ - symbol_.size()) << " of string [" << s_ << "].");
}

template<typename T>
void
Parser::throwException(const char* errorDescription, const T& complement) const
{
	THROW_EXCEPTION(GS::VTMControlModelException, "Boolean expression parser error: "
				<< errorDescription << complement
				<< " at position " << (pos_ - symbol_.size()) << " of string [" << s_ << "].");
}

void
Parser::skipSpaces()
{
	while (!finished() && std::isspace(s_[pos_])) ++pos_;
}

void
Parser::nextSymbol()
{
	skipSpaces();

	symbol_.resize(0);

	if (finished()) {
		symbolType_ = SymbolType::invalid;
		return;
	}

	char c = s_[pos_++];
	symbol_ = c;
	switch (c) {
	case rightParenChar:
		symbolType_ = SymbolType::rightParen;
		break;
	case leftParenChar:
		symbolType_ = SymbolType::leftParen;
		break;
	default:
		while ( !finished() && !isSeparator(c = s_[pos_]) ) {
			symbol_ += c;
			++pos_;
		}
		if (symbol_ == orOpSymb) {
			symbolType_ = SymbolType::orOp;
		} else if (symbol_ == andOpSymb) {
			symbolType_ = SymbolType::andOp;
		} else if (symbol_ == notOpSymb) {
			symbolType_ = SymbolType::notOp;
		} else if (symbol_ == xorOpSymb) {
			symbolType_ = SymbolType::xorOp;
		} else if (symbol_ == markedOpSymb) {
			symbolType_ = SymbolType::markedOp;
		} else {
			symbolType_ = SymbolType::string;
		}
	}
}

RuleBooleanNode_ptr
Parser::getBooleanNode()
{
	switch (symbolType_) {
	case SymbolType::leftParen:
	{
		RuleBooleanNode_ptr p;

		nextSymbol();
		if (symbolType_ == SymbolType::markedOp) {
			// Operand.
			nextSymbol();
			RuleBooleanNode_ptr op(getBooleanNode());

			p = std::make_unique<RuleBooleanMarkedExpression>(std::move(op));
		} else if (symbolType_ == SymbolType::notOp) {
			// Operand.
			nextSymbol();
			RuleBooleanNode_ptr op(getBooleanNode());

			p = std::make_unique<RuleBooleanNotExpression>(std::move(op));
		} else {
			// 1st operand.
			RuleBooleanNode_ptr op1(getBooleanNode());

			// Operator.
			switch (symbolType_) {
			case SymbolType::orOp:
			{	// 2nd operand.
				nextSymbol();
				RuleBooleanNode_ptr op2(getBooleanNode());

				p = std::make_unique<RuleBooleanOrExpression>(std::move(op1), std::move(op2));
				break;
			}
			case SymbolType::andOp:
			{	// 2nd operand.
				nextSymbol();
				RuleBooleanNode_ptr op2(getBooleanNode());

				p = std::make_unique<RuleBooleanAndExpression>(std::move(op1), std::move(op2));
				break;
			}
			case SymbolType::xorOp:
			{	// 2nd operand.
				nextSymbol();
				RuleBooleanNode_ptr op2(getBooleanNode());

				p = std::make_unique<RuleBooleanXorExpression>(std::move(op1), std::move(op2));
				break;
			}
			case SymbolType::notOp:
				throwException("Invalid operator");
			default:
				throwException("Missing operator");
			}
		}

		if (symbolType_ != SymbolType::rightParen) {
			throwException("Right parenthesis not found");
		}
		nextSymbol();
		return p;
	}
	case SymbolType::string:
	{
		std::shared_ptr<Category> category;
		const Posture* posture = model_.postureList().find(symbol_);
		if (posture) {
			category = posture->findCategory(symbol_);
		} else {
			category = model_.findCategory(symbol_);
		}
		if (!category) {
			throwException("Could not find category: ", symbol_);
		}

		nextSymbol();
		return std::make_unique<RuleBooleanTerminal>(category);
	}
	case SymbolType::orOp:
		throwException("Unexpected OR op.");
	case SymbolType::notOp:
		throwException("Unexpected NOT op.");
	case SymbolType::xorOp:
		throwException("Unexpected XOR op.");
	case SymbolType::andOp:
		throwException("Unexpected AND op.");
	case SymbolType::rightParen:
		throwException("Unexpected right parenthesis");
	default:
		throwException("Missing symbol");
	}
	// Unreachable.
}

RuleBooleanNode_ptr
Parser::parse()
{
	RuleBooleanNode_ptr booleanRoot = getBooleanNode();
	if (symbolType_ != SymbolType::invalid) { // there is a symbol available
		throwException("Invalid text");
	}
	return booleanRoot;
}

} /* namespace */

//==============================================================================

namespace GS {
namespace VTMControlModel {

bool
RuleBooleanAndExpression::eval(const RuleExpressionData& expressionData) const
{
	assert(child1_.get() != 0 && child2_.get() != 0);

	return child1_->eval(expressionData) && child2_->eval(expressionData);
}

void
RuleBooleanAndExpression::print(std::ostream& out, int level) const
{
	assert(child1_.get() != 0 && child2_.get() != 0);

	std::string prefix(level * 8, ' ');
	out << prefix << andOpSymb << " [\n";

	child1_->print(out, level + 1);
	child2_->print(out, level + 1);

	out << prefix << "]" << std::endl;
}

bool
RuleBooleanOrExpression::eval(const RuleExpressionData& expressionData) const
{
	assert(child1_.get() != 0 && child2_.get() != 0);

	return child1_->eval(expressionData) || child2_->eval(expressionData);
}

void
RuleBooleanOrExpression::print(std::ostream& out, int level) const
{
	assert(child1_.get() != 0 && child2_.get() != 0);

	std::string prefix(level * 8, ' ');
	out << prefix << orOpSymb << " [\n";

	child1_->print(out, level + 1);
	child2_->print(out, level + 1);

	out << prefix << "]" << std::endl;
}

bool
RuleBooleanXorExpression::eval(const RuleExpressionData& expressionData) const
{
	assert(child1_.get() != 0 && child2_.get() != 0);

	return child1_->eval(expressionData) != child2_->eval(expressionData);
}

void
RuleBooleanXorExpression::print(std::ostream& out, int level) const
{
	assert(child1_.get() != 0 && child2_.get() != 0);

	std::string prefix(level * 8, ' ');
	out << prefix << xorOpSymb << " [\n";

	child1_->print(out, level + 1);
	child2_->print(out, level + 1);

	out << prefix << "]" << std::endl;
}

bool
RuleBooleanNotExpression::eval(const RuleExpressionData& expressionData) const
{
	assert(child_.get() != 0);

	return !(child_->eval(expressionData));
}

void
RuleBooleanNotExpression::print(std::ostream& out, int level) const
{
	assert(child_.get() != 0);

	std::string prefix(level * 8, ' ');
	out << prefix << notOpSymb << " [\n";

	child_->print(out, level + 1);

	out << prefix << "]" << std::endl;
}

bool
RuleBooleanMarkedExpression::eval(const RuleExpressionData& expressionData) const
{
	assert(child_.get() != 0);

	return expressionData.marked && child_->eval(expressionData);
}

void
RuleBooleanMarkedExpression::print(std::ostream& out, int level) const
{
	assert(child_.get() != 0);

	std::string prefix(level * 8, ' ');
	out << prefix << markedOpSymb << " [\n";

	child_->print(out, level + 1);

	out << prefix << "]" << std::endl;
}

bool
RuleBooleanTerminal::eval(const RuleExpressionData& expressionData) const
{
	if (expressionData.posture->isMemberOfCategory(*category_)) {
		return true;
	}
	return false;
}

void
RuleBooleanTerminal::print(std::ostream& out, int level) const
{
	std::string prefix(level * 8, ' ');

	out << prefix << "category [" << category_->name();
	out << "]" << std::endl;
}

Rule::Rule(unsigned int numParameters)
	: paramProfileTransitionList_(numParameters)
	, specialProfileTransitionList_(numParameters)
	, type_(Type::invalid)
{
}

Rule::~Rule()
{
}

/*******************************************************************************
 *
 */
void
Rule::printBooleanNodeTree() const
{
	for (RuleBooleanNodeList::size_type size = booleanNodeList_.size(), i = 0; i < size; ++i) {
		std::cout << "Posture: " << (i + 1) << std::endl;
		booleanNodeList_[i]->print(std::cout);
	}
}

/*******************************************************************************
 *
 */
bool
Rule::evalBooleanExpression(const std::vector<RuleExpressionData>& expressionData) const
{
	if (booleanNodeList_.empty()) return false;
	if (expressionData.size() < booleanNodeList_.size()) return false;

	for (RuleBooleanNodeList::size_type size = booleanNodeList_.size(), i = 0; i < size; ++i) {
		if ( !(booleanNodeList_[i]->eval(expressionData[i])) ) {
			return false;
		}
	}
	return true;
}

/*******************************************************************************
 *
 */
bool
Rule::evalBooleanExpression(const RuleExpressionData& expressionData, unsigned int expressionIndex) const
{
	if (expressionIndex >= booleanNodeList_.size()) return false;

	return booleanNodeList_[expressionIndex]->eval(expressionData);
}

/*******************************************************************************
 *
 */
std::size_t
Rule::numberOfExpressions() const
{
	return booleanNodeList_.size();
}

// ruleSymbols[Rule::NUM_SYMBOLS]
void
Rule::evaluateExpressionSymbols(const std::vector<RuleExpressionData>& expressionData, Model& model, double* ruleSymbols) const
{
	assert(expressionData.size() <= 4);
	model.clearFormulaSymbolList();

	if (expressionData.size() >= 2) {
		const Posture& posture = *expressionData[0].posture;
		if (expressionData[0].marked) {
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION1, posture.getSymbolTarget(Posture::SYMB_MARKED_TRANSITION));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA1      , posture.getSymbolTarget(Posture::SYMB_MARKED_QSSA));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB1      , posture.getSymbolTarget(Posture::SYMB_MARKED_QSSB));
		} else {
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION1, posture.getSymbolTarget(Posture::SYMB_TRANSITION));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA1      , posture.getSymbolTarget(Posture::SYMB_QSSA));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB1      , posture.getSymbolTarget(Posture::SYMB_QSSB));
		}
		model.setFormulaSymbolValue(FormulaSymbol::SYMB_TEMPO1, static_cast<float>(expressionData[0].tempo));

		const Posture& posture2 = *expressionData[1].posture;
		if (expressionData[1].marked) {
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION2, posture2.getSymbolTarget(Posture::SYMB_MARKED_TRANSITION));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA2      , posture2.getSymbolTarget(Posture::SYMB_MARKED_QSSA));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB2      , posture2.getSymbolTarget(Posture::SYMB_MARKED_QSSB));
		} else {
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION2, posture2.getSymbolTarget(Posture::SYMB_TRANSITION));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA2      , posture2.getSymbolTarget(Posture::SYMB_QSSA));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB2      , posture2.getSymbolTarget(Posture::SYMB_QSSB));
		}
		model.setFormulaSymbolValue(FormulaSymbol::SYMB_TEMPO2, static_cast<float>(expressionData[1].tempo));
	}
	if (expressionData.size() >= 3) {
		const Posture& posture = *expressionData[2].posture;
		if (expressionData[2].marked) {
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION3, posture.getSymbolTarget(Posture::SYMB_MARKED_TRANSITION));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA3      , posture.getSymbolTarget(Posture::SYMB_MARKED_QSSA));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB3      , posture.getSymbolTarget(Posture::SYMB_MARKED_QSSB));
		} else {
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION3, posture.getSymbolTarget(Posture::SYMB_TRANSITION));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA3      , posture.getSymbolTarget(Posture::SYMB_QSSA));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB3      , posture.getSymbolTarget(Posture::SYMB_QSSB));
		}
		model.setFormulaSymbolValue(FormulaSymbol::SYMB_TEMPO3, static_cast<float>(expressionData[2].tempo));
	}
	if (expressionData.size() == 4) {
		const Posture& posture = *expressionData[3].posture;
		if (expressionData[3].marked) {
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION4, posture.getSymbolTarget(Posture::SYMB_MARKED_TRANSITION));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA4      , posture.getSymbolTarget(Posture::SYMB_MARKED_QSSA));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB4      , posture.getSymbolTarget(Posture::SYMB_MARKED_QSSB));
		} else {
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION4, posture.getSymbolTarget(Posture::SYMB_TRANSITION));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA4      , posture.getSymbolTarget(Posture::SYMB_QSSA));
			model.setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB4      , posture.getSymbolTarget(Posture::SYMB_QSSB));
		}
		model.setFormulaSymbolValue(FormulaSymbol::SYMB_TEMPO4, static_cast<float>(expressionData[3].tempo));
	}

	// Execute in this order.
	if (exprSymbolEquations_.duration) {
		model.setFormulaSymbolValue(FormulaSymbol::SYMB_RULE_DURATION, model.evalEquationFormula(*exprSymbolEquations_.duration));
	}
	if (exprSymbolEquations_.mark1) {
		model.setFormulaSymbolValue(FormulaSymbol::SYMB_MARK1        , model.evalEquationFormula(*exprSymbolEquations_.mark1));
	}
	if (exprSymbolEquations_.mark2) {
		model.setFormulaSymbolValue(FormulaSymbol::SYMB_MARK2        , model.evalEquationFormula(*exprSymbolEquations_.mark2));
	}
	if (exprSymbolEquations_.mark3) {
		model.setFormulaSymbolValue(FormulaSymbol::SYMB_MARK3        , model.evalEquationFormula(*exprSymbolEquations_.mark3));
	}
	if (exprSymbolEquations_.beat) {
		model.setFormulaSymbolValue(FormulaSymbol::SYMB_BEAT         , model.evalEquationFormula(*exprSymbolEquations_.beat));
	}

	ruleSymbols[Rule::SYMB_DURATION] = model.getFormulaSymbolValue(FormulaSymbol::SYMB_RULE_DURATION);
	ruleSymbols[Rule::SYMB_BEAT    ] = model.getFormulaSymbolValue(FormulaSymbol::SYMB_BEAT);
	ruleSymbols[Rule::SYMB_MARK1   ] = model.getFormulaSymbolValue(FormulaSymbol::SYMB_MARK1);
	ruleSymbols[Rule::SYMB_MARK2   ] = model.getFormulaSymbolValue(FormulaSymbol::SYMB_MARK2);
	ruleSymbols[Rule::SYMB_MARK3   ] = model.getFormulaSymbolValue(FormulaSymbol::SYMB_MARK3);
}

void
Rule::setBooleanExpressionList(const std::vector<std::string>& exprList, const Model& model)
{
	const unsigned int size = exprList.size();
	Type ruleType;
	switch (size) {
	case 2:
		ruleType = Type::diphone;
		break;
	case 3:
		ruleType = Type::triphone;
		break;
	case 4:
		ruleType = Type::tetraphone;
		break;
	default:
		THROW_EXCEPTION(InvalidParameterException, "Invalid number of boolean expressions: " << size << '.');
	}

	RuleBooleanNodeList testBooleanNodeList;

	for (unsigned int i = 0; i < size; ++i) {
		Parser p(exprList[i], model);
		testBooleanNodeList.push_back(p.parse());
	}

	booleanExpressionList_ = exprList;
	booleanNodeList_ = std::move(testBooleanNodeList);
	type_ = ruleType;
}

// This validation is incomplete.
void
Rule::validate(const Model& model) const
{
	// Parse the boolean expressions.
	const unsigned int numExpr = booleanExpressionList_.size();
	// booleanExpressionList_.size() and type_ are validated in setBooleanExpressionList.
	for (unsigned int i = 0; i < numExpr; ++i) {
		Parser p{booleanExpressionList_[i], model};
		p.parse();
	}

	// Check the types of the transitions.
	unsigned int paramNumber = 1;
	for (const auto& trans : paramProfileTransitionList_) {
		if (!trans) {
			THROW_EXCEPTION(ValidationException, "Transition not set for parameter " << paramNumber << '.');
		}
		if (static_cast<int>(trans->type()) != static_cast<int>(type_)) {
			THROW_EXCEPTION(ValidationException, "Wrong type of transition (" << static_cast<int>(trans->type())
						<< ") for parameter " << paramNumber << '.');
		}
		++paramNumber;
	}

	// Check the types of the special transitions.
	paramNumber = 1;
	for (const auto& trans : specialProfileTransitionList_) {
		if (trans && static_cast<int>(trans->type()) != static_cast<int>(type_)) {
			THROW_EXCEPTION(ValidationException, "Wrong type of special transition (" << static_cast<int>(trans->type())
						<< ") for parameter " << paramNumber << '.');
		}
		++paramNumber;
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */
