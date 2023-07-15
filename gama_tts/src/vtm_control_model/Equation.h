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

#ifndef VTM_CONTROL_MODEL_EQUATION_H_
#define VTM_CONTROL_MODEL_EQUATION_H_

#include <iostream>
#include <string>
#include <memory>
#include <utility> /* swap, move */
#include <vector>

#include "FormulaSymbol.h"



namespace GS {
namespace VTMControlModel {

class FormulaNode {
public:
	FormulaNode() = default;
	virtual ~FormulaNode() = default;

	virtual float eval(const FormulaSymbolList& symbolList) const = 0;
	virtual void print(std::ostream& out, int level = 0) const = 0;
private:
	FormulaNode(const FormulaNode&) = delete;
	FormulaNode& operator=(const FormulaNode&) = delete;
	FormulaNode(FormulaNode&&) = delete;
	FormulaNode& operator=(FormulaNode&&) = delete;
};

typedef std::unique_ptr<FormulaNode> FormulaNode_ptr;

class FormulaMinusUnaryOp : public FormulaNode {
public:
	explicit FormulaMinusUnaryOp(FormulaNode_ptr c)
			: FormulaNode(), child_(std::move(c)) {}
	virtual ~FormulaMinusUnaryOp() = default;

	virtual float eval(const FormulaSymbolList& symbolList) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	FormulaMinusUnaryOp(const FormulaMinusUnaryOp&) = delete;
	FormulaMinusUnaryOp& operator=(const FormulaMinusUnaryOp&) = delete;
	FormulaMinusUnaryOp(FormulaMinusUnaryOp&&) = delete;
	FormulaMinusUnaryOp& operator=(FormulaMinusUnaryOp&&) = delete;

	FormulaNode_ptr child_;
};

class FormulaAddBinaryOp : public FormulaNode {
public:
	FormulaAddBinaryOp(FormulaNode_ptr c1, FormulaNode_ptr c2)
			: FormulaNode(), child1_(std::move(c1)), child2_(std::move(c2)) {}
	virtual ~FormulaAddBinaryOp() = default;

	virtual float eval(const FormulaSymbolList& symbolList) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	FormulaAddBinaryOp(const FormulaAddBinaryOp&) = delete;
	FormulaAddBinaryOp& operator=(const FormulaAddBinaryOp&) = delete;
	FormulaAddBinaryOp(FormulaAddBinaryOp&&) = delete;
	FormulaAddBinaryOp& operator=(FormulaAddBinaryOp&&) = delete;

	FormulaNode_ptr child1_;
	FormulaNode_ptr child2_;
};

class FormulaSubBinaryOp : public FormulaNode {
public:
	FormulaSubBinaryOp(FormulaNode_ptr c1, FormulaNode_ptr c2)
			: FormulaNode(), child1_(std::move(c1)), child2_(std::move(c2)) {}
	virtual ~FormulaSubBinaryOp() = default;

	virtual float eval(const FormulaSymbolList& symbolList) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	FormulaSubBinaryOp(const FormulaSubBinaryOp&) = delete;
	FormulaSubBinaryOp& operator=(const FormulaSubBinaryOp&) = delete;
	FormulaSubBinaryOp(FormulaSubBinaryOp&&) = delete;
	FormulaSubBinaryOp& operator=(FormulaSubBinaryOp&&) = delete;

	FormulaNode_ptr child1_;
	FormulaNode_ptr child2_;
};

class FormulaMultBinaryOp : public FormulaNode {
public:
	FormulaMultBinaryOp(FormulaNode_ptr c1, FormulaNode_ptr c2)
			: FormulaNode(), child1_(std::move(c1)), child2_(std::move(c2)) {}
	virtual ~FormulaMultBinaryOp() = default;

	virtual float eval(const FormulaSymbolList& symbolList) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	FormulaMultBinaryOp(const FormulaMultBinaryOp&) = delete;
	FormulaMultBinaryOp& operator=(const FormulaMultBinaryOp&) = delete;
	FormulaMultBinaryOp(FormulaMultBinaryOp&&) = delete;
	FormulaMultBinaryOp& operator=(FormulaMultBinaryOp&&) = delete;

	FormulaNode_ptr child1_;
	FormulaNode_ptr child2_;
};

class FormulaDivBinaryOp : public FormulaNode {
public:
	FormulaDivBinaryOp(FormulaNode_ptr c1, FormulaNode_ptr c2)
			: FormulaNode(), child1_(std::move(c1)), child2_(std::move(c2)) {}
	virtual ~FormulaDivBinaryOp() = default;

	virtual float eval(const FormulaSymbolList& symbolList) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	FormulaDivBinaryOp(const FormulaDivBinaryOp&) = delete;
	FormulaDivBinaryOp& operator=(const FormulaDivBinaryOp&) = delete;
	FormulaDivBinaryOp(FormulaDivBinaryOp&&) = delete;
	FormulaDivBinaryOp& operator=(FormulaDivBinaryOp&&) = delete;

	FormulaNode_ptr child1_;
	FormulaNode_ptr child2_;
};

class FormulaConst : public FormulaNode {
public:
	explicit FormulaConst(float value)
			: FormulaNode(), value_(value) {}
	virtual ~FormulaConst() = default;

	virtual float eval(const FormulaSymbolList& symbolList) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	FormulaConst(const FormulaConst&) = delete;
	FormulaConst& operator=(const FormulaConst&) = delete;
	FormulaConst(FormulaConst&&) = delete;
	FormulaConst& operator=(FormulaConst&&) = delete;

	float value_;
};

class FormulaSymbolValue : public FormulaNode {
public:
	explicit FormulaSymbolValue(FormulaSymbol::Code symbol)
			: FormulaNode(), symbol_(symbol) {}
	virtual ~FormulaSymbolValue() = default;

	virtual float eval(const FormulaSymbolList& symbolList) const;
	virtual void print(std::ostream& out, int level = 0) const;
private:
	FormulaSymbolValue(const FormulaSymbolValue&) = delete;
	FormulaSymbolValue& operator=(const FormulaSymbolValue&) = delete;
	FormulaSymbolValue(FormulaSymbolValue&&) = delete;
	FormulaSymbolValue& operator=(FormulaSymbolValue&&) = delete;

	FormulaSymbol::Code symbol_;
};

class Equation;
std::ostream& operator<<(std::ostream& out, const Equation& equation);

class Equation {
public:
	explicit Equation(const std::string& name) : name_(name) {}

	void setName(const std::string& name) { name_ = name; }
	const std::string& name() const { return name_; }

	const std::string& formula() const { return formula_; }
	void setFormula(const std::string& formula);

	void setComment(const std::string& comment) { comment_ = comment; }
	const std::string& comment() const { return comment_; }

	float evalFormula(const FormulaSymbolList& symbolList) const;

	friend std::ostream& operator<<(std::ostream& out, const Equation& equation);
private:
	std::string name_;
	std::string formula_;
	std::string comment_;
	FormulaNode_ptr formulaRoot_;
};

struct EquationGroup {
	std::string name;
	std::vector<std::shared_ptr<Equation>> equationList;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_EQUATION_H_ */
