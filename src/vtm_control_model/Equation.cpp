/***************************************************************************
 *  Copyright 2014 Marcelo Y. Matuda                                       *
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

#include "Equation.h"

#include <cctype> /* isspace */

#include "Exception.h"
#include "Log.h"
#include "Text.h"



namespace {

using namespace GS::VTMControlModel;

constexpr char        addChar = '+';
constexpr char        subChar = '-';
constexpr char       multChar = '*';
constexpr char        divChar = '/';
constexpr char rightParenChar = ')';
constexpr char  leftParenChar = '(';



class FormulaNodeParser {
public:
	FormulaNodeParser(const std::string& s)
				: s_(GS::Text::trim(s))
				, pos_(0)
				, symbolType_(SymbolType::invalid) {
		if (s_.empty()) {
			THROW_EXCEPTION(GS::VTMControlModelException, "Formula expression parser error: Empty string.");
		}
		nextSymbol();
	}

	FormulaNode_ptr parse();
private:
	enum class SymbolType {
		invalid,
		add,
		sub,
		mult,
		div,
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
	FormulaNode_ptr parseFactor();
	FormulaNode_ptr parseTerm();
	FormulaNode_ptr parseExpression();
	void skipSpaces();

	static bool isSeparator(char c) {
		switch (c) {
		case rightParenChar: return true;
		case leftParenChar:  return true;
		default:             return std::isspace(c) != 0;
		}
	}

	const FormulaSymbol formulaSymbol_;
	const std::string s_;
	std::string::size_type pos_;
	std::string symbol_;
	SymbolType symbolType_;
};

void
FormulaNodeParser::throwException(const char* errorDescription) const
{
	THROW_EXCEPTION(GS::VTMControlModelException, "Formula expression parser error: "
				<< errorDescription
				<< " at position " << (pos_ - symbol_.size()) << " of string [" << s_ << "].");
}

template<typename T>
void
FormulaNodeParser::throwException(const char* errorDescription, const T& complement) const
{
	THROW_EXCEPTION(GS::VTMControlModelException, "Formula expression parser error: "
				<< errorDescription << complement
				<< " at position " << (pos_ - symbol_.size()) << " of string [" << s_ << "].");
}

void
FormulaNodeParser::skipSpaces()
{
	while (!finished() && std::isspace(s_[pos_])) ++pos_;
}

void
FormulaNodeParser::nextSymbol()
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
	case addChar:
		symbolType_ = SymbolType::add;
		break;
	case subChar:
		symbolType_ = SymbolType::sub;
		break;
	case multChar:
		symbolType_ = SymbolType::mult;
		break;
	case divChar:
		symbolType_ = SymbolType::div;
		break;
	case rightParenChar:
		symbolType_ = SymbolType::rightParen;
		break;
	case leftParenChar:
		symbolType_ = SymbolType::leftParen;
		break;
	default:
		symbolType_ = SymbolType::string;
		while ( !finished() && !isSeparator(c = s_[pos_]) ) {
			symbol_ += c;
			++pos_;
		}
	}
}

/*******************************************************************************
 * FACTOR -> "(" EXPRESSION ")" | SYMBOL | CONST | ADD_OP FACTOR
 */
FormulaNode_ptr
FormulaNodeParser::parseFactor()
{
	switch (symbolType_) {
	case SymbolType::leftParen: // (expression)
	{
		nextSymbol();
		FormulaNode_ptr res(parseExpression());
		if (symbolType_ != SymbolType::rightParen) {
			throwException("Right parenthesis not found");
		}
		nextSymbol();
		return res;
	}
	case SymbolType::add: // unary plus
		nextSymbol();
		return parseFactor();
	case SymbolType::sub: // unary minus
		nextSymbol();
		return std::make_unique<FormulaMinusUnaryOp>(parseFactor());
	case SymbolType::string: // const / symbol
	{
		std::string symbolTmp = symbol_;
		nextSymbol();
		auto iter = formulaSymbol_.codeMap.find(symbolTmp);
		if (iter == formulaSymbol_.codeMap.end()) {
			// It's not a symbol.
			return std::make_unique<FormulaConst>(GS::Text::parseString<float>(symbolTmp));
		} else {
			return std::make_unique<FormulaSymbolValue>(iter->second);
		}
	}
	case SymbolType::rightParen:
		throwException("Unexpected symbol: ", rightParenChar);
	case SymbolType::mult:
		throwException("Unexpected symbol: ", multChar);
	case SymbolType::div:
		throwException("Unexpected symbol: ", divChar);
	default:
		throwException("Invalid symbol");
	}
	return FormulaNode_ptr(); // unreachable
}

/*******************************************************************************
 * TERM -> FACTOR { MULT_OP FACTOR }
 */
FormulaNode_ptr FormulaNodeParser::parseTerm()
{
	FormulaNode_ptr term1 = parseFactor();

	SymbolType type = symbolType_;
	while (type == SymbolType::mult || type == SymbolType::div) {
		nextSymbol();
		FormulaNode_ptr term2 = parseFactor();
		FormulaNode_ptr expr;
		if (type == SymbolType::mult) {
			expr = std::make_unique<FormulaMultBinaryOp>(std::move(term1), std::move(term2));
		} else {
			expr = std::make_unique<FormulaDivBinaryOp>(std::move(term1), std::move(term2));
		}
		term1.swap(expr);
		type = symbolType_;
	}

	return term1;
}

/*******************************************************************************
 * EXPRESSION -> TERM { ADD_OP TERM }
 */
FormulaNode_ptr
FormulaNodeParser::parseExpression()
{
	FormulaNode_ptr term1 = parseTerm();

	SymbolType type = symbolType_;
	while (type == SymbolType::add || type == SymbolType::sub) {

		nextSymbol();
		FormulaNode_ptr term2 = parseTerm();

		FormulaNode_ptr expr;
		if (type == SymbolType::add) {
			expr = std::make_unique<FormulaAddBinaryOp>(std::move(term1), std::move(term2));
		} else {
			expr = std::make_unique<FormulaSubBinaryOp>(std::move(term1), std::move(term2));
		}

		term1 = std::move(expr);
		type = symbolType_;
	}

	return term1;
}

/*******************************************************************************
 *
 */
FormulaNode_ptr
FormulaNodeParser::parse()
{
	FormulaNode_ptr formulaRoot = parseExpression();
	if (symbolType_ != SymbolType::invalid) { // there is a symbol available
		throwException("Invalid text");
	}
	return formulaRoot;
}

} /* namespace */

//==============================================================================

namespace GS {
namespace VTMControlModel {

float
FormulaMinusUnaryOp::eval(const FormulaSymbolList& symbolList) const
{
	return -(child_->eval(symbolList));
}

void
FormulaMinusUnaryOp::print(std::ostream& out, int level) const
{
	std::string prefix(level * 8, ' ');
	out << prefix << "- [\n";

	child_->print(out, level + 1);

	out << prefix << "]" << std::endl;
}

float
FormulaAddBinaryOp::eval(const FormulaSymbolList& symbolList) const
{
	return child1_->eval(symbolList) + child2_->eval(symbolList);
}

void
FormulaAddBinaryOp::print(std::ostream& out, int level) const
{
	std::string prefix(level * 8, ' ');
	out << prefix << "+ [\n";

	child1_->print(out, level + 1);
	child2_->print(out, level + 1);

	out << prefix << "]" << std::endl;
}

float
FormulaSubBinaryOp::eval(const FormulaSymbolList& symbolList) const
{
	return child1_->eval(symbolList) - child2_->eval(symbolList);
}

void
FormulaSubBinaryOp::print(std::ostream& out, int level) const
{
	std::string prefix(level * 8, ' ');
	out << prefix << "- [\n";

	child1_->print(out, level + 1);
	child2_->print(out, level + 1);

	out << prefix << "]" << std::endl;
}

float
FormulaMultBinaryOp::eval(const FormulaSymbolList& symbolList) const
{
	return child1_->eval(symbolList) * child2_->eval(symbolList);
}

void
FormulaMultBinaryOp::print(std::ostream& out, int level) const
{
	std::string prefix(level * 8, ' ');
	out << prefix << "* [\n";

	child1_->print(out, level + 1);
	child2_->print(out, level + 1);

	out << prefix << "]" << std::endl;
}

float
FormulaDivBinaryOp::eval(const FormulaSymbolList& symbolList) const
{
	return child1_->eval(symbolList) / child2_->eval(symbolList);
}

void
FormulaDivBinaryOp::print(std::ostream& out, int level) const
{
	std::string prefix(level * 8, ' ');
	out << prefix << "/ [\n";

	child1_->print(out, level + 1);
	child2_->print(out, level + 1);

	out << prefix << "]" << std::endl;
}

float
FormulaConst::eval(const FormulaSymbolList& /*symbolList*/) const
{
	return value_;
}

void
FormulaConst::print(std::ostream& out, int level) const
{
	out << std::string(level * 8, ' ') << "const=" << value_ << std::endl;
}

float
FormulaSymbolValue::eval(const FormulaSymbolList& symbolList) const
{
	return symbolList[symbol_];
}

void
FormulaSymbolValue::print(std::ostream& out, int level) const
{
	out << std::string(level * 8, ' ') << "symbol=" << symbol_ << std::endl;
}

/*******************************************************************************
 *
 */
void
Equation::setFormula(const std::string& formula)
{
	FormulaNodeParser p(formula);
	FormulaNode_ptr tempFormulaRoot = p.parse();

	formula_ = formula;
	std::swap(tempFormulaRoot, formulaRoot_);
}

/*******************************************************************************
 *
 */
float
Equation::evalFormula(const FormulaSymbolList& symbolList) const
{
	if (!formulaRoot_) {
		THROW_EXCEPTION(InvalidStateException, "Empty formula.");
	}

	return formulaRoot_->eval(symbolList);
}

/*******************************************************************************
 *
 */
std::ostream&
operator<<(std::ostream& out, const Equation& equation)
{
	if (equation.formulaRoot_) {
		equation.formulaRoot_->print(out);
	}
	return out;
}

} /* namespace VTMControlModel */
} /* namespace GS */
