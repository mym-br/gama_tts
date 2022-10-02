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

#include "Model.h"

#include <algorithm> /* sort */
#include <iostream>
#include <utility> /* make_pair */

#include "Index.h"
#include "Log.h"
#include "Text.h"
#include "XMLConfigFileReader.h"
#include "XMLConfigFileWriter.h"



namespace GS {
namespace VTMControlModel {

/*******************************************************************************
 *
 */
void
Model::clear()
{
	categoryList_.clear();
	parameterList_.clear();
	postureList_.clear();
	ruleList_.clear();
	equationGroupList_.clear();
	transitionGroupList_.clear();
	specialTransitionGroupList_.clear();
	formulaSymbolList_.fill(0.0f);
}

/*******************************************************************************
 *
 */
void
Model::load(const Index& index)
{
	clear();

	try {
		std::string filePath = index.entry("artic_file");

		// Load the configuration file.
		LOG_DEBUG("Loading xml configuration: " << filePath);
		XMLConfigFileReader cfg(*this, filePath);
		cfg.loadModel();

		if (Log::debugEnabled) {
			printInfo();
		}
	} catch (...) {
		clear();
		throw;
	}
}

/*******************************************************************************
 *
 */
void
Model::load(const char* configDirPath, const char* configFileName)
{
	clear();

	try {
		std::string filePath = std::string(configDirPath) + configFileName;

		// Load the configuration file.
		LOG_DEBUG("Loading xml configuration: " << filePath);
		XMLConfigFileReader cfg(*this, filePath);
		cfg.loadModel();

		if (Log::debugEnabled) {
			printInfo();
		}
	} catch (...) {
		clear();
		throw;
	}
}

/*******************************************************************************
 *
 */
void
Model::save(const char* configDirPath, const char* configFileName)
{
	std::string filePath = std::string(configDirPath) + configFileName;

	// Save the configuration file.
	LOG_DEBUG("Saving xml configuration: " << filePath);
	XMLConfigFileWriter cfg(*this, filePath);
	cfg.saveModel();
}

/*******************************************************************************
 *
 */
void
Model::printInfo() const
{
	//---------------------------------------------------------
	// Categories.
	std::cout << std::string(40, '-') << "\nCategories:\n" << std::endl;
	for (const auto& category : categoryList_) {
		std::cout << "category: " << category->name() << std::endl;
	}

	//---------------------------------------------------------
	// Postures.
	std::cout << std::string(40, '-') << "\nPostures:\n" << std::endl;
	for (unsigned int i = 0, size = postureList_.size(); i < size; ++i) {
		const auto& posture = postureList_[i];
		std::cout << "posture symbol: " << posture.name() << std::endl;
		for (const auto& category : posture.categoryList()) {
			std::cout << "  categ: " << category->name() << std::endl;
		}
		std::cout << "  parameter targets:" << std::endl;
		for (unsigned int j = 0, size = parameterList_.size(); j < size; ++j) {
			const Parameter& param = parameterList_[j];
			std::cout << "    " << param.name() << ": " << posture.getParameterTarget(j) << std::endl;
		}
		std::cout << "  symbol targets:" << std::endl;
		for (unsigned int j = 0, size = symbolList_.size(); j < size; ++j) {
			const Symbol& symbol = symbolList_[j];
			std::cout << "    " << symbol.name() << ": " << posture.getSymbolTarget(j) << std::endl;
		}
	}

	//---------------------------------------------------------
	// Equations.
	std::cout << std::string(40, '-') << "\nEquations:\n" << std::endl;
	FormulaSymbolList symbolList;
	symbolList[FormulaSymbol::SYMB_TRANSITION1] = 100.1f;
	symbolList[FormulaSymbol::SYMB_TRANSITION2] = 100.2f;
	symbolList[FormulaSymbol::SYMB_TRANSITION3] = 100.3f;
	symbolList[FormulaSymbol::SYMB_TRANSITION4] = 100.4f;
	symbolList[FormulaSymbol::SYMB_QSSA1] = 20.5f;
	symbolList[FormulaSymbol::SYMB_QSSA2] = 20.6f;
	symbolList[FormulaSymbol::SYMB_QSSA3] = 20.7f;
	symbolList[FormulaSymbol::SYMB_QSSA4] = 20.8f;
	symbolList[FormulaSymbol::SYMB_QSSB1] = 20.9f;
	symbolList[FormulaSymbol::SYMB_QSSB2] = 20.0f;
	symbolList[FormulaSymbol::SYMB_QSSB3] = 20.1f;
	symbolList[FormulaSymbol::SYMB_QSSB4] = 20.2f;
	symbolList[FormulaSymbol::SYMB_TEMPO1] = 1.1f;
	symbolList[FormulaSymbol::SYMB_TEMPO2] = 1.2f;
	symbolList[FormulaSymbol::SYMB_TEMPO3] = 1.3f;
	symbolList[FormulaSymbol::SYMB_TEMPO4] = 1.4f;
	symbolList[FormulaSymbol::SYMB_RULE_DURATION] = 150.4f;
	symbolList[FormulaSymbol::SYMB_MARK1] = 150.5f;
	symbolList[FormulaSymbol::SYMB_MARK2] = 150.6f;
	//symbolList[FormulaSymbol::SYMB_NULL] = 1.0f;
	for (const auto& group : equationGroupList_) {
		std::cout << "=== Equation group: " << group.name << std::endl;
		for (const auto& equation : group.equationList) {
			std::cout << "=== Equation: [" << equation->name() << "]" << std::endl;
			std::cout << "    [" << equation->formula() << "]" << std::endl;
			std::cout << *equation << std::endl;
			std::cout << "*** EVAL=" << equation->evalFormula(symbolList) << std::endl;
		}
	}

	//---------------------------------------------------------
	// Transitions.
	std::cout << std::string(40, '-') << "\nTransitions:" << std::endl << std::endl;
	for (const auto& group : transitionGroupList_) {
		std::cout << "=== Transition group: " << group.name << std::endl;
		for (const auto& transition : group.transitionList) {
			std::cout << "### Transition: [" << transition->name() << "]" << std::endl;
			std::cout << "    type=" << Transition::getNameFromType(transition->type()) << " special=" << transition->special() << std::endl;

			for (auto& pointOrSlope : transition->pointOrSlopeList()) {
				if (!pointOrSlope->isSlopeRatio()) {
					const auto& point = dynamic_cast<const Transition::Point&>(*pointOrSlope);
					std::cout << "       point: type=" << Transition::Point::getNameFromType(point.type) << " value=" << point.value
						<< " freeTime=" << point.freeTime
						<< " timeExpression=" << (point.timeExpression ? point.timeExpression->name() : "")
						<< std::endl;
				} else {
					const auto& slopeRatio = dynamic_cast<const Transition::SlopeRatio&>(*pointOrSlope);
					for (auto& point : slopeRatio.pointList) {
						std::cout << "         point: type=" << Transition::Point::getNameFromType(point->type) << " value=" << point->value
							<< " freeTime=" << point->freeTime
							<< " timeExpression=" << (point->timeExpression ? point->timeExpression->name() : "")
							<< std::endl;
					}
					for (auto& slope : slopeRatio.slopeList) {
						std::cout << "         slope: slope=" << slope->slope << std::endl;
					}
				}
			}
		}
	}

	//---------------------------------------------------------
	// Special transitions.
	std::cout << std::string(40, '-') << "\nSpecial transitions:" << std::endl << std::endl;
	for (const auto& group : specialTransitionGroupList_) {
		std::cout << "=== Special transition group: " << group.name << std::endl;
		for (const auto& transition : group.transitionList) {
			std::cout << "### Transition: [" << transition->name() << "]" << std::endl;
			std::cout << "    type=" << Transition::getNameFromType(transition->type()) << " special=" << transition->special() << std::endl;

			for (const auto& pointOrSlope : transition->pointOrSlopeList()) {
				const auto& point = dynamic_cast<const Transition::Point&>(*pointOrSlope);

				std::cout << "       point: type=" << Transition::Point::getNameFromType(point.type) << " value=" << point.value
					<< " freeTime=" << point.freeTime
					<< " timeExpression=" << (point.timeExpression ? point.timeExpression->name() : "")
					<< std::endl;
			}
		}
	}

	//---------------------------------------------------------
	// Rules.

	// Print boolean expression tree for each Rule.
	std::cout << std::string(40, '-') << "\nRules:\n" << std::endl;
	unsigned int ruleNumber = 0;
	for (auto& r : ruleList_) {
		std::cout << "--------------------------------------\n";
		std::cout << "Rule number: " << ++ruleNumber << '\n';
		std::cout << "Number of boolean expressions = " << r->numberOfExpressions() << std::endl;
		r->printBooleanNodeTree();
	}
}

/*******************************************************************************
 *
 */
unsigned int
Model::findParameterIndex(const std::string& name) const
{
	for (unsigned int i = 0; i < parameterList_.size(); ++i) {
		if (parameterList_[i].name() == name) {
			return i;
		}
	}
	THROW_EXCEPTION(InvalidParameterException, "Parameter name not found: " << name << '.');
}

/*******************************************************************************
 * Find the Category.
 *
 * Returns an empty shared_ptr if the Category was not found.
 */
const std::shared_ptr<Category>
Model::findCategory(const std::string& name) const
{
	for (auto& category : categoryList_) {
		if (category->name() == name) {
			return category;
		}
	}
	return std::shared_ptr<Category>();
}

/*******************************************************************************
 * Find the Category.
 *
 * Returns an empty shared_ptr if the Category was not found.
 */
std::shared_ptr<Category>
Model::findCategory(const std::string& name)
{
	for (auto& category : categoryList_) {
		if (category->name() == name) {
			return category;
		}
	}
	return std::shared_ptr<Category>();
}

/*******************************************************************************
 *
 */
bool
Model::findCategoryName(const std::string& name) const
{
	for (const auto& item : categoryList_) {
		if (item->name() == name) {
			return true;
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
void
Model::clearFormulaSymbolList()
{
	formulaSymbolList_.fill(0.0);
}

/*******************************************************************************
 *
 */
void
Model::setFormulaSymbolValue(FormulaSymbol::Code symbol, float value)
{
	formulaSymbolList_[symbol] = value;
}

/*******************************************************************************
 *
 */
float
Model::getFormulaSymbolValue(FormulaSymbol::Code symbol) const
{
	return formulaSymbolList_[symbol];
}

/*******************************************************************************
 *
 */
void
Model::setDefaultFormulaSymbols(Transition::Type transitionType)
{
	setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION1, 33.3333f);
	setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION2, 33.3333f);
	setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION3, 33.3333f);
	setFormulaSymbolValue(FormulaSymbol::SYMB_TRANSITION4, 33.3333f);

	setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA1      , 33.3333f);
	setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA2      , 33.3333f);
	setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA3      , 33.3333f);
	setFormulaSymbolValue(FormulaSymbol::SYMB_QSSA4      , 33.3333f);

	setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB1      , 33.3333f);
	setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB2      , 33.3333f);
	setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB3      , 33.3333f);
	setFormulaSymbolValue(FormulaSymbol::SYMB_QSSB4      , 33.3333f);

	setFormulaSymbolValue(FormulaSymbol::SYMB_TEMPO1, 1.0);
	setFormulaSymbolValue(FormulaSymbol::SYMB_TEMPO2, 1.0);
	setFormulaSymbolValue(FormulaSymbol::SYMB_TEMPO3, 1.0);
	setFormulaSymbolValue(FormulaSymbol::SYMB_TEMPO4, 1.0);

	setFormulaSymbolValue(FormulaSymbol::SYMB_BEAT ,  33.0);
	setFormulaSymbolValue(FormulaSymbol::SYMB_MARK1, 100.0);
	switch (transitionType) {
	case Transition::Type::diphone:
		setFormulaSymbolValue(FormulaSymbol::SYMB_RULE_DURATION, 100.0);
		setFormulaSymbolValue(FormulaSymbol::SYMB_MARK2        ,   0.0);
		setFormulaSymbolValue(FormulaSymbol::SYMB_MARK3        ,   0.0);
		break;
	case Transition::Type::triphone:
		setFormulaSymbolValue(FormulaSymbol::SYMB_RULE_DURATION, 200.0);
		setFormulaSymbolValue(FormulaSymbol::SYMB_MARK2        , 200.0);
		setFormulaSymbolValue(FormulaSymbol::SYMB_MARK3        ,   0.0);
		break;
	case Transition::Type::tetraphone:
		setFormulaSymbolValue(FormulaSymbol::SYMB_RULE_DURATION, 300.0);
		setFormulaSymbolValue(FormulaSymbol::SYMB_MARK2        , 200.0);
		setFormulaSymbolValue(FormulaSymbol::SYMB_MARK3        , 300.0);
		break;
	default:
		THROW_EXCEPTION(VTMControlModelException, "Invalid transition type: " << static_cast<int>(transitionType) << '.');
	}
}

/*******************************************************************************
 *
 */
float
Model::evalEquationFormula(const Equation& equation) const
{
	return equation.evalFormula(formulaSymbolList_);
}

/*******************************************************************************
 *
 */
bool
Model::findEquationGroupName(const std::string& name) const
{
	for (const auto& item : equationGroupList_) {
		if (item.name == name) {
			return true;
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
bool
Model::findEquationName(const std::string& name) const
{
	for (const auto& group : equationGroupList_) {
		for (const auto& item : group.equationList) {
			if (item->name() == name) {
				return true;
			}
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
bool
Model::findEquationIndex(const std::string& name, unsigned int& groupIndex, unsigned int& index) const
{
	for (unsigned int i = 0, groupListSize = equationGroupList_.size(); i < groupListSize; ++i) {
		const auto& group = equationGroupList_[i];
		for (unsigned int j = 0, size = group.equationList.size(); j < size; ++j) {
			if (group.equationList[j]->name() == name) {
				groupIndex = i;
				index = j;
				return true;
			}
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
std::shared_ptr<Equation>
Model::findEquation(const std::string& name)
{
	for (const auto& group : equationGroupList_) {
		for (const auto& equation : group.equationList) {
			if (equation->name() == name) {
				return equation;
			}
		}
	}
	return std::shared_ptr<Equation>();
}

/*******************************************************************************
 *
 */
float
Model::getParameterMinimum(unsigned int parameterIndex) const
{
	if (parameterIndex >= parameterList_.size()) {
		THROW_EXCEPTION(InvalidParameterException, "Invalid parameter index: " << parameterIndex << '.');
	}

	return parameterList_[parameterIndex].minimum();
}

/*******************************************************************************
 *
 */
float
Model::getParameterMaximum(unsigned int parameterIndex) const
{
	if (parameterIndex >= parameterList_.size()) {
		THROW_EXCEPTION(InvalidParameterException, "Invalid parameter index: " << parameterIndex << '.');
	}

	return parameterList_[parameterIndex].maximum();
}

/*******************************************************************************
 *
 */
const Parameter&
Model::getParameter(unsigned int parameterIndex) const
{
	if (parameterIndex >= parameterList_.size()) {
		THROW_EXCEPTION(InvalidParameterException, "Invalid parameter index: " << parameterIndex << '.');
	}

	return parameterList_[parameterIndex];
}

/*******************************************************************************
 *
 */
bool
Model::findParameterName(const std::string& name) const
{
	for (const auto& item : parameterList_) {
		if (item.name() == name) {
			return true;
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
bool
Model::findSymbolName(const std::string& name) const
{
	for (const auto& item : symbolList_) {
		if (item.name() == name) {
			return true;
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
bool
Model::isValidPostureCharacter(char c)
{
	if (Text::isNonprintableAscii(c)) return false;
	if (c >= '0' && c <= '9') return false; // digits are used to define tempo
	switch (c) {
	case ' ':
	case '\'': // marked posture
	case '_':  // posture separator, new foot
	case '*':  // new marked foot
	case '.':  // new syllable, decimal point
	case '/':  // start of control code
		// Characters used in the phonetic string (Gnuspeech) and to indicate a marked posture.
		return false;
	default:
		return true;
	}
}

/*******************************************************************************
 * Find a Transition with the given name.
 *
 * Returns an empty shared_ptr if the Transition was not found.
 */
const std::shared_ptr<Transition>
Model::findTransition(const std::string& name) const
{
	for (const auto& group : transitionGroupList_) {
		for (const auto& transition : group.transitionList) {
			if (transition->name() == name) {
				return transition;
			}
		}
	}
	return std::shared_ptr<Transition>();
}

/*******************************************************************************
 *
 */
bool
Model::findTransitionGroupName(const std::string& name) const
{
	for (const auto& item : transitionGroupList_) {
		if (item.name == name) {
			return true;
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
bool
Model::findTransitionName(const std::string& name) const
{
	for (const auto& group : transitionGroupList_) {
		for (const auto& item : group.transitionList) {
			if (item->name() == name) {
				return true;
			}
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
bool
Model::findTransitionIndex(const std::string& name, unsigned int& groupIndex, unsigned int& index) const
{
	for (unsigned int i = 0, groupListSize = transitionGroupList_.size(); i < groupListSize; ++i) {
		const auto& group = transitionGroupList_[i];
		for (unsigned int j = 0, size = group.transitionList.size(); j < size; ++j) {
			if (group.transitionList[j]->name() == name) {
				groupIndex = i;
				index = j;
				return true;
			}
		}
	}
	return false;
}

/*******************************************************************************
 * Find a Special Transition with the given name.
 *
 * Returns an empty shared_ptr if the Transition was not found.
 */
const std::shared_ptr<Transition>
Model::findSpecialTransition(const std::string& name) const
{
	for (const auto& group : specialTransitionGroupList_) {
		for (const auto& transition : group.transitionList) {
			if (transition->name() == name) {
				return transition;
			}
		}
	}
	return std::shared_ptr<Transition>();
}

/*******************************************************************************
 *
 */
bool
Model::findSpecialTransitionGroupName(const std::string& name) const
{
	for (const auto& item : specialTransitionGroupList_) {
		if (item.name == name) {
			return true;
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
bool
Model::findSpecialTransitionName(const std::string& name) const
{
	for (const auto& group : specialTransitionGroupList_) {
		for (const auto& item : group.transitionList) {
			if (item->name() == name) {
				return true;
			}
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
bool
Model::findSpecialTransitionIndex(const std::string& name, unsigned int& groupIndex, unsigned int& index) const
{
	for (unsigned int i = 0, groupListSize = specialTransitionGroupList_.size(); i < groupListSize; ++i) {
		const auto& group = specialTransitionGroupList_[i];
		for (unsigned int j = 0, size = group.transitionList.size(); j < size; ++j) {
			if (group.transitionList[j]->name() == name) {
				groupIndex = i;
				index = j;
				return true;
			}
		}
	}
	return false;
}

/*******************************************************************************
 * Finds the first Rule that matches the given sequence of Postures.
 */
const Rule*
Model::findFirstMatchingRule(const std::vector<RuleExpressionData>& ruleExpressionData, unsigned int& ruleIndex) const
{
	if (ruleList_.empty()) {
		ruleIndex = 0;
		return nullptr;
	}

	unsigned int i = 0;
	for (const auto& r : ruleList_) {
		if (r->numberOfExpressions() <= ruleExpressionData.size()) {
			if (r->evalBooleanExpression(ruleExpressionData)) {
				ruleIndex = i;
				return r.get();
			}
		}
		++i;
	}

	ruleIndex = 0;
	return nullptr;
}

/*******************************************************************************
 * Validate the model.
 *
 * NOTE: The validation is not complete.
 */
void
Model::validate() const
{
	if (ruleList_.empty()) return;
	unsigned int ruleNumber = 1;
	for (auto& rule : ruleList_) {
		try {
			rule->validate(*this);
		} catch (Exception& e) {
			THROW_EXCEPTION(ValidationException, "Error in the validation of rule " << ruleNumber
						<< ". Cause: " << e.what());
		}

		++ruleNumber;
	}
}

} /* namespace VTMControlModel */
} /* namespace GS */
