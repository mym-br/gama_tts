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

#include "PrototypeManagerWindow.h"

#include <memory>
#include <sstream>
#include <utility> /* move, swap */

#include <QMessageBox>
#include <QSignalBlocker>

#include "Exception.h"
#include "Model.h"
#include "ui_PrototypeManagerWindow.h"

#define NUM_EQUATIONS_TREE_COLUMNS 2
#define NUM_TRANSITIONS_TREE_COLUMNS 2
#define NEW_ITEM_NAME "___new___"
#define NEW_EQUATION_FORMULA "0.0"



namespace GS {

PrototypeManagerWindow::PrototypeManagerWindow(QWidget* parent)
		: QWidget(parent)
		, ui_(std::make_unique<Ui::PrototypeManagerWindow>())
		, model_()
		, currentEquation_()
		, currentTransition_()
		, currentSpecialTransition_()
{
	ui_->setupUi(this);

	ui_->equationsTree->setColumnCount(NUM_EQUATIONS_TREE_COLUMNS);
	ui_->equationsTree->setHeaderLabels(QStringList() << tr("Equation") << tr("Is used?"));

	ui_->transitionsTree->setColumnCount(NUM_TRANSITIONS_TREE_COLUMNS);
	ui_->transitionsTree->setHeaderLabels(QStringList() << tr("Transition") << tr("Is used?"));

	ui_->specialTransitionsTree->setColumnCount(NUM_TRANSITIONS_TREE_COLUMNS);
	ui_->specialTransitionsTree->setHeaderLabels(QStringList() << tr("Special transition") << tr("Is used?"));

	ui_->specialTransitionWidget->setSpecial();
}

PrototypeManagerWindow::~PrototypeManagerWindow()
{
}

void
PrototypeManagerWindow::resetModel(VTMControlModel::Model* model)
{
	model_ = model;

	if (model_ == nullptr) {
		clearEquationData();
		ui_->equationsTree->clear();

		clearTransitionData();
		ui_->transitionsTree->clear();

		clearSpecialTransitionData();
		ui_->specialTransitionsTree->clear();
	} else {
		setupEquationsTree();
		setupTransitionsTree();
		setupSpecialTransitionsTree();
	}
}

//=============================================================================
// Equations.
//

void
PrototypeManagerWindow::on_addEquationButton_clicked()
{
	if (model_ == nullptr) return;

	QModelIndex currentIndex = ui_->equationsTree->currentIndex();
	if (currentIndex.isValid() && !currentIndex.parent().isValid()) { // root item is selected
		unsigned int groupIndex = currentIndex.row();
		if (model_->findEquationName(NEW_ITEM_NAME)) {
			QMessageBox::critical(this, tr("Error"), tr("Duplicate equation name."));
			return;
		}
		auto equation = std::make_shared<VTMControlModel::Equation>(NEW_ITEM_NAME);
		try {
			equation->setFormula(NEW_EQUATION_FORMULA);
		} catch (const Exception& exc) {
			QMessageBox::critical(this, tr("Error"), exc.what());
			return;
		}
		model_->equationGroupList()[groupIndex].equationList.push_back(std::move(equation));
	} else {
		if (model_->findEquationGroupName(NEW_ITEM_NAME)) {
			QMessageBox::critical(this, tr("Error"), tr("Duplicate equation group name."));
			return;
		}
		VTMControlModel::EquationGroup equationGroup;
		equationGroup.name = NEW_ITEM_NAME;
		model_->equationGroupList().push_back(std::move(equationGroup));
	}
	setupEquationsTree();
	emit equationChanged();
}

void
PrototypeManagerWindow::on_removeEquationButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->equationsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) {
		unsigned int groupIndex = currentIndex.parent().row();
		unsigned int index = currentIndex.row();
		auto& equationList = model_->equationGroupList()[groupIndex].equationList;
		auto iter = equationList.begin() + index;
		if (iter->use_count() > 1) {
			QMessageBox::critical(this, tr("Error"), tr("Can't remove the equation: it is being used."));
			return;
		}
		equationList.erase(iter);
	} else { // root item
		unsigned int groupIndex = currentIndex.row();
		if (!model_->equationGroupList()[groupIndex].equationList.empty()) {
			QMessageBox::critical(this, tr("Error"), tr("Can't remove the group: the group is not empty."));
			return;
		}
		model_->equationGroupList().erase(model_->equationGroupList().begin() + groupIndex);
	}
	setupEquationsTree();
	emit equationChanged();
}

void
PrototypeManagerWindow::on_moveEquationUpButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->equationsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) { // equation
		unsigned int groupRow = currentIndex.parent().row();
		unsigned int row = currentIndex.row();

		auto& equationList = model_->equationGroupList()[groupRow].equationList;
		if (row > 0) {
			std::swap(equationList[row], equationList[row - 1]);
			setupEquationsTree();
			QModelIndex groupIndex = ui_->equationsTree->model()->index(groupRow, 0);
			ui_->equationsTree->setCurrentIndex(ui_->equationsTree->model()->index(row - 1, 0, groupIndex));
		} else if (groupRow > 0) {
			auto& newGroup = model_->equationGroupList()[groupRow - 1];
			newGroup.equationList.push_back(std::move(equationList[0]));
			equationList.erase(equationList.begin());
			setupEquationsTree();
			QModelIndex newGroupIndex = ui_->equationsTree->model()->index(groupRow - 1, 0);
			ui_->equationsTree->setCurrentIndex(ui_->equationsTree->model()->index(newGroup.equationList.size() - 1, 0, newGroupIndex));
		}
	} else { // root item : group
		unsigned int groupRow = currentIndex.row();
		if (groupRow == 0) {
			return;
		}
		auto& groupList = model_->equationGroupList();
		std::swap(groupList[groupRow], groupList[groupRow - 1]);
		setupEquationsTree();
		ui_->equationsTree->setCurrentIndex(ui_->equationsTree->model()->index(groupRow - 1, 0));
	}
	emit equationChanged();
}

void
PrototypeManagerWindow::on_moveEquationDownButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->equationsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) { // equation
		unsigned int groupRow = currentIndex.parent().row();
		unsigned int row = currentIndex.row();

		auto& equationList = model_->equationGroupList()[groupRow].equationList;
		if (row < equationList.size() - 1) {
			std::swap(equationList[row], equationList[row + 1]);
			setupEquationsTree();
			QModelIndex groupIndex = ui_->equationsTree->model()->index(groupRow, 0);
			ui_->equationsTree->setCurrentIndex(ui_->equationsTree->model()->index(row + 1, 0, groupIndex));
		} else if (groupRow < model_->equationGroupList().size() - 1) {
			auto& newGroup = model_->equationGroupList()[groupRow + 1];
			newGroup.equationList.insert(newGroup.equationList.begin(), std::move(equationList.back()));
			equationList.pop_back();
			setupEquationsTree();
			QModelIndex newGroupIndex = ui_->equationsTree->model()->index(groupRow + 1, 0);
			ui_->equationsTree->setCurrentIndex(ui_->equationsTree->model()->index(0, 0, newGroupIndex));
		}
	} else { // root item : group
		unsigned int groupRow = currentIndex.row();
		auto& groupList = model_->equationGroupList();
		if (groupRow >= groupList.size() - 1) {
			return;
		}
		std::swap(groupList[groupRow], groupList[groupRow + 1]);
		setupEquationsTree();
		ui_->equationsTree->setCurrentIndex(ui_->equationsTree->model()->index(groupRow + 1, 0));
	}
	emit equationChanged();
}

void
PrototypeManagerWindow::on_updateEquationButton_clicked()
{
	if (model_ == nullptr) return;
	if (currentEquation_ == nullptr) {
		return;
	}

	try {
		currentEquation_->setFormula(ui_->equationFormulaTextEdit->toPlainText().toStdString());
	} catch (const Exception& exc) {
		ui_->eqParserMessagesTextEdit->setPlainText(exc.what());
		return;
	}
	currentEquation_->setComment(ui_->equationCommentTextEdit->toPlainText().toStdString());
	ui_->eqParserMessagesTextEdit->setPlainText(tr("No errors."));

	std::ostringstream out;
	out << "\nFormula tree:\n" << *currentEquation_;
	ui_->eqParserMessagesTextEdit->appendPlainText(out.str().c_str());
	ui_->eqParserMessagesTextEdit->moveCursor(QTextCursor::Start);
	emit equationChanged();
}

void
PrototypeManagerWindow::on_equationsTree_itemChanged(QTreeWidgetItem* item, int /*column*/)
{
	if (model_ == nullptr) return;

	QTreeWidgetItem* parent = item->parent();
	std::string newText = item->text(0).toStdString();
	if (parent == nullptr) { // root item
		int index = ui_->equationsTree->indexOfTopLevelItem(item);
		if (model_->findEquationGroupName(newText)) {
			{
				QSignalBlocker blocker(ui_->equationsTree);
				item->setText(0, model_->equationGroupList()[index].name.c_str());
			}
			QMessageBox::critical(this, tr("Error"), tr("Duplicate equation group name."));
			return;
		}
		model_->equationGroupList()[index].name = newText;
	} else {
		int parentIndex = ui_->equationsTree->indexOfTopLevelItem(parent);
		int index = parent->indexOfChild(item);
		if (model_->findEquationName(newText)) {
			{
				QSignalBlocker blocker(ui_->equationsTree);
				item->setText(0, model_->equationGroupList()[parentIndex].equationList[index]->name().c_str());
			}
			QMessageBox::critical(this, tr("Error"), tr("Duplicate equation name."));
			return;
		}
		model_->equationGroupList()[parentIndex].equationList[index]->setName(newText);
	}
	emit equationChanged();
}

void
PrototypeManagerWindow::on_equationsTree_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/)
{
	if (model_ == nullptr) return;
	if (current == nullptr) {
		clearEquationData();
		return;
	}
	QTreeWidgetItem* parent = current->parent();
	if (parent == nullptr) { // root item
		clearEquationData();
		return;
	}
	int parentIndex = ui_->equationsTree->indexOfTopLevelItem(parent);
	int index = parent->indexOfChild(current);
	auto& equation = model_->equationGroupList()[parentIndex].equationList[index];
	ui_->equationCommentTextEdit->setPlainText(equation->comment().c_str());
	ui_->equationFormulaTextEdit->setPlainText(equation->formula().c_str());
	ui_->eqParserMessagesTextEdit->document()->clear();
	currentEquation_ = equation.get();
}

// Slot.
void
PrototypeManagerWindow::setupEquationsTree()
{
	qDebug("PrototypeManagerWindow::setupEquationsTree");

	if (model_ == nullptr) return;

	{
		QTreeWidget* tree = ui_->equationsTree;
		QSignalBlocker blocker(tree);

		tree->clear();

		for (const auto& group : model_->equationGroupList()) {
			auto item = std::make_unique<QTreeWidgetItem>();
			item->setText(0, group.name.c_str());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

			for (const auto& equation : group.equationList) {
				auto childItem = std::make_unique<QTreeWidgetItem>();
				childItem->setText(0, equation->name().c_str());
				childItem->setData(1, Qt::CheckStateRole, equation.use_count() > 1 ? Qt::Checked : Qt::Unchecked);
				childItem->setData(1, Qt::DisplayRole, static_cast<int>(equation.use_count()) - 1);
				childItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
				item->addChild(childItem.release());
			}

			tree->addTopLevelItem(item.release());
		}

		tree->expandAll();
		tree->resizeColumnToContents(0);
	}
	clearEquationData();
}

void
PrototypeManagerWindow::clearEquationData()
{
	qDebug("PrototypeManagerWindow::clearEquationData");

	ui_->equationCommentTextEdit->document()->clear();
	ui_->equationFormulaTextEdit->document()->clear();
	ui_->eqParserMessagesTextEdit->document()->clear();
	currentEquation_ = nullptr;
}

//=============================================================================
// Transitions.
//

void
PrototypeManagerWindow::on_addTransitionButton_clicked()
{
	if (model_ == nullptr) return;

	QModelIndex currentIndex = ui_->transitionsTree->currentIndex();
	if (currentIndex.isValid() && !currentIndex.parent().isValid()) { // root item is selected
		unsigned int groupIndex = currentIndex.row();
		if (model_->findTransitionName(NEW_ITEM_NAME)) {
			QMessageBox::critical(this, tr("Error"), tr("Duplicate transition name."));
			return;
		}

		auto transition = std::make_shared<VTMControlModel::Transition>(NEW_ITEM_NAME, VTMControlModel::Transition::Type::diphone, false);
		model_->transitionGroupList()[groupIndex].transitionList.push_back(transition);
	} else {
		if (model_->findTransitionGroupName(NEW_ITEM_NAME)) {
			QMessageBox::critical(this, tr("Error"), tr("Duplicate transition group name."));
			return;
		}
		VTMControlModel::TransitionGroup transitionGroup;
		transitionGroup.name = NEW_ITEM_NAME;
		model_->transitionGroupList().push_back(std::move(transitionGroup));
	}
	setupTransitionsTree();
	emit transitionChanged();
}

void
PrototypeManagerWindow::on_removeTransitionButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->transitionsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) {
		unsigned int groupIndex = currentIndex.parent().row();
		unsigned int index = currentIndex.row();
		auto& transitionList = model_->transitionGroupList()[groupIndex].transitionList;
		auto iter = transitionList.begin() + index;
		if (iter->use_count() > 1) {
			QMessageBox::critical(this, tr("Error"), tr("Can't remove the transition: it is being used."));
			return;
		}
		transitionList.erase(iter);
	} else { // root item
		unsigned int groupIndex = currentIndex.row();
		if (!model_->transitionGroupList()[groupIndex].transitionList.empty()) {
			QMessageBox::critical(this, tr("Error"), tr("Can't remove the group: the group is not empty."));
			return;
		}
		model_->transitionGroupList().erase(model_->transitionGroupList().begin() + groupIndex);
	}
	setupTransitionsTree();
	emit transitionChanged();
}

void
PrototypeManagerWindow::on_moveTransitionUpButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->transitionsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) { // transition
		unsigned int groupRow = currentIndex.parent().row();
		unsigned int row = currentIndex.row();

		auto& transitionList = model_->transitionGroupList()[groupRow].transitionList;
		if (row > 0) {
			std::swap(transitionList[row], transitionList[row - 1]);
			setupTransitionsTree();
			QModelIndex groupIndex = ui_->transitionsTree->model()->index(groupRow, 0);
			ui_->transitionsTree->setCurrentIndex(ui_->transitionsTree->model()->index(row - 1, 0, groupIndex));
		} else if (groupRow > 0) {
			auto& newGroup = model_->transitionGroupList()[groupRow - 1];
			newGroup.transitionList.push_back(std::move(transitionList[0]));
			transitionList.erase(transitionList.begin());
			setupTransitionsTree();
			QModelIndex newGroupIndex = ui_->transitionsTree->model()->index(groupRow - 1, 0);
			ui_->transitionsTree->setCurrentIndex(ui_->transitionsTree->model()->index(newGroup.transitionList.size() - 1, 0, newGroupIndex));
		}
	} else { // root item : group
		unsigned int groupRow = currentIndex.row();
		if (groupRow == 0) {
			return;
		}
		auto& groupList = model_->transitionGroupList();
		std::swap(groupList[groupRow], groupList[groupRow - 1]);
		setupTransitionsTree();
		ui_->transitionsTree->setCurrentIndex(ui_->transitionsTree->model()->index(groupRow - 1, 0));
	}
	emit transitionChanged();
}

void
PrototypeManagerWindow::on_moveTransitionDownButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->transitionsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) { // transition
		unsigned int groupRow = currentIndex.parent().row();
		unsigned int row = currentIndex.row();

		auto& transitionList = model_->transitionGroupList()[groupRow].transitionList;
		if (row < transitionList.size() - 1) {
			std::swap(transitionList[row], transitionList[row + 1]);
			setupTransitionsTree();
			QModelIndex groupIndex = ui_->transitionsTree->model()->index(groupRow, 0);
			ui_->transitionsTree->setCurrentIndex(ui_->transitionsTree->model()->index(row + 1, 0, groupIndex));
		} else if (groupRow < model_->transitionGroupList().size() - 1) {
			auto& newGroup = model_->transitionGroupList()[groupRow + 1];
			newGroup.transitionList.insert(newGroup.transitionList.begin(), std::move(transitionList.back()));
			transitionList.pop_back();
			setupTransitionsTree();
			QModelIndex newGroupIndex = ui_->transitionsTree->model()->index(groupRow + 1, 0);
			ui_->transitionsTree->setCurrentIndex(ui_->transitionsTree->model()->index(0, 0, newGroupIndex));
		}
	} else { // root item : group
		unsigned int groupRow = currentIndex.row();
		auto& groupList = model_->transitionGroupList();
		if (groupRow >= groupList.size() - 1) {
			return;
		}
		std::swap(groupList[groupRow], groupList[groupRow + 1]);
		setupTransitionsTree();
		ui_->transitionsTree->setCurrentIndex(ui_->transitionsTree->model()->index(groupRow + 1, 0));
	}
	emit transitionChanged();
}

void
PrototypeManagerWindow::on_updateTransitionButton_clicked()
{
	if (model_ == nullptr) return;
	if (currentTransition_ == nullptr) {
		return;
	}

	currentTransition_->setComment(ui_->transitionCommentTextEdit->toPlainText().toStdString());
}

void
PrototypeManagerWindow::on_editTransitionButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->transitionsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) { // transition
		unsigned int groupRow = currentIndex.parent().row();
		unsigned int row = currentIndex.row();
		emit editTransitionButtonClicked(groupRow, row);

		setupEquationsTree();
	}
}

void
PrototypeManagerWindow::on_transitionsTree_itemChanged(QTreeWidgetItem* item, int /*column*/)
{
	if (model_ == nullptr) return;

	QTreeWidgetItem* parent = item->parent();
	std::string newText = item->text(0).toStdString();
	if (parent == nullptr) { // root item
		int index = ui_->transitionsTree->indexOfTopLevelItem(item);
		if (model_->findTransitionGroupName(newText)) {
			{
				QSignalBlocker blocker(ui_->transitionsTree);
				item->setText(0, model_->transitionGroupList()[index].name.c_str());
			}
			QMessageBox::critical(this, tr("Error"), tr("Duplicate transition group name."));
			return;
		}
		model_->transitionGroupList()[index].name = newText;
	} else {
		int parentIndex = ui_->transitionsTree->indexOfTopLevelItem(parent);
		int index = parent->indexOfChild(item);
		if (model_->findTransitionName(newText)) {
			{
				QSignalBlocker blocker(ui_->transitionsTree);
				item->setText(0, model_->transitionGroupList()[parentIndex].transitionList[index]->name().c_str());
			}
			QMessageBox::critical(this, tr("Error"), tr("Duplicate transition name."));
			return;
		}
		model_->transitionGroupList()[parentIndex].transitionList[index]->setName(newText);
	}
	emit transitionChanged();
}

void
PrototypeManagerWindow::on_transitionsTree_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/)
{
	if (model_ == nullptr) return;
	if (current == nullptr) {
		clearTransitionData();
		return;
	}
	QTreeWidgetItem* parent = current->parent();
	if (parent == nullptr) { // root item
		clearTransitionData();
		return;
	}
	int parentIndex = ui_->transitionsTree->indexOfTopLevelItem(parent);
	int index = parent->indexOfChild(current);
	auto& transition = model_->transitionGroupList()[parentIndex].transitionList[index];
	ui_->transitionCommentTextEdit->setPlainText(transition->comment().c_str());

	TransitionPoint::copyPointsFromTransition(*transition, pointList_);

	model_->setDefaultFormulaSymbols(transition->type());
	TransitionPoint::calculateTimes(*model_, pointList_);

	TransitionPoint::sortPointListByTypeAndTime(specialPointList_);
	TransitionPoint::adjustValuesInSlopeRatios(pointList_);

	ui_->transitionWidget->updateData(
		transition->type(),
		&pointList_,
		model_->getFormulaSymbolValue(VTMControlModel::FormulaSymbol::SYMB_RULE_DURATION),
		model_->getFormulaSymbolValue(VTMControlModel::FormulaSymbol::SYMB_MARK1),
		model_->getFormulaSymbolValue(VTMControlModel::FormulaSymbol::SYMB_MARK2),
		model_->getFormulaSymbolValue(VTMControlModel::FormulaSymbol::SYMB_MARK3)
	);

	currentTransition_ = transition.get();
}

// Slot.
void
PrototypeManagerWindow::setupTransitionsTree()
{
	if (model_ == nullptr) return;

	{
		QTreeWidget* tree = ui_->transitionsTree;
		QSignalBlocker blocker(tree);

		tree->clear();

		for (const auto& group : model_->transitionGroupList()) {
			auto item = std::make_unique<QTreeWidgetItem>();
			item->setText(0, group.name.c_str());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

			for (const auto& transition : group.transitionList) {
				auto childItem = std::make_unique<QTreeWidgetItem>();
				childItem->setText(0, transition->name().c_str());
				childItem->setData(1, Qt::CheckStateRole, transition.use_count() > 1 ? Qt::Checked : Qt::Unchecked);
				childItem->setData(1, Qt::DisplayRole, static_cast<int>(transition.use_count()) - 1);
				childItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
				item->addChild(childItem.release());
			}

			tree->addTopLevelItem(item.release());
		}

		tree->expandAll();
		tree->resizeColumnToContents(0);
	}
	clearTransitionData();
}

void
PrototypeManagerWindow::clearTransitionData()
{
	ui_->transitionCommentTextEdit->document()->clear();
	ui_->transitionWidget->clear();
	pointList_.clear();
	currentTransition_ = nullptr;
}

//=============================================================================
// Special transitions.
//

void
PrototypeManagerWindow::on_addSpecialTransitionButton_clicked()
{
	if (model_ == nullptr) return;

	QModelIndex currentIndex = ui_->specialTransitionsTree->currentIndex();
	if (currentIndex.isValid() && !currentIndex.parent().isValid()) { // root item is selected
		unsigned int groupIndex = currentIndex.row();
		if (model_->findSpecialTransitionName(NEW_ITEM_NAME)) {
			QMessageBox::critical(this, tr("Error"), tr("Duplicate special transition name."));
			return;
		}

		auto specialTransition = std::make_shared<VTMControlModel::Transition>(NEW_ITEM_NAME, VTMControlModel::Transition::Type::diphone, true);
		model_->specialTransitionGroupList()[groupIndex].transitionList.push_back(specialTransition);
	} else {
		if (model_->findSpecialTransitionGroupName(NEW_ITEM_NAME)) {
			QMessageBox::critical(this, tr("Error"), tr("Duplicate special transition group name."));
			return;
		}
		VTMControlModel::TransitionGroup specialTransitionGroup;
		specialTransitionGroup.name = NEW_ITEM_NAME;
		model_->specialTransitionGroupList().push_back(std::move(specialTransitionGroup));
	}
	setupSpecialTransitionsTree();
	emit specialTransitionChanged();
}

void
PrototypeManagerWindow::on_removeSpecialTransitionButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->specialTransitionsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) {
		unsigned int groupIndex = currentIndex.parent().row();
		unsigned int index = currentIndex.row();
		auto& specialTransitionList = model_->specialTransitionGroupList()[groupIndex].transitionList;
		auto iter = specialTransitionList.begin() + index;
		if (iter->use_count() > 1) {
			QMessageBox::critical(this, tr("Error"), tr("Can't remove the special transition: it is being used."));
			return;
		}
		specialTransitionList.erase(iter);
	} else { // root item
		unsigned int groupIndex = currentIndex.row();
		if (!model_->specialTransitionGroupList()[groupIndex].transitionList.empty()) {
			QMessageBox::critical(this, tr("Error"), tr("Can't remove the group: the group is not empty."));
			return;
		}
		model_->specialTransitionGroupList().erase(model_->specialTransitionGroupList().begin() + groupIndex);
	}
	setupSpecialTransitionsTree();
	emit specialTransitionChanged();
}

void
PrototypeManagerWindow::on_moveSpecialTransitionUpButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->specialTransitionsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) { // special transition
		unsigned int groupRow = currentIndex.parent().row();
		unsigned int row = currentIndex.row();

		auto& specialTransitionList = model_->specialTransitionGroupList()[groupRow].transitionList;
		if (row > 0) {
			std::swap(specialTransitionList[row], specialTransitionList[row - 1]);
			setupSpecialTransitionsTree();
			QModelIndex groupIndex = ui_->specialTransitionsTree->model()->index(groupRow, 0);
			ui_->specialTransitionsTree->setCurrentIndex(ui_->specialTransitionsTree->model()->index(row - 1, 0, groupIndex));
		} else if (groupRow > 0) {
			auto& newGroup = model_->specialTransitionGroupList()[groupRow - 1];
			newGroup.transitionList.push_back(std::move(specialTransitionList[0]));
			specialTransitionList.erase(specialTransitionList.begin());
			setupSpecialTransitionsTree();
			QModelIndex newGroupIndex = ui_->specialTransitionsTree->model()->index(groupRow - 1, 0);
			ui_->specialTransitionsTree->setCurrentIndex(ui_->specialTransitionsTree->model()->index(newGroup.transitionList.size() - 1, 0, newGroupIndex));
		}
	} else { // root item : group
		unsigned int groupRow = currentIndex.row();
		if (groupRow == 0) {
			return;
		}
		auto& groupList = model_->specialTransitionGroupList();
		std::swap(groupList[groupRow], groupList[groupRow - 1]);
		setupSpecialTransitionsTree();
		ui_->specialTransitionsTree->setCurrentIndex(ui_->specialTransitionsTree->model()->index(groupRow - 1, 0));
	}
	emit specialTransitionChanged();
}

void
PrototypeManagerWindow::on_moveSpecialTransitionDownButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->specialTransitionsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) { // special transition
		unsigned int groupRow = currentIndex.parent().row();
		unsigned int row = currentIndex.row();

		auto& specialTransitionList = model_->specialTransitionGroupList()[groupRow].transitionList;
		if (row < specialTransitionList.size() - 1) {
			std::swap(specialTransitionList[row], specialTransitionList[row + 1]);
			setupSpecialTransitionsTree();
			QModelIndex groupIndex = ui_->specialTransitionsTree->model()->index(groupRow, 0);
			ui_->specialTransitionsTree->setCurrentIndex(ui_->specialTransitionsTree->model()->index(row + 1, 0, groupIndex));
		} else if (groupRow < model_->specialTransitionGroupList().size() - 1) {
			auto& newGroup = model_->specialTransitionGroupList()[groupRow + 1];
			newGroup.transitionList.insert(newGroup.transitionList.begin(), std::move(specialTransitionList.back()));
			specialTransitionList.pop_back();
			setupSpecialTransitionsTree();
			QModelIndex newGroupIndex = ui_->specialTransitionsTree->model()->index(groupRow + 1, 0);
			ui_->specialTransitionsTree->setCurrentIndex(ui_->specialTransitionsTree->model()->index(0, 0, newGroupIndex));
		}
	} else { // root item : group
		unsigned int groupRow = currentIndex.row();
		auto& groupList = model_->specialTransitionGroupList();
		if (groupRow >= groupList.size() - 1) {
			return;
		}
		std::swap(groupList[groupRow], groupList[groupRow + 1]);
		setupSpecialTransitionsTree();
		ui_->specialTransitionsTree->setCurrentIndex(ui_->specialTransitionsTree->model()->index(groupRow + 1, 0));
	}
	emit specialTransitionChanged();
}

void
PrototypeManagerWindow::on_updateSpecialTransitionButton_clicked()
{
	if (model_ == nullptr) return;
	if (currentSpecialTransition_ == nullptr) {
		return;
	}

	currentSpecialTransition_->setComment(ui_->specialTransitionCommentTextEdit->toPlainText().toStdString());
}

void
PrototypeManagerWindow::on_editSpecialTransitionButton_clicked()
{
	if (model_ == nullptr) return;
	QModelIndex currentIndex = ui_->specialTransitionsTree->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	if (currentIndex.parent().isValid()) { // special transition
		unsigned int groupRow = currentIndex.parent().row();
		unsigned int row = currentIndex.row();
		emit editSpecialTransitionButtonClicked(groupRow, row);

		setupEquationsTree();
	}
}

void
PrototypeManagerWindow::on_specialTransitionsTree_itemChanged(QTreeWidgetItem* item, int /*column*/)
{
	if (model_ == nullptr) return;

	QTreeWidgetItem* parent = item->parent();
	std::string newText = item->text(0).toStdString();
	if (parent == nullptr) { // root item
		int index = ui_->specialTransitionsTree->indexOfTopLevelItem(item);
		if (model_->findSpecialTransitionGroupName(newText)) {
			{
				QSignalBlocker blocker(ui_->specialTransitionsTree);
				item->setText(0, model_->specialTransitionGroupList()[index].name.c_str());
			}
			QMessageBox::critical(this, tr("Error"), tr("Duplicate special transition group name."));
			return;
		}
		model_->specialTransitionGroupList()[index].name = newText;
	} else {
		int parentIndex = ui_->specialTransitionsTree->indexOfTopLevelItem(parent);
		int index = parent->indexOfChild(item);
		if (model_->findSpecialTransitionName(newText)) {
			{
				QSignalBlocker blocker(ui_->specialTransitionsTree);
				item->setText(0, model_->specialTransitionGroupList()[parentIndex].transitionList[index]->name().c_str());
			}
			QMessageBox::critical(this, tr("Error"), tr("Duplicate special transition name."));
			return;
		}
		model_->specialTransitionGroupList()[parentIndex].transitionList[index]->setName(newText);
	}
	emit specialTransitionChanged();
}

void
PrototypeManagerWindow::on_specialTransitionsTree_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/)
{
	if (model_ == nullptr) return;
	if (current == nullptr) {
		clearSpecialTransitionData();
		return;
	}
	QTreeWidgetItem* parent = current->parent();
	if (parent == nullptr) { // root item
		clearSpecialTransitionData();
		return;
	}
	int parentIndex = ui_->specialTransitionsTree->indexOfTopLevelItem(parent);
	int index = parent->indexOfChild(current);
	auto& specialTransition = model_->specialTransitionGroupList()[parentIndex].transitionList[index];
	ui_->specialTransitionCommentTextEdit->setPlainText(specialTransition->comment().c_str());

	TransitionPoint::copyPointsFromTransition(*specialTransition, specialPointList_);

	model_->setDefaultFormulaSymbols(specialTransition->type());
	TransitionPoint::calculateTimes(*model_, specialPointList_);

	TransitionPoint::sortPointListByTime(specialPointList_);

	ui_->specialTransitionWidget->updateData(
		specialTransition->type(),
		&specialPointList_,
		model_->getFormulaSymbolValue(VTMControlModel::FormulaSymbol::SYMB_RULE_DURATION),
		model_->getFormulaSymbolValue(VTMControlModel::FormulaSymbol::SYMB_MARK1),
		model_->getFormulaSymbolValue(VTMControlModel::FormulaSymbol::SYMB_MARK2),
		model_->getFormulaSymbolValue(VTMControlModel::FormulaSymbol::SYMB_MARK3)
	);

	currentSpecialTransition_ = specialTransition.get();
}

// Slot.
void
PrototypeManagerWindow::setupSpecialTransitionsTree()
{
	if (model_ == nullptr) return;

	{
		QTreeWidget* tree = ui_->specialTransitionsTree;
		QSignalBlocker blocker(tree);

		tree->clear();

		for (const auto& group : model_->specialTransitionGroupList()) {
			auto item = std::make_unique<QTreeWidgetItem>();
			item->setText(0, group.name.c_str());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

			for (const auto& specialTransition : group.transitionList) {
				auto childItem = std::make_unique<QTreeWidgetItem>();
				childItem->setText(0, specialTransition->name().c_str());
				childItem->setData(1, Qt::CheckStateRole, specialTransition.use_count() > 1 ? Qt::Checked : Qt::Unchecked);
				childItem->setData(1, Qt::DisplayRole, static_cast<int>(specialTransition.use_count()) - 1);
				childItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
				item->addChild(childItem.release());
			}

			tree->addTopLevelItem(item.release());
		}

		tree->expandAll();
		tree->resizeColumnToContents(0);
	}
	clearSpecialTransitionData();
}

// Slot.
void
PrototypeManagerWindow::unselectTransition()
{
	ui_->transitionsTree->setCurrentItem(nullptr);
}

// Slot.
void
PrototypeManagerWindow::unselectSpecialTransition()
{
	ui_->specialTransitionsTree->setCurrentItem(nullptr);
}

void
PrototypeManagerWindow::clearSpecialTransitionData()
{
	ui_->specialTransitionCommentTextEdit->document()->clear();
	ui_->specialTransitionWidget->clear();
	specialPointList_.clear();
	currentSpecialTransition_ = nullptr;
}

} // namespace GS
