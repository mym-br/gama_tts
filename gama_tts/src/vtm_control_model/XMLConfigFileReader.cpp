/***************************************************************************
 *  Copyright 2014, 2015 Marcelo Y. Matuda                                 *
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

#include "XMLConfigFileReader.h"

#include <memory>
#include <string_view>

#include "Exception.h"
#include "Log.h"
#include "Model.h"
#include "RapidXmlUtil.h"
#include "Text.h"



using namespace rapidxml;

namespace {

constexpr std::string_view booleanExpressionTagName   = "boolean-expression";
constexpr std::string_view booleanExpressionsTagName  = "boolean-expressions";
constexpr std::string_view categoriesTagName          = "categories";
constexpr std::string_view categoryTagName            = "category";
constexpr std::string_view categoryRefTagName         = "category-ref";
constexpr std::string_view commentTagName             = "comment";
constexpr std::string_view equationTagName            = "equation";
constexpr std::string_view equationGroupTagName       = "equation-group";
constexpr std::string_view equationsTagName           = "equations";
constexpr std::string_view expressionSymbolsTagName   = "expression-symbols";
constexpr std::string_view parameterTagName           = "parameter";
constexpr std::string_view parameterProfilesTagName   = "parameter-profiles";
constexpr std::string_view parametersTagName          = "parameters";
constexpr std::string_view parameterTargetsTagName    = "parameter-targets";
constexpr std::string_view parameterTransitionTagName = "parameter-transition";
constexpr std::string_view pointOrSlopesTagName       = "point-or-slopes";
constexpr std::string_view pointTagName               = "point";
constexpr std::string_view pointsTagName              = "points";
constexpr std::string_view postureCategoriesTagName   = "posture-categories";
constexpr std::string_view posturesTagName            = "postures";
constexpr std::string_view postureTagName             = "posture";
constexpr std::string_view ruleTagName                = "rule";
constexpr std::string_view rulesTagName               = "rules";
constexpr std::string_view slopeTagName               = "slope";
constexpr std::string_view slopeRatioTagName          = "slope-ratio";
constexpr std::string_view slopesTagName              = "slopes";
constexpr std::string_view specialProfilesTagName     = "special-profiles";
constexpr std::string_view specialTransitionsTagName  = "special-transitions";
constexpr std::string_view symbolEquationTagName      = "symbol-equation";
constexpr std::string_view symbolsTagName             = "symbols";
constexpr std::string_view symbolTagName              = "symbol";
constexpr std::string_view symbolTargetsTagName       = "symbol-targets";
constexpr std::string_view targetTagName              = "target";
constexpr std::string_view transitionTagName          = "transition";
constexpr std::string_view transitionGroupTagName     = "transition-group";
constexpr std::string_view transitionsTagName         = "transitions";

constexpr std::string_view defaultAttrName        = "default";
constexpr std::string_view equationAttrName       = "equation";
constexpr std::string_view formulaAttrName        = "formula";
constexpr std::string_view freeTimeAttrName       = "free-time";
constexpr std::string_view maximumAttrName        = "maximum";
constexpr std::string_view minimumAttrName        = "minimum";
constexpr std::string_view nameAttrName           = "name";
//constexpr std::string_view p12AttrName            = "p12";
//constexpr std::string_view p23AttrName            = "p23";
//constexpr std::string_view p34AttrName            = "p34";
constexpr std::string_view slopeAttrName          = "slope";
constexpr std::string_view symbolAttrName         = "symbol";
constexpr std::string_view timeExpressionAttrName = "time-expression";
constexpr std::string_view typeAttrName           = "type";
constexpr std::string_view transitionAttrName     = "transition";
constexpr std::string_view valueAttrName          = "value";

constexpr std::string_view beatSymbolName       = "beat";
//constexpr std::string_view durationSymbolName   = "duration";
constexpr std::string_view mark1SymbolName      = "mark1";
constexpr std::string_view mark2SymbolName      = "mark2";
constexpr std::string_view mark3SymbolName      = "mark3";
//constexpr std::string_view qssaSymbolName       = "qssa";
//constexpr std::string_view qssbSymbolName       = "qssb";
constexpr std::string_view rdSymbolName         = "rd";
//constexpr std::string_view transitionSymbolName = "transition";

} /* namespace */

namespace GS {
namespace VTMControlModel {

using namespace RapidXmlUtil;

void
XMLConfigFileReader::parseCategories(rapidxml::xml_node<char>* categoriesElem)
{
	for (xml_node<char>* categoryElem = firstChild(categoriesElem, categoryTagName);
				categoryElem;
				categoryElem = nextSibling(categoryElem, categoryTagName)) {

		auto newCategory = std::make_shared<Category>(attributeValue(categoryElem, nameAttrName));
		model_.categoryList().push_back(newCategory);

		xml_node<char>* commentElem = firstChild(categoryElem, commentTagName);
		if (commentElem) {
			model_.categoryList().back()->setComment(commentElem->value());
		}
	}
}

void
XMLConfigFileReader::parseParameters(rapidxml::xml_node<char>* parametersElem)
{
	for (xml_node<char>* parameterElem = firstChild(parametersElem, parameterTagName);
				parameterElem;
				parameterElem = nextSibling(parameterElem, parameterTagName)) {

		std::string name   = attributeValue(parameterElem, nameAttrName);
		float minimum      = Text::parseString<float>(attributeValue(parameterElem, minimumAttrName));
		float maximum      = Text::parseString<float>(attributeValue(parameterElem, maximumAttrName));
		float defaultValue = Text::parseString<float>(attributeValue(parameterElem, defaultAttrName));
		std::string comment;

		xml_node<char>* commentElem = firstChild(parameterElem, commentTagName);
		if (commentElem) {
			comment = commentElem->value();
		}

		model_.parameterList().emplace_back(name, minimum, maximum, defaultValue, comment);
	}
}

void
XMLConfigFileReader::parseSymbols(rapidxml::xml_node<char>* symbolsElem)
{
	for (xml_node<char>* symbolElem = firstChild(symbolsElem, symbolTagName);
				symbolElem;
				symbolElem = nextSibling(symbolElem, symbolTagName)) {

		std::string name   = attributeValue(symbolElem, nameAttrName);
		float minimum      = Text::parseString<float>(attributeValue(symbolElem, minimumAttrName));
		float maximum      = Text::parseString<float>(attributeValue(symbolElem, maximumAttrName));
		float defaultValue = Text::parseString<float>(attributeValue(symbolElem, defaultAttrName));
		std::string comment;

		xml_node<char>* commentElem = firstChild(symbolElem, commentTagName);
		if (commentElem) {
			comment = commentElem->value();
		}

		model_.symbolList().emplace_back(name, minimum, maximum, defaultValue, comment);
	}
}

void
XMLConfigFileReader::parsePostureSymbols(rapidxml::xml_node<char>* symbolTargetsElem, Posture& posture)
{
	for (xml_node<char>* targetElem = firstChild(symbolTargetsElem, targetTagName);
				targetElem;
				targetElem = nextSibling(targetElem, targetTagName)) {

		const std::string name = attributeValue(targetElem, nameAttrName);
		const std::string value = attributeValue(targetElem, valueAttrName);
		for (unsigned int i = 0, size = model_.symbolList().size(); i < size; ++i) {
			const Symbol& symbol = model_.symbolList()[i];
			if (symbol.name() == name) {
				posture.setSymbolTarget(i, Text::parseString<float>(value));
			}
		}
	}
}

void
XMLConfigFileReader::parsePostureCategories(rapidxml::xml_node<char>* postureCategoriesElem, Posture& posture)
{
	for (xml_node<char>* catRefElem = firstChild(postureCategoriesElem, categoryRefTagName);
				catRefElem;
				catRefElem = nextSibling(catRefElem, categoryRefTagName)) {

		const std::string name = attributeValue(catRefElem, nameAttrName);
		if (name != posture.name()) {
			std::shared_ptr<Category> postureCat = model_.findCategory(name);
			if (!postureCat) {
				THROW_EXCEPTION(VTMControlModelException, "Posture category not found: " << name << '.');
			}
			posture.categoryList().push_back(postureCat);
		}
	}
}

void
XMLConfigFileReader::parsePostureParameters(rapidxml::xml_node<char>* parameterTargetsElem, Posture& posture)
{
	for (xml_node<char>* targetElem = firstChild(parameterTargetsElem, targetTagName);
				targetElem;
				targetElem = nextSibling(targetElem, targetTagName)) {

		std::string parameterName = attributeValue(targetElem, nameAttrName);
		unsigned int parameterIndex = model_.findParameterIndex(parameterName);

		posture.setParameterTarget(parameterIndex, Text::parseString<float>(attributeValue(targetElem, valueAttrName)));
	}
}

void
XMLConfigFileReader::parsePosture(rapidxml::xml_node<char>* postureElem)
{
	auto posture = std::make_unique<Posture>(
					attributeValue(postureElem, symbolAttrName),
					model_.parameterList().size(),
					model_.symbolList().size());

	for (xml_node<char>* childElem = firstChild(postureElem);
				childElem;
				childElem = nextSibling(childElem)) {
		if (compareElementName(childElem, postureCategoriesTagName)) {
			parsePostureCategories(childElem, *posture);
		} else if (compareElementName(childElem, parameterTargetsTagName)) {
			parsePostureParameters(childElem, *posture);
		} else if (compareElementName(childElem, symbolTargetsTagName)) {
			parsePostureSymbols(childElem, *posture);
		} else if (compareElementName(childElem, commentTagName)) {
			posture->setComment(childElem->value());
		}
	}

	model_.postureList().add(std::move(posture));
}

void
XMLConfigFileReader::parsePostures(rapidxml::xml_node<char>* posturesElem)
{
	for (xml_node<char>* postureElem = firstChild(posturesElem, postureTagName);
				postureElem;
				postureElem = nextSibling(postureElem, postureTagName)) {
		parsePosture(postureElem);
	}
}

void
XMLConfigFileReader::parseEquationsGroup(rapidxml::xml_node<char>* equationGroupElem)
{
	EquationGroup group;
	group.name = attributeValue(equationGroupElem, nameAttrName);

	for (xml_node<char>* equationElem = firstChild(equationGroupElem, equationTagName);
				equationElem;
				equationElem = nextSibling(equationElem, equationTagName)) {

		std::string name = attributeValue(equationElem, nameAttrName);
		std::string formula = attributeValue(equationElem, formulaAttrName, true);
		std::string comment;
		xml_node<char>* commentElem = firstChild(equationElem, commentTagName);
		if (commentElem) {
			comment = commentElem->value();
		}

		if (formula.empty()) {
			LOG_ERROR("Equation " << name << " without formula (ignored).");
		} else {
			auto eq = std::make_shared<Equation>(name);
			eq->setFormula(formula);
			if (!comment.empty()) {
				eq->setComment(comment);
			}

			group.equationList.push_back(eq);
		}
	}

	model_.equationGroupList().push_back(std::move(group));
}

void
XMLConfigFileReader::parseEquations(rapidxml::xml_node<char>* equationsElem)
{
	for (xml_node<char>* groupElem = firstChild(equationsElem, equationGroupTagName);
				groupElem;
				groupElem = nextSibling(groupElem, equationGroupTagName)) {
		parseEquationsGroup(groupElem);
	}
}

void
XMLConfigFileReader::parseSlopeRatio(rapidxml::xml_node<char>* slopeRatioElem, Transition& transition)
{
	auto p = std::make_unique<Transition::SlopeRatio>();

	for (xml_node<char>* childElem = firstChild(slopeRatioElem);
				childElem;
				childElem = nextSibling(childElem)) {
		if (compareElementName(childElem, pointsTagName)) {
			for (xml_node<char>* pointElem = firstChild(childElem, pointTagName);
						pointElem;
						pointElem = nextSibling(pointElem, pointTagName)) {
				auto p2 = std::make_unique<Transition::Point>();
				p2->type = Transition::Point::getTypeFromName(attributeValue(pointElem, typeAttrName));
				p2->value = Text::parseString<float>(attributeValue(pointElem, valueAttrName));

				const std::string timeExpr = attributeValue(pointElem, timeExpressionAttrName, true);
				if (timeExpr.empty()) {
					p2->freeTime = Text::parseString<float>(attributeValue(pointElem, freeTimeAttrName));
				} else {
					std::shared_ptr<Equation> equation = model_.findEquation(timeExpr);
					if (!equation) {
						THROW_EXCEPTION(UnavailableResourceException, "Equation not found: " << timeExpr << '.');
					}
					p2->timeExpression = equation;
				}

				p->pointList.push_back(std::move(p2));
			}
		} else if (compareElementName(childElem, slopesTagName)) {
			for (xml_node<char>* slopeElem = firstChild(childElem, slopeTagName);
						slopeElem;
						slopeElem = nextSibling(slopeElem, slopeTagName)) {
				auto p2 = std::make_unique<Transition::Slope>();
				p2->slope = Text::parseString<float>(attributeValue(slopeElem, slopeAttrName));
				p->slopeList.push_back(std::move(p2));
			}
		}
	}

	transition.pointOrSlopeList().push_back(std::move(p));
}

void
XMLConfigFileReader::parseTransitionPointOrSlopes(rapidxml::xml_node<char>* pointOrSlopesElem, Transition& transition)
{
	for (xml_node<char>* childElem = firstChild(pointOrSlopesElem);
				childElem;
				childElem = nextSibling(childElem)) {
		if (compareElementName(childElem, pointTagName)) {
			auto p = std::make_unique<Transition::Point>();
			p->type = Transition::Point::getTypeFromName(attributeValue(childElem, typeAttrName));
			p->value = Text::parseString<float>(attributeValue(childElem, valueAttrName));

			const std::string timeExpr = attributeValue(childElem, timeExpressionAttrName, true);
			if (timeExpr.empty()) {
				p->freeTime = Text::parseString<float>(attributeValue(childElem, freeTimeAttrName));
			} else {
				std::shared_ptr<Equation> equation = model_.findEquation(timeExpr);
				if (!equation) {
					THROW_EXCEPTION(UnavailableResourceException, "Equation not found: " << timeExpr << '.');
				}
				p->timeExpression = equation;
			}

			transition.pointOrSlopeList().push_back(std::move(p));
		} else if (compareElementName(childElem, slopeRatioTagName)) {
			parseSlopeRatio(childElem, transition);
		}
	}
}

void
XMLConfigFileReader::parseTransitionsGroup(rapidxml::xml_node<char>* transitionGroupElem, bool special)
{
	TransitionGroup group;
	group.name = attributeValue(transitionGroupElem, nameAttrName);

	for (xml_node<char>* childElem = firstChild(transitionGroupElem,transitionTagName);
				childElem;
				childElem = nextSibling(childElem, transitionTagName)) {

		std::string name = attributeValue(childElem, nameAttrName);
		Transition::Type type = Transition::getTypeFromName(attributeValue(childElem, typeAttrName));

		auto tr = std::make_shared<Transition>(name, type, special);

		for (xml_node<char>* transitionChildElem = firstChild(childElem);
					transitionChildElem;
					transitionChildElem = nextSibling(transitionChildElem)) {
			if (compareElementName(transitionChildElem, pointOrSlopesTagName)) {
				parseTransitionPointOrSlopes(transitionChildElem, *tr);
			} else if (compareElementName(transitionChildElem, commentTagName)) {
				tr->setComment(transitionChildElem->value());
			}
		}

		group.transitionList.push_back(tr);
	}

	if (special) {
		model_.specialTransitionGroupList().push_back(std::move(group));
	} else {
		model_.transitionGroupList().push_back(std::move(group));
	}
}

void
XMLConfigFileReader::parseTransitions(rapidxml::xml_node<char>* transitionsElem, bool special)
{
	for (xml_node<char>* groupElem = firstChild(transitionsElem, transitionGroupTagName);
				groupElem;
				groupElem = nextSibling(groupElem, transitionGroupTagName)) {
		parseTransitionsGroup(groupElem, special);
	}
}

void
XMLConfigFileReader::parseRuleParameterProfiles(rapidxml::xml_node<char>* parameterProfilesElem, Rule& rule)
{
	for (xml_node<char>* paramTransElem = firstChild(parameterProfilesElem,parameterTransitionTagName);
				paramTransElem;
				paramTransElem = nextSibling(paramTransElem, parameterTransitionTagName)) {

		std::string parameterName = attributeValue(paramTransElem, nameAttrName);
		unsigned int parameterIndex = model_.findParameterIndex(parameterName);

		const std::string transitionName = attributeValue(paramTransElem, transitionAttrName);
		std::shared_ptr<Transition> transition = model_.findTransition(transitionName);
		if (!transition) {
			THROW_EXCEPTION(UnavailableResourceException, "Transition not found: " << transitionName << '.');
		}

		rule.setParamProfileTransition(parameterIndex, transition);
	}
}

void
XMLConfigFileReader::parseRuleSpecialProfiles(rapidxml::xml_node<char>* specialProfilesElem, Rule& rule)
{
	for (xml_node<char>* paramTransElem = firstChild(specialProfilesElem, parameterTransitionTagName);
				paramTransElem;
				paramTransElem = nextSibling(paramTransElem, parameterTransitionTagName)) {

		std::string parameterName = attributeValue(paramTransElem, nameAttrName);
		unsigned int parameterIndex = model_.findParameterIndex(parameterName);

		const std::string transitionName = attributeValue(paramTransElem, transitionAttrName);
		std::shared_ptr<Transition> transition = model_.findSpecialTransition(transitionName);
		if (!transition) {
			THROW_EXCEPTION(UnavailableResourceException, "Special transition not found: " << transitionName << '.');
		}

		rule.setSpecialProfileTransition(parameterIndex, transition);
	}
}

void
XMLConfigFileReader::parseRuleExpressionSymbols(rapidxml::xml_node<char>* expressionSymbolsElem, Rule& rule)
{
	for (xml_node<char>* symbEquElem = firstChild(expressionSymbolsElem, symbolEquationTagName);
				symbEquElem;
				symbEquElem = nextSibling(symbEquElem, symbolEquationTagName)) {

		const std::string name = attributeValue(symbEquElem, nameAttrName);
		const std::string equationName = attributeValue(symbEquElem, equationAttrName);

		std::shared_ptr<Equation> equation = model_.findEquation(equationName);
		if (!equation) {
			THROW_EXCEPTION(UnavailableResourceException, "Equation not found: " << equationName << '.');
		}

		if (name == rdSymbolName) {
			rule.exprSymbolEquations().duration = equation;
		} else if (name == beatSymbolName) {
			rule.exprSymbolEquations().beat = equation;
		} else if (name == mark1SymbolName) {
			rule.exprSymbolEquations().mark1 = equation;
		} else if (name == mark2SymbolName) {
			rule.exprSymbolEquations().mark2 = equation;
		} else if (name == mark3SymbolName) {
			rule.exprSymbolEquations().mark3 = equation;
		}
	}
}

void
XMLConfigFileReader::parseRuleBooleanExpressions(rapidxml::xml_node<char>* booleanExpressionsElem, Rule& rule)
{
	std::vector<std::string> exprList;
	for (xml_node<char>* boolExprElem = firstChild(booleanExpressionsElem, booleanExpressionTagName);
				boolExprElem;
				boolExprElem = nextSibling(boolExprElem, booleanExpressionTagName)) {
		exprList.push_back(boolExprElem->value());
	}
	rule.setBooleanExpressionList(exprList, model_);
}

void
XMLConfigFileReader::parseRule(rapidxml::xml_node<char>* ruleElem)
{
	auto rule = std::make_unique<Rule>(model_.parameterList().size());

	for (xml_node<char>* childElem = firstChild(ruleElem);
				childElem;
				childElem = nextSibling(childElem)) {
		if (compareElementName(childElem, booleanExpressionsTagName)) {
			parseRuleBooleanExpressions(childElem, *rule);
		} else if (compareElementName(childElem, parameterProfilesTagName)) {
			parseRuleParameterProfiles(childElem, *rule);
		} else if (compareElementName(childElem, specialProfilesTagName)) {
			parseRuleSpecialProfiles(childElem, *rule);
		} else if (compareElementName(childElem, expressionSymbolsTagName)) {
			parseRuleExpressionSymbols(childElem, *rule);
		} else if (compareElementName(childElem, commentTagName)) {
			rule->setComment(childElem->value());
		}
	}

	model_.ruleList().push_back(std::move(rule));
}

void
XMLConfigFileReader::parseRules(rapidxml::xml_node<char>* rulesElem)
{
	for (xml_node<char>* ruleElem = firstChild(rulesElem, ruleTagName);
				ruleElem;
				ruleElem = nextSibling(ruleElem, ruleTagName)) {
		parseRule(ruleElem);
	}
}



/*******************************************************************************
 * Constructor.
 */
XMLConfigFileReader::XMLConfigFileReader(Model& model, const std::string& filePath)
		: model_(model)
		, filePath_(filePath)
{
}

void
XMLConfigFileReader::loadModel()
{
	std::string source = readXMLFile(filePath_.c_str());
	xml_document<char> doc;
	doc.parse<parse_no_data_nodes | parse_validate_closing_tags>(&source[0]);

	xml_node<char>* root = doc.first_node();
	if (root == 0) {
		THROW_EXCEPTION(XMLException, "Root element not found.");
	}

	LOG_DEBUG("categories");
	xml_node<char>* categoriesElem = firstChild(root, categoriesTagName);
	if (categoriesElem == 0) {
		THROW_EXCEPTION(VTMControlModelException, "Categories element not found.");
	}
	parseCategories(categoriesElem);

	LOG_DEBUG("parameters");
	xml_node<char>* parametersElem = firstChild(root, parametersTagName);
	if (parametersElem == 0) {
		THROW_EXCEPTION(VTMControlModelException, "Parameters element not found.");
	}
	parseParameters(parametersElem);

	LOG_DEBUG("symbols");
	xml_node<char>* symbolsElem = firstChild(root, symbolsTagName);
	if (symbolsElem == 0) {
		THROW_EXCEPTION(VTMControlModelException, "Categories element not found.");
	}
	parseSymbols(symbolsElem);

	LOG_DEBUG("postures");
	xml_node<char>* posturesElem = firstChild(root, posturesTagName);
	if (posturesElem == 0) {
		THROW_EXCEPTION(VTMControlModelException, "Postures element not found.");
	}
	parsePostures(posturesElem);

	LOG_DEBUG("equations");
	xml_node<char>* equationsElem = firstChild(root, equationsTagName);
	if (equationsElem == 0) {
		THROW_EXCEPTION(VTMControlModelException, "Equations element not found.");
	}
	parseEquations(equationsElem);

	LOG_DEBUG("transitions");
	xml_node<char>* transitionsElem = firstChild(root, transitionsTagName);
	if (transitionsElem == 0) {
		THROW_EXCEPTION(VTMControlModelException, "Transitions element not found.");
	}
	parseTransitions(transitionsElem, false);

	LOG_DEBUG("special-transitions");
	xml_node<char>* specialTransitionsElem = firstChild(root, specialTransitionsTagName);
	if (specialTransitionsElem == 0) {
		THROW_EXCEPTION(VTMControlModelException, "Special-transitions element not found.");
	}
	parseTransitions(specialTransitionsElem, true);

	LOG_DEBUG("rules");
	xml_node<char>* rulesElem = firstChild(root, rulesTagName);
	if (rulesElem == 0) {
		THROW_EXCEPTION(VTMControlModelException, "Rules element not found.");
	}
	parseRules(rulesElem);
}

} /* namespace VTMControlModel */
} /* namespace GS */
