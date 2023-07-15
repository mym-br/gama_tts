/***************************************************************************
 *  Copyright 2014, 2015 Marcelo Y. Matuda                                 *
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

#include "RuleManagerWindow.h"

#include <memory>
#include <utility> /* move, swap */

#include <QMessageBox>
#include <QSignalBlocker>

#include "Model.h"
#include "ui_RuleManagerWindow.h"

#define NUM_RULES_TABLE_COLUMNS 1
#define NUM_RULE_TRANSITIONS_TABLE_COLUMNS 2
#define NUM_RULE_SPECIAL_TRANSITIONS_TABLE_COLUMNS 2
#define NUM_RULE_SYMBOL_EQUATIONS_TABLE_COLUMNS 2
#define NUM_TRANSITIONS_TREE_COLUMNS 1
#define NUM_SPECIAL_TRANSITIONS_TREE_COLUMNS 1
#define NUM_EQUATIONS_TREE_COLUMNS 1



namespace GS {

RuleManagerWindow::RuleManagerWindow(QWidget* parent)
		: QWidget(parent)
		, ui_(std::make_unique<Ui::RuleManagerWindow>())
		, model_()
		, selectedRule_()
{
	ui_->setupUi(this);

	QFontMetrics fm = fontMetrics();
	int rowHeight = fm.height() + fm.ascent();

	QHeaderView* vHeader = ui_->rulesTable->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->rulesTable->setColumnCount(NUM_RULES_TABLE_COLUMNS);
	ui_->rulesTable->setHorizontalHeaderLabels(QStringList() << tr("Rule"));
	ui_->rulesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	vHeader = ui_->ruleTransitionsTable->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->ruleTransitionsTable->setColumnCount(NUM_RULE_TRANSITIONS_TABLE_COLUMNS);
	ui_->ruleTransitionsTable->setHorizontalHeaderLabels(QStringList() << tr("Parameter") << tr("Rule transition"));
	ui_->ruleTransitionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	vHeader = ui_->ruleSpecialTransitionsTable->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->ruleSpecialTransitionsTable->setColumnCount(NUM_RULE_SPECIAL_TRANSITIONS_TABLE_COLUMNS);
	ui_->ruleSpecialTransitionsTable->setHorizontalHeaderLabels(QStringList() << tr("Parameter") << tr("Rule special transition"));
	ui_->ruleSpecialTransitionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	vHeader = ui_->ruleSymbolEquationsTable->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->ruleSymbolEquationsTable->setColumnCount(NUM_RULE_SYMBOL_EQUATIONS_TABLE_COLUMNS);
	ui_->ruleSymbolEquationsTable->setHorizontalHeaderLabels(QStringList() << tr("Symbol") << tr("Rule equation"));
	ui_->ruleSymbolEquationsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	ui_->transitionsTree->setColumnCount(NUM_TRANSITIONS_TREE_COLUMNS);
	ui_->transitionsTree->setHeaderLabels(QStringList() << tr("Transition"));

	ui_->specialTransitionsTree->setColumnCount(NUM_SPECIAL_TRANSITIONS_TREE_COLUMNS);
	ui_->specialTransitionsTree->setHeaderLabels(QStringList() << tr("Special transition"));

	ui_->equationsTree->setColumnCount(NUM_EQUATIONS_TREE_COLUMNS);
	ui_->equationsTree->setHeaderLabels(QStringList() << tr("Equation"));
}

RuleManagerWindow::~RuleManagerWindow()
{
}

void
RuleManagerWindow::resetModel(VTMControlModel::Model* model)
{
	model_ = model;
	selectedRule_ = nullptr;

	if (model_ == nullptr) {
		clearRuleData();
		ui_->rulesTable->setRowCount(0);
	} else {
		setupRulesList();
		ui_->rulesTable->setCurrentItem(nullptr);

		setupTransitionsTree();
		setupSpecialTransitionsTree();
		setupEquationsTree();
	}
}

void
RuleManagerWindow::on_removeButton_clicked()
{
	if (model_ == nullptr) return;

	QTableWidgetItem* currItem = ui_->rulesTable->currentItem();
	if (currItem == nullptr) return;

	int currRow = currItem->row();
	model_->ruleList().erase(model_->ruleList().begin() + currRow);

	clearRuleData();
	setupRulesList();
	ui_->rulesTable->setCurrentItem(nullptr);
	selectedRule_ = nullptr;
}

void
RuleManagerWindow::on_addButton_clicked()
{
	if (model_ == nullptr) return;

	QString exp1 = ui_->expression1LineEdit->text().trimmed();
	QString exp2 = ui_->expression2LineEdit->text().trimmed();
	QString exp3 = ui_->expression3LineEdit->text().trimmed();
	QString exp4 = ui_->expression4LineEdit->text().trimmed();

	unsigned int numExpr = numInputExpressions(exp1, exp2, exp3, exp4);
	if (numExpr == 0) {
		QMessageBox::critical(this, tr("Error"), tr("Wrong number of expressions."));
		return;
	}

	auto newRule = std::make_unique<VTMControlModel::Rule>(model_->parameterList().size());

	std::vector<std::string> exprList;
	exprList.push_back(exp1.toStdString());
	exprList.push_back(exp2.toStdString());
	if (numExpr >= 3) {
		exprList.push_back(exp3.toStdString());
	}
	if (numExpr == 4) {
		exprList.push_back(exp4.toStdString());
	}
	try {
		newRule->setBooleanExpressionList(exprList, *model_);
	} catch (const Exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
		return;
	}

	QTableWidgetItem* currItem = ui_->rulesTable->currentItem();
	unsigned int insertRow;
	if (currItem == nullptr) {
		insertRow = model_->ruleList().size();
	} else {
		insertRow = currItem->row();
	}
	model_->ruleList().insert(model_->ruleList().begin() + insertRow, std::move(newRule));

	setupRulesList();

	// Force the emission of signal currentItemChanged.
	ui_->rulesTable->setCurrentItem(nullptr);
	ui_->rulesTable->setCurrentCell(insertRow, 0);

	emit categoryReferenceChanged();
}

void
RuleManagerWindow::on_updateButton_clicked()
{
	qDebug("on_updateButton_clicked");

	if (model_ == nullptr) return;

	QTableWidgetItem* currItem = ui_->rulesTable->currentItem();
	if (currItem == nullptr) return;

	QString exp1 = ui_->expression1LineEdit->text().trimmed();
	QString exp2 = ui_->expression2LineEdit->text().trimmed();
	QString exp3 = ui_->expression3LineEdit->text().trimmed();
	QString exp4 = ui_->expression4LineEdit->text().trimmed();

	unsigned int numExpr = numInputExpressions(exp1, exp2, exp3, exp4);
	if (numExpr == 0) {
		QMessageBox::critical(this, tr("Error"), tr("Wrong number of expressions."));
		return;
	}

	std::vector<std::string> exprList;
	exprList.push_back(exp1.toStdString());
	exprList.push_back(exp2.toStdString());
	if (numExpr >= 3) {
		exprList.push_back(exp3.toStdString());
	}
	if (numExpr == 4) {
		exprList.push_back(exp4.toStdString());
	}
	const int currRow = currItem->row();
	try {
		VTMControlModel::Rule& rule = *model_->ruleList()[currRow];
		rule.setBooleanExpressionList(exprList, *model_);
	} catch (const Exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
	}

	setupRulesList();

	// Force the emission of signal currentItemChanged.
	ui_->rulesTable->setCurrentItem(nullptr);
	ui_->rulesTable->setCurrentCell(currRow, 0);

	emit categoryReferenceChanged();
}

void
RuleManagerWindow::on_moveUpButton_clicked()
{
	if (model_ == nullptr) return;

	QTableWidgetItem* currItem = ui_->rulesTable->currentItem();
	if (currItem == nullptr) return;

	int currRow = currItem->row();
	if (currRow == 0) return;

	std::swap(model_->ruleList()[currRow], model_->ruleList()[currRow - 1]);

	setupRulesList();
	ui_->rulesTable->setCurrentCell(currRow - 1, 0);
}

void
RuleManagerWindow::on_moveDownButton_clicked()
{
	if (model_ == nullptr) return;

	QTableWidgetItem* currItem = ui_->rulesTable->currentItem();
	if (currItem == nullptr) return;

	int currRow = currItem->row();
	if (currRow == static_cast<int>(model_->ruleList().size()) - 1) return;

	std::swap(model_->ruleList()[currRow], model_->ruleList()[currRow + 1]);

	setupRulesList();
	ui_->rulesTable->setCurrentCell(currRow + 1, 0);
}

void
RuleManagerWindow::on_rulesTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* /*previous*/)
{
	qDebug("on_rulesTable_currentItemChanged");

	if (model_ == nullptr) return;
	if (current == nullptr) {
		clearRuleData();
		selectedRule_ = nullptr;
		return;
	}

	int row = current->row();
	selectedRule_ = model_->ruleList()[row].get();

	loadRuleData();
}

void
RuleManagerWindow::clearRuleData()
{
	ui_->expression1LineEdit->clear();
	ui_->expression2LineEdit->clear();
	ui_->expression3LineEdit->clear();
	ui_->expression4LineEdit->clear();
	ui_->matches1Label->setText(tr("Total matches: 0"));
	ui_->matches2Label->setText(tr("Total matches: 0"));
	ui_->matches3Label->setText(tr("Total matches: 0"));
	ui_->matches4Label->setText(tr("Total matches: 0"));
	ui_->matches1ListWidget->clear();
	ui_->matches2ListWidget->clear();
	ui_->matches3ListWidget->clear();
	ui_->matches4ListWidget->clear();
	ui_->combinationsLineEdit->clear();

	ui_->ruleTransitionsTable->setRowCount(0);
	ui_->ruleSpecialTransitionsTable->setRowCount(0);
	ui_->ruleSymbolEquationsTable->setRowCount(0);

	ui_->ruleTransitionsTable->setCurrentItem(nullptr);
	ui_->ruleSpecialTransitionsTable->setCurrentItem(nullptr);
	ui_->ruleSymbolEquationsTable->setCurrentItem(nullptr);

	ui_->transitionsTree->setCurrentItem(nullptr);
	ui_->specialTransitionsTree->setCurrentItem(nullptr);
	ui_->equationsTree->setCurrentItem(nullptr);

	ui_->commentTextEdit->clear();
}

// Slot.
void
RuleManagerWindow::unselectRule()
{
	ui_->rulesTable->setCurrentItem(nullptr);
}

// Slot.
void
RuleManagerWindow::loadRuleData()
{
	clearRuleData();

	if (selectedRule_ == nullptr) {
		return;
	}

	unsigned int numExpr = selectedRule_->booleanExpressionList().size();
	if (numExpr >= 1) ui_->expression1LineEdit->setText(selectedRule_->booleanExpressionList()[0].c_str());
	if (numExpr >= 2) ui_->expression2LineEdit->setText(selectedRule_->booleanExpressionList()[1].c_str());
	if (numExpr >= 3) ui_->expression3LineEdit->setText(selectedRule_->booleanExpressionList()[2].c_str());
	if (numExpr == 4) ui_->expression4LineEdit->setText(selectedRule_->booleanExpressionList()[3].c_str());

	showRuleStatistics(*selectedRule_);

	setupRuleTransitionsTable();
	setupRuleSpecialTransitionsTable();
	setupRuleSymbolEquationsTable();

	ui_->commentTextEdit->setPlainText(selectedRule_->comment().c_str());
}

void
RuleManagerWindow::setupRulesList()
{
	if (model_ == nullptr) return;

	QTableWidget* table = ui_->rulesTable;
	{
		QSignalBlocker blocker(table);

		table->setRowCount(model_->ruleList().size());
		for (unsigned int i = 0, size = model_->ruleList().size(); i < size; ++i) {
			const auto& rule = model_->ruleList()[i];

			QString ruleText;
			for (unsigned int j = 0, size = rule->booleanExpressionList().size(); j < size; ++j) {
				if (j > 0) {
					ruleText += " >> ";
				}
				ruleText += rule->booleanExpressionList()[j].c_str();
			}

			auto item = std::make_unique<QTableWidgetItem>(ruleText);
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			table->setItem(i, 0, item.release());
		}
	}
}

void
RuleManagerWindow::showRuleStatistics(const VTMControlModel::Rule& rule)
{
	unsigned int count1 = 0;
	unsigned int count2 = 0;
	unsigned int count3 = 0;
	unsigned int count4 = 0;

	VTMControlModel::RuleExpressionData exprData;
	exprData.tempo = 1.0;
	for (unsigned int i = 0, size = model_->postureList().size(); i < size; ++i) {
		const auto& posture = model_->postureList()[i];
		exprData.posture = &posture;

		exprData.marked = false;
		if (rule.evalBooleanExpression(exprData, 0)) {
			++count1;
			ui_->matches1ListWidget->addItem(posture.name().c_str());
		}
		exprData.marked = true;
		if (rule.evalBooleanExpression(exprData, 0)) {
			++count1;
			ui_->matches1ListWidget->addItem(QString(posture.name().c_str()) + '\'');
		}

		exprData.marked = false;
		if (rule.evalBooleanExpression(exprData, 1)) {
			++count2;
			ui_->matches2ListWidget->addItem(posture.name().c_str());
		}
		exprData.marked = true;
		if (rule.evalBooleanExpression(exprData, 1)) {
			++count2;
			ui_->matches2ListWidget->addItem(QString(posture.name().c_str()) + '\'');
		}

		exprData.marked = false;
		if (rule.evalBooleanExpression(exprData, 2)) {
			++count3;
			ui_->matches3ListWidget->addItem(posture.name().c_str());
		}
		exprData.marked = true;
		if (rule.evalBooleanExpression(exprData, 2)) {
			++count3;
			ui_->matches3ListWidget->addItem(QString(posture.name().c_str()) + '\'');
		}

		exprData.marked = false;
		if (rule.evalBooleanExpression(exprData, 3)) {
			++count4;
			ui_->matches4ListWidget->addItem(posture.name().c_str());
		}
		exprData.marked = true;
		if (rule.evalBooleanExpression(exprData, 3)) {
			++count4;
			ui_->matches4ListWidget->addItem(QString(posture.name().c_str()) + '\'');
		}
	}

	ui_->matches1Label->setText(tr("Total matches: %1").arg(count1));
	ui_->matches2Label->setText(tr("Total matches: %1").arg(count2));
	ui_->matches3Label->setText(tr("Total matches: %1").arg(count3));
	ui_->matches4Label->setText(tr("Total matches: %1").arg(count4));

	double combinations = 1.0;
	if (count1 != 0) {
		combinations *= static_cast<double>(count1);
	}
	if (count2 != 0) {
		combinations *= static_cast<double>(count2);
	}
	if (count3 != 0) {
		combinations *= static_cast<double>(count3);
	}
	if (count4 != 0) {
		combinations *= static_cast<double>(count4);
	}

	ui_->combinationsLineEdit->setText(QString::number(combinations));
}

unsigned int
RuleManagerWindow::numInputExpressions(const QString& exp1, const QString& exp2, const QString& exp3, const QString& exp4)
{
	if (!exp1.isEmpty() && !exp2.isEmpty()) {
		if (!exp3.isEmpty()) {
			if (!exp4.isEmpty()) {
				return 4;
			} else {
				return 3;
			}
		} else if (exp4.isEmpty()) {
			return 2;
		}
	}
	return 0;
}

void
RuleManagerWindow::on_ruleTransitionsTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* /*previous*/)
{
	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;
	if (current == nullptr) return;

	int row = ui_->ruleTransitionsTable->row(current);
	qDebug("on_ruleTransitionsTable_currentItemChanged row=%d", row);

	auto transition = selectedRule_->getParamProfileTransition(row);
	if (transition) {
		unsigned int groupIndex;
		unsigned int transitionIndex;
		if (model_->findTransitionIndex(transition->name(), groupIndex, transitionIndex)) {
			QTreeWidgetItem* topLevelItem = ui_->transitionsTree->topLevelItem(groupIndex);
			if (topLevelItem == nullptr) {
				qCritical("[RuleManagerWindow::on_ruleTransitionsTable_currentItemChanged]: Invalid group index: %u.", groupIndex);
				return;
			}
			QTreeWidgetItem* item = topLevelItem->child(transitionIndex);
			if (item == nullptr) {
				qCritical("[RuleManagerWindow::on_ruleTransitionsTable_currentItemChanged]: Invalid index: %u for groupIndex: %u.", transitionIndex, groupIndex);
				return;
			}
			ui_->transitionsTree->setCurrentItem(item);
		}
	} else {
		ui_->transitionsTree->setCurrentItem(nullptr);
	}
}

void
RuleManagerWindow::on_ruleSpecialTransitionsTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* /*previous*/)
{
	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;
	if (current == nullptr) return;

	int row = ui_->ruleSpecialTransitionsTable->row(current);
	qDebug("on_ruleSpecialTransitionsTable_currentItemChanged row=%d", row);

	auto transition = selectedRule_->getSpecialProfileTransition(row);
	if (transition) {
		unsigned int groupIndex;
		unsigned int transitionIndex;
		if (model_->findSpecialTransitionIndex(transition->name(), groupIndex, transitionIndex)) {
			QTreeWidgetItem* topLevelItem = ui_->specialTransitionsTree->topLevelItem(groupIndex);
			if (topLevelItem == nullptr) {
				qCritical("[RuleManagerWindow::on_ruleSpecialTransitionsTable_currentItemChanged]: Invalid group index: %u.", groupIndex);
				return;
			}
			QTreeWidgetItem* item = topLevelItem->child(transitionIndex);
			if (item == nullptr) {
				qCritical("[RuleManagerWindow::on_ruleSpecialTransitionsTable_currentItemChanged]: Invalid index: %u for groupIndex: %u.", transitionIndex, groupIndex);
				return;
			}
			ui_->specialTransitionsTree->setCurrentItem(item);
		}
	} else {
		ui_->specialTransitionsTree->setCurrentItem(nullptr);
	}
}

void
RuleManagerWindow::on_ruleSymbolEquationsTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* /*previous*/)
{
	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;
	if (current == nullptr) return;

	int row = ui_->ruleSymbolEquationsTable->row(current);
	qDebug("on_ruleSymbolEquationsTable_currentItemChanged row=%d", row);

	std::shared_ptr<VTMControlModel::Equation> equation;
	switch (row) {
	case 0:
		equation = selectedRule_->exprSymbolEquations().duration;
		break;
	case 1:
		equation = selectedRule_->exprSymbolEquations().beat;
		break;
	case 2:
		equation = selectedRule_->exprSymbolEquations().mark1;
		break;
	case 3:
		equation = selectedRule_->exprSymbolEquations().mark2;
		break;
	case 4:
		equation = selectedRule_->exprSymbolEquations().mark3;
		break;
	}

	if (equation) {
		unsigned int groupIndex;
		unsigned int equationIndex;
		if (model_->findEquationIndex(equation->name(), groupIndex, equationIndex)) {
			QTreeWidgetItem* topLevelItem = ui_->equationsTree->topLevelItem(groupIndex);
			if (topLevelItem == nullptr) {
				qCritical("[RuleManagerWindow::on_ruleSymbolEquationsTable_currentItemChanged]: Invalid group index: %u.", groupIndex);
				return;
			}
			QTreeWidgetItem* item = topLevelItem->child(equationIndex);
			if (item == nullptr) {
				qCritical("[RuleManagerWindow::on_ruleSymbolEquationsTable_currentItemChanged]: Invalid index: %u for groupIndex: %u.", equationIndex, groupIndex);
				return;
			}
			ui_->equationsTree->setCurrentItem(item);
		}
	} else {
		ui_->equationsTree->setCurrentItem(nullptr);
	}
}

void
RuleManagerWindow::on_transitionsTree_itemClicked(QTreeWidgetItem* item, int /*column*/)
{
	qDebug("on_transitionsTree_itemClicked");

	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;
	if (item == nullptr) return;

	QTreeWidgetItem* parent = item->parent();
	if (parent == nullptr) { // root item
		return;
	}
	int parentIndex = ui_->transitionsTree->indexOfTopLevelItem(parent);
	int index = parent->indexOfChild(item);
	const auto& transition = model_->transitionGroupList()[parentIndex].transitionList[index];

	QTableWidgetItem* currentItem = ui_->ruleTransitionsTable->currentItem();
	if (currentItem == nullptr) return;

	int row = currentItem->row();
	const auto& ruleTransition = selectedRule_->getParamProfileTransition(row);
	if (ruleTransition != transition) {
		selectedRule_->setParamProfileTransition(row, transition);
		setupRuleTransitionsTable();

		emit transitionReferenceChanged();
	}
}

void
RuleManagerWindow::on_specialTransitionsTree_itemClicked(QTreeWidgetItem* item, int /*column*/)
{
	qDebug("on_specialTransitionsTree_itemClicked");

	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;
	if (item == nullptr) return;

	QTreeWidgetItem* parent = item->parent();
	if (parent == nullptr) { // root item
		return;
	}
	int parentIndex = ui_->specialTransitionsTree->indexOfTopLevelItem(parent);
	int index = parent->indexOfChild(item);
	const auto& transition = model_->specialTransitionGroupList()[parentIndex].transitionList[index];

	QTableWidgetItem* currentItem = ui_->ruleSpecialTransitionsTable->currentItem();
	if (currentItem == nullptr) return;

	int row = currentItem->row();
	const auto& ruleTransition = selectedRule_->getSpecialProfileTransition(row);
	if (ruleTransition != transition) {
		selectedRule_->setSpecialProfileTransition(row, transition);
		setupRuleSpecialTransitionsTable();

		emit specialTransitionReferenceChanged();
	}
}

void
RuleManagerWindow::on_equationsTree_itemClicked(QTreeWidgetItem* item, int /*column*/)
{
	qDebug("on_equationsTree_itemClicked");

	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;
	if (item == nullptr) return;

	QTreeWidgetItem* parent = item->parent();
	if (parent == nullptr) { // root item
		return;
	}
	int parentIndex = ui_->equationsTree->indexOfTopLevelItem(parent);
	int index = parent->indexOfChild(item);
	const auto& equation = model_->equationGroupList()[parentIndex].equationList[index];

	QTableWidgetItem* currentItem = ui_->ruleSymbolEquationsTable->currentItem();
	if (currentItem == nullptr) return;

	std::shared_ptr<VTMControlModel::Equation>* ruleEquation = nullptr;
	int row = currentItem->row();
	switch (row) {
	case 0:
		ruleEquation = &selectedRule_->exprSymbolEquations().duration;
		break;
	case 1:
		ruleEquation = &selectedRule_->exprSymbolEquations().beat;
		break;
	case 2:
		ruleEquation = &selectedRule_->exprSymbolEquations().mark1;
		break;
	case 3:
		ruleEquation = &selectedRule_->exprSymbolEquations().mark2;
		break;
	case 4:
		ruleEquation = &selectedRule_->exprSymbolEquations().mark3;
		break;
	default:
		return;
	}

	if (*ruleEquation != equation) {
		*ruleEquation = equation;
		setupRuleSymbolEquationsTable();

		emit equationReferenceChanged();
	}
}

void
RuleManagerWindow::on_updateCommentButton_clicked()
{
	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;

	selectedRule_->setComment(ui_->commentTextEdit->toPlainText().toStdString());
}

// Slot.
void
RuleManagerWindow::setupRuleTransitionsTable()
{
	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;

	QTableWidget* table = ui_->ruleTransitionsTable;

	QSignalBlocker blocker(table);

	table->setRowCount(model_->parameterList().size());
	for (unsigned int i = 0, size = model_->parameterList().size(); i < size; ++i) {
		const auto& parameter = model_->parameterList()[i];

		auto item = std::make_unique<QTableWidgetItem>(parameter.name().c_str());
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(i, 0, item.release());

		const auto& transition = selectedRule_->getParamProfileTransition(i);
		item = std::make_unique<QTableWidgetItem>(transition ? transition->name().c_str() : "");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(i, 1, item.release());
	}
}

// Slot.
void
RuleManagerWindow::setupTransitionsTree()
{
	if (model_ == nullptr) return;

	QTreeWidget* tree = ui_->transitionsTree;
	tree->clear();
	for (const auto& group : model_->transitionGroupList()) {
		auto item = std::make_unique<QTreeWidgetItem>();
		item->setText(0, group.name.c_str());
		item->setFlags(Qt::ItemIsEnabled);

		for (const auto& transition : group.transitionList) {
			auto childItem = std::make_unique<QTreeWidgetItem>();
			childItem->setText(0, transition->name().c_str());
			childItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->addChild(childItem.release());
		}

		tree->addTopLevelItem(item.release());
	}
	tree->expandAll();
}

// Slot.
void
RuleManagerWindow::setupRuleSpecialTransitionsTable()
{
	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;

	QTableWidget* table = ui_->ruleSpecialTransitionsTable;

	QSignalBlocker blocker(table);

	table->setRowCount(model_->parameterList().size());
	for (unsigned int i = 0, size = model_->parameterList().size(); i < size; ++i) {
		const auto& parameter = model_->parameterList()[i];

		auto item = std::make_unique<QTableWidgetItem>(parameter.name().c_str());
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(i, 0, item.release());

		const auto& transition = selectedRule_->getSpecialProfileTransition(i);
		item = std::make_unique<QTableWidgetItem>(transition ? transition->name().c_str() : "");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(i, 1, item.release());
	}
}

// Slot.
void
RuleManagerWindow::setupSpecialTransitionsTree()
{
	if (model_ == nullptr) return;

	QTreeWidget* tree = ui_->specialTransitionsTree;
	tree->clear();
	for (const auto& group : model_->specialTransitionGroupList()) {
		auto item = std::make_unique<QTreeWidgetItem>();
		item->setText(0, group.name.c_str());
		item->setFlags(Qt::ItemIsEnabled);

		for (const auto& specialTransition : group.transitionList) {
			auto childItem = std::make_unique<QTreeWidgetItem>();
			childItem->setText(0, specialTransition->name().c_str());
			childItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->addChild(childItem.release());
		}

		tree->addTopLevelItem(item.release());
	}
	tree->expandAll();
}

// Slot.
void
RuleManagerWindow::setupRuleSymbolEquationsTable()
{
	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;

	QTableWidget* table = ui_->ruleSymbolEquationsTable;

	QSignalBlocker blocker(table);

	table->setRowCount(5);
	{
		auto item = std::make_unique<QTableWidgetItem>("Rule duration");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(0, 0, item.release());

		const auto& equation = selectedRule_->exprSymbolEquations().duration;
		item = std::make_unique<QTableWidgetItem>(equation ? equation->name().c_str() : "");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(0, 1, item.release());
	}
	{
		auto item = std::make_unique<QTableWidgetItem>("Beat location");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(1, 0, item.release());

		const auto& equation = selectedRule_->exprSymbolEquations().beat;
		item = std::make_unique<QTableWidgetItem>(equation ? equation->name().c_str() : "");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(1, 1, item.release());
	}
	{
		auto item = std::make_unique<QTableWidgetItem>("Mark 1");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(2, 0, item.release());

		const auto& equation = selectedRule_->exprSymbolEquations().mark1;
		item = std::make_unique<QTableWidgetItem>(equation ? equation->name().c_str() : "");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(2, 1, item.release());
	}
	{
		auto item = std::make_unique<QTableWidgetItem>("Mark 2");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(3, 0, item.release());

		const auto& equation = selectedRule_->exprSymbolEquations().mark2;
		item = std::make_unique<QTableWidgetItem>(equation ? equation->name().c_str() : "");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(3, 1, item.release());
	}
	{
		auto item = std::make_unique<QTableWidgetItem>("Mark 3");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(4, 0, item.release());

		const auto& equation = selectedRule_->exprSymbolEquations().mark3;
		item = std::make_unique<QTableWidgetItem>(equation ? equation->name().c_str() : "");
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		table->setItem(4, 1, item.release());
	}
}

// Slot.
void
RuleManagerWindow::setupEquationsTree()
{
	if (model_ == nullptr) return;

	QTreeWidget* tree = ui_->equationsTree;
	tree->clear();
	for (const auto& group : model_->equationGroupList()) {
		auto item = std::make_unique<QTreeWidgetItem>();
		item->setText(0, group.name.c_str());
		item->setFlags(Qt::ItemIsEnabled);

		for (const auto& equation : group.equationList) {
			auto childItem = std::make_unique<QTreeWidgetItem>();
			childItem->setText(0, equation->name().c_str());
			childItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->addChild(childItem.release());
		}

		tree->addTopLevelItem(item.release());
	}
	tree->expandAll();
}

void
RuleManagerWindow::on_clearSpecialTransitionButton_clicked()
{
	qDebug("on_clearSpecialTransitionButton_clicked");

	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;

	QTableWidgetItem* currentItem = ui_->ruleSpecialTransitionsTable->currentItem();
	if (currentItem == nullptr) return;

	int row = currentItem->row();

	std::shared_ptr<VTMControlModel::Transition> empty;
	selectedRule_->setSpecialProfileTransition(row, empty);

	setupRuleSpecialTransitionsTable();
	ui_->specialTransitionsTree->setCurrentItem(nullptr);

	emit specialTransitionReferenceChanged();
}

void
RuleManagerWindow::on_clearEquationButton_clicked()
{
	qDebug("on_clearEquationButton_clicked");

	if (model_ == nullptr) return;
	if (selectedRule_ == nullptr) return;

	QTableWidgetItem* currentItem = ui_->ruleSymbolEquationsTable->currentItem();
	if (currentItem == nullptr) return;

	std::shared_ptr<VTMControlModel::Equation>* ruleEquation = nullptr;
	int row = currentItem->row();
	switch (row) {
	case 0:
		ruleEquation = &selectedRule_->exprSymbolEquations().duration;
		break;
	case 1:
		ruleEquation = &selectedRule_->exprSymbolEquations().beat;
		break;
	case 2:
		ruleEquation = &selectedRule_->exprSymbolEquations().mark1;
		break;
	case 3:
		ruleEquation = &selectedRule_->exprSymbolEquations().mark2;
		break;
	case 4:
		ruleEquation = &selectedRule_->exprSymbolEquations().mark3;
		break;
	default:
		return;
	}

	ruleEquation->reset();

	setupRuleSymbolEquationsTable();
	ui_->equationsTree->setCurrentItem(nullptr);

	emit equationReferenceChanged();
}

} // namespace GS
